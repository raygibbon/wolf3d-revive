/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_VL_H
#define ID_VL_H
#include "w3r_types.h"
#include <stdbool.h>
#include <stddef.h>

#define VL_PIXELS ((size_t)W3R_SCREEN_WIDTH * W3R_SCREEN_HEIGHT)
extern w3r_byte framebuffer[VL_PIXELS];
extern w3r_byte palette[768]; /* Original 0..63 DAC values, RGB order. */
extern const w3r_byte gamepal[768];
bool VL_SetPalette(const w3r_byte *source, size_t length);
/* Four consecutive planes, width divisible by four; no partial writes on error. */
bool VL_MemToScreen(const w3r_byte *source, size_t length,
                    int width, int height, int x, int y);
bool VL_ConvertRGBA(w3r_byte *dest, size_t length);
#endif
