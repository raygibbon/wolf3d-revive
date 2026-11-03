/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_CA_H
#define ID_CA_H
#include "ID_MM.h"
#include "w3r_types.h"
#include "GFXV_WL6.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct { w3r_word bit0, bit1; } huffnode;
typedef struct { w3r_word width, height; } pictabletype;
extern memptr grsegs[NUMCHUNKS];
extern size_t grsizes[NUMCHUNKS];
extern pictabletype pictable[NUMPICS];

bool CAL_ValidateHuffman(const huffnode table[255]);
/* Same root/LSB-first codes as the original asm, now with source bounds. */
bool CAL_HuffExpand(const w3r_byte *source, size_t source_length,
                    w3r_byte *dest, size_t length, const huffnode table[255]);
bool CA_Startup(const char *data_dir);
void CA_Shutdown(void);
bool CA_CacheGrChunk(int chunk);
bool CA_GrChunkPresent(int chunk);
bool CA_CacheScreen(int chunk);
const char *CA_Error(void);
bool CA_Fail(const char *format, ...);
#endif
