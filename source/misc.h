#pragma once

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "filesystem.h"

//#define DEBUG

void init_services();
void exit_services();
int file_exists(const char *fname);
const char *get_filename_ext(const char *filename);
void reboot();
void wait_key(const u32 key);
void print_debug(const char *msg, ...);
void print_error(const char *msg, ...);
void boot_fix(int delay);
