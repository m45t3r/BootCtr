#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#include "filesystem.h"
#include "loader.h"
#include "config.h"
#include "misc.h"

#include "boot_screen_bgr.h"

#define DEFAULT_PATH NULL
#define DEFAULT_DELAY 2500 /* ms */
#define DEFAULT_PAYLOAD -1 /* <0 - auto, 0 - disable, >0 - enabled */
#define DEFAULT_OFFSET 0x12000
#define DEFAULT_SECTION "GLOBAL"
#define INI_FILE "/boot_config.ini"

// handled in main
// doing it in main is preferred because execution ends in launching another 3dsx
void __appInit()
{
    srvInit();
    aptInit();
    gfxInitDefault();
    initFilesystem();
    openSDArchive();
    hidInit();
}

// same
void __appExit()
{
    hidExit();
    gfxExit();
    closeSDArchive();
    exitFilesystem();
    aptExit();
    srvExit();
}

int main()
{
    // offset potential issues caused by homebrew that just ran
    aptOpenSession();
    APT_SetAppCpuTimeLimit(0);
    aptCloseSession();

    // set application default preferences
    application app = {
        .config = {
            .section = DEFAULT_SECTION,
            .path = DEFAULT_PATH,
            .delay = DEFAULT_DELAY,
            .payload = DEFAULT_PAYLOAD,
            .offset = DEFAULT_OFFSET,
        }
    };

    // override default options with [GLOBAL] section
    // don't need to check error for now
    ini_parse(INI_FILE, handler, &app.config);

    // get pressed user key, convert to string to pass to ini_parse
    hidScanInput();
    u32 key = hidKeysDown();
    switch (key) {
        // using X-macros to generate each switch-case rules
        // https://en.wikibooks.org/wiki/C_Programming/Preprocessor#X-Macros
        #define KEY(k) \
        case KEY_##k: \
            app.config.section = "KEY_"#k; \
            break;
        #include "keys.def"
        default:
            app.config.section = "DEFAULT";
            break;
    }

    // load config from section [KEY_*], check error for issues
    int config_err = ini_parse(INI_FILE, handler, &app.config);
    switch (config_err) {
        case 0:
            // section not found, try to load [DEFAULT] section
            if (!app.config.path) {
                app.config.section = "DEFAULT";
                // don't need to check error again
                ini_parse(INI_FILE, handler, &app.config);
                if (!app.config.path)
                    panic("Section [DEFAULT] not found or \"path\" not set");
            } else if (app.config.path[0] != '/') {
                panic("In section [%s], %s is an invalid path",
                        app.config.section, app.config.path);
            } else if (!file_exists(app.config.path)) {
                panic("In section [%s], file %s not found",
                        app.config.section, app.config.path);
            }
            break;
        case -1:
            panic("Config file %s not found", INI_FILE);
            break;
        case -2:
            // should not happen, however better be safe than sorry
            panic("Config file %s too big", INI_FILE);
            break;
        default:
            panic("Error found in config file %s on line %d",
                    INI_FILE, config_err);
            break;
    }

    // load boot image in top screen
    gfxSetDoubleBuffering(GFX_TOP, false);
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    memcpy(fb, boot_screen_bgr, boot_screen_bgr_size);
    // print entry information in bottom screen
    consoleInit(GFX_BOTTOM, NULL);
    printf("Booting entry:\n\n"
           "[%s]\n"
           "\tpath = %s\n"
           "\tdelay = %llu\n"
           "\tpayload = %d\n"
           "\toffset = 0x%lx",
           app.config.section, app.config.path, app.config.delay,
           app.config.payload, app.config.offset);
    // flush, swap framebuffers and wait for VBlank
    // also known as Voodoo™ fix
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();

    return load(app);
}
