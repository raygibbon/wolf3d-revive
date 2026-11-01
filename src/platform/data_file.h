/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DATA_FILE_H
#define DATA_FILE_H
#include <stdio.h>
/* Exact spelling wins; otherwise one unambiguous ASCII case-insensitive match. */
FILE *CA_OpenDataFile(const char *directory, const char *name);
#endif
