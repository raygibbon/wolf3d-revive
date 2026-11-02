/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef W3R_CONFIG_FILE_H
#define W3R_CONFIG_FILE_H
#include <stdbool.h>
typedef enum { CONFIG_MISSING,CONFIG_LOADED,CONFIG_INVALID,CONFIG_UNSUPPORTED,CONFIG_IO_ERROR } config_result;
config_result Config_Load(const char *directory);
bool Config_Save(const char *directory);
const char *Config_Error(void);
#endif
