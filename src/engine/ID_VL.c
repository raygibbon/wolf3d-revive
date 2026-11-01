/* SPDX-License-Identifier: GPL-2.0-only */
/* WOLFSRC/ID_VL.C: VGA planes and DAC writes translated to portable buffers. */
#include "ID_VL.h"
#include <string.h>

w3r_byte framebuffer[VL_PIXELS];
w3r_byte palette[768];
const w3r_byte gamepal[768] = {
#include "gamepal.inc"
};

bool VL_SetPalette(const w3r_byte *source, size_t length)
{
    if (!source || length != sizeof(palette)) return false;
    for (size_t i = 0; i < length; ++i) if (source[i] > 63) return false;
    memcpy(palette, source, length);
    return true;
}

bool VL_MemToScreen(const w3r_byte *source, size_t length,
                    int width, int height, int x, int y)
{
    size_t plane_size, row_size;
    if (!source || width <= 0 || height <= 0 || width > W3R_SCREEN_WIDTH ||
        height > W3R_SCREEN_HEIGHT || width % 4 || x < 0 || y < 0 ||
        x > W3R_SCREEN_WIDTH - width || y > W3R_SCREEN_HEIGHT - height)
        return false;
    if (length != (size_t)width * (size_t)height) return false;
    row_size = (size_t)width / 4;
    plane_size = row_size * (size_t)height;
    for (size_t plane = 0; plane < 4; ++plane)
        for (size_t row = 0; row < (size_t)height; ++row)
            for (size_t col = 0; col < row_size; ++col)
                framebuffer[((size_t)y + row) * W3R_SCREEN_WIDTH + (size_t)x + col * 4 + plane] =
                    source[plane * plane_size + row * row_size + col];
    return true;
}

bool VL_ConvertRGBA(w3r_byte *dest, size_t length)
{
    if (!dest || length < VL_PIXELS * 4) return false;
    for (size_t i = 0; i < VL_PIXELS; ++i) {
        size_t color = (size_t)framebuffer[i] * 3;
        for (size_t channel = 0; channel < 3; ++channel) {
            unsigned value = palette[color + channel];
            dest[i * 4 + channel] = (w3r_byte)((value << 2) | (value >> 4));
        }
        dest[i * 4 + 3] = 255;
    }
    return true;
}
