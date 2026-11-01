/* SPDX-License-Identifier: GPL-2.0-only */
/* Portable type baseline. Original DOS headers remain untouched. */
#ifndef W3R_TYPES_H
#define W3R_TYPES_H

#include <limits.h>
#include <stdint.h>

typedef uint8_t w3r_byte;
typedef uint16_t w3r_word;
typedef uint32_t w3r_longword;
typedef int32_t w3r_fixed;

_Static_assert(CHAR_BIT == 8, "Wolf3D data requires 8-bit bytes");
_Static_assert(sizeof(w3r_word) == 2, "Wolf3D words must be 16-bit");
_Static_assert(sizeof(w3r_longword) == 4, "Wolf3D longwords must be 32-bit");
_Static_assert(sizeof(w3r_fixed) == 4, "Wolf3D fixed-point values must be 32-bit");

enum { W3R_SCREEN_WIDTH = 320, W3R_SCREEN_HEIGHT = 200, W3R_TICK_HZ = 70 };

#endif
