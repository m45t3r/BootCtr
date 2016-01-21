#include <3ds.h>

#include <stdio.h>
#include <string.h>

#include "boot.h"
#include "filesystem.h"

extern void (*__system_retAddr)(void);

static Handle hbFileHandle;
static u32 argbuffer[0x100];
static u32 argbuffer_length = 0;

// ninjhax 1.x
void (*callBootloader_1x)(Handle hb, Handle file);
void (*setArgs_1x)(u32* src, u32 length);

static void launchFile_1x(void)
{
    // jump to bootloader
    callBootloader_1x(0x00000000, hbFileHandle);
}

void (*callBootloader_2x)
    (Handle file, u32* argbuf, u32 arglength) = (void*)0x00100000;
void (*callBootloaderNewProcess_2x)
    (int processId, u32* argbuf, u32 arglength) = (void*)0x00100008;
void (*callBootloaderRunTitle_2x)
    (u8 mediatype, u32* argbuf, u32 argbuflength,
     u32 tid_low, u32 tid_high) = (void*)0x00100010;

int targetProcessId = -1;
titleInfo_s target_title;

static void launchFile_2x(void)
{
    // jump to bootloader
    if (targetProcessId == -1) {
        callBootloader_2x(hbFileHandle, argbuffer, argbuffer_length);
    } else if (targetProcessId == -2) {
        callBootloaderRunTitle_2x(target_title.mediatype, argbuffer,
                argbuffer_length, target_title.title_id & 0xffffffff,
                (target_title.title_id >> 32) & 0xffffffff);
    } else {
        callBootloaderNewProcess_2x(targetProcessId, argbuffer, argbuffer_length);
    }
}

bool isNinjhax2(void)
{
    Result ret = hbInit();
    if (!ret) {
        hbExit();
        return false;
    } else {
        return true;
    }
}

int bootApp(char* executablePath)
{
    // set argv/argc
    argbuffer_length = sizeof(argbuffer);
    argbuffer[0] = 1;
    snprintf((char*)&argbuffer[1], sizeof(argbuffer) - 4, "sdmc:%s", executablePath);

    // open file that we're going to boot up
    fsInit();
    FSUSER_OpenFileDirectly(&hbFileHandle, sdmcArchive,
            fsMakePath(PATH_ASCII, executablePath),
            FS_OPEN_READ, 0);
    fsExit();

    // figure out the preferred way of running the 3dsx
    if(!hbInit()) {
        // ninjhax 1.x !
        // grab bootloader addresses
        HB_GetBootloaderAddresses((void**)&callBootloader_1x, (void**)&setArgs_1x);
        hbExit();
        // set argv
        setArgs_1x(argbuffer, sizeof(argbuffer));
        // override return address to homebrew booting code
        __system_retAddr = launchFile_1x;
    } else {
        // ninjhax 2.0+
        // override return address to homebrew booting code
        __system_retAddr = launchFile_2x;
    }

    return 0;
}
