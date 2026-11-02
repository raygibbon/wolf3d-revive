/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SAVE_FILE_H
#define SAVE_FILE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
FILE *SaveFile_Open(const char *path);
bool SaveFile_Missing(const char *path);
bool SaveFile_WriteAtomic(const char *path,const void *bytes,size_t length,bool overwrite);
#endif
