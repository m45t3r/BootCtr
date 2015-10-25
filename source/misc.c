#include "misc.h"

void init_services()
{
    srvInit();
    aptInit();
    initFilesystem();
    openSDArchive();
    hidInit(NULL);
    irrstInit(NULL);
    acInit();
    ptmInit();
    amInit();
}

void exit_services()
{
    amExit();
    ptmExit();
    acExit();
    irrstExit();
    hidExit();
    closeSDArchive();
    exitFilesystem();
    aptExit();
    srvExit();
}

int file_exists(const char *fname)
{
    FILE *file = fopen(fname, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// http://stackoverflow.com/a/5309508
const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

void reboot()
{
	aptInit();
	aptOpenSession();
	APT_HardwareResetAsync(NULL);
	aptCloseSession();
	aptExit();
	exit(EXIT_SUCCESS);
}

void print_error(const char *msg, ...)
{
	va_list args;
	char fmt[512];

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	va_start(args, msg);
	vsprintf(fmt, msg, args);
	va_end(args);

	printf("ERROR: %s\n", fmt);
	printf("Press START to reboot...\n");

	while (aptMainLoop()) {
		hidScanInput();
		if (hidKeysDown() & KEY_START) break;

		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}
	gfxExit();
	reboot();
}
