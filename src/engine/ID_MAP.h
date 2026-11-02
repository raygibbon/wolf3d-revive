/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_MAP_H
#define ID_MAP_H
#include "w3r_types.h"
#include <stdbool.h>
#include <stddef.h>
#define MAPSIZE 64
#define MAPAREA 4096
typedef struct { w3r_word planes[3][MAPAREA]; char name[17]; unsigned number; } maptype;
bool CAL_CarmackExpand(const w3r_byte *source,size_t bytes,w3r_word *dest,size_t words);
bool CA_RLEWexpand(const w3r_word *source,size_t words,w3r_word *dest,size_t output_words,w3r_word tag);
bool CA_ReadMap(unsigned number,maptype *destination);
bool CA_InstallMap(const maptype *map);
bool CA_CacheMap(unsigned number);
const maptype *CA_Map(void);
void CA_UncacheMap(void);
#endif
