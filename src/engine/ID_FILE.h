/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_FILE_H
#define ID_FILE_H
#include "ID_MM.h"
#include <stdint.h>
static inline uint16_t CA_LE16(const unsigned char *p) { return (uint16_t)((unsigned)p[0] | ((unsigned)p[1]<<8)); }
static inline uint32_t CA_LE32(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
bool CA_ReadDataFile(const char *name, memptr *data, size_t *size);
const char *CA_DataDirectory(void);
#endif
