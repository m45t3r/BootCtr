#ifndef BOOT_H
#define BOOT_H

#include <3ds.h>
#include "titles.h"

extern int targetProcessId;
extern titleInfo_s target_title;

bool isNinjhax2(void);
int bootApp(char* executablePath);

#endif
