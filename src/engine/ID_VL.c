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

/* ID_VL.C interpolation: orig + (target-orig)*i/steps, truncating toward zero. */
static w3r_byte fade_from[768], fade_to[768];
static uint64_t fade_start;
static unsigned fade_steps;
bool VL_Fading(void) { return fade_steps != 0; }
bool VL_FadeIn(const w3r_byte target[768], unsigned steps, uint64_t ticks)
{
    if (!target || steps > 1000) return false;
    for (size_t i = 0; i < 768; ++i) if (target[i] > 63) return false;
    memcpy(fade_from, palette, 768); memcpy(fade_to, target, 768);
    fade_start = ticks; fade_steps = steps;
    if (!steps) (void)VL_SetPalette(target, 768);
    return true;
}
bool VL_FadeOut(unsigned red, unsigned green, unsigned blue, unsigned steps, uint64_t ticks)
{
    w3r_byte target[768];
    if (red > 63 || green > 63 || blue > 63) return false;
    for (size_t i = 0; i < 768; i += 3) {
        target[i] = (w3r_byte)red; target[i + 1] = (w3r_byte)green; target[i + 2] = (w3r_byte)blue;
    }
    return VL_FadeIn(target, steps, ticks);
}
void VL_FadeStep(uint64_t ticks)
{
    if (!fade_steps || ticks < fade_start) return;
    uint64_t elapsed = ticks - fade_start;
    if (elapsed >= fade_steps) {
        (void)VL_SetPalette(fade_to, 768); fade_steps = 0; return;
    }
    for (size_t i = 0; i < 768; ++i) {
        int original = fade_from[i], delta = (int)fade_to[i] - original;
        palette[i] = (w3r_byte)(original + delta * (int)elapsed / (int)fade_steps);
    }
}
