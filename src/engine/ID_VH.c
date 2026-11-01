/* SPDX-License-Identifier: GPL-2.0-only */
/* ID_VH.C drawing/measurement: original planar pictures and row-major fonts. */
#include "ID_VH.h"
#include "ID_CA.h"
#include <limits.h>
#include <string.h>
int px, py, fontnumber;
w3r_byte fontcolor;
static unsigned LE16(const w3r_byte *p) { return (unsigned)p[0] | ((unsigned)p[1] << 8); }

bool VW_ValidateFont(int number)
{
    if (number < 0 || number >= NUMFONT) return CA_Fail("Invalid font number");
    int chunk = STARTFONT + number;
    if (!CA_CacheGrChunk(chunk)) return false;
    const w3r_byte *raw = grsegs[chunk];
    size_t length = grsizes[chunk];
    if (length < 770 || !LE16(raw) || LE16(raw) > 200) return CA_Fail("Malformed font header");
    for (size_t i = 0; i < 256; ++i) {
        size_t width = raw[514 + i], offset = LE16(raw + 2 + i * 2);
        size_t pixels = width * LE16(raw);
        if (width && (offset < 770 || offset > length || pixels > length - offset))
            return CA_Fail("Font glyph %zu extends outside its chunk", i);
    }
    return true;
}

bool VWB_DrawPic(int x, int y, int chunk)
{
    if (chunk < STARTPICS || chunk >= STARTPICS + NUMPICS) return CA_Fail("Invalid picture index");
    if (!CA_CacheGrChunk(chunk)) return false;
    pictabletype pic = pictable[chunk - STARTPICS];
    x &= ~7; /* Original buffered picture alignment, ID_VH.C. */
    return VL_MemToScreen(grsegs[chunk], grsizes[chunk], pic.width, pic.height, x, y) ||
           CA_Fail("Malformed picture dimensions/length or drawing bounds");
}

void VWB_Bar(int x, int y, int width, int height, w3r_byte color)
{
    if (width <= 0 || height <= 0 || x < 0 || y < 0 || x > 320 - width || y > 200 - height) return;
    for (int row = y; row < y + height; ++row) memset(framebuffer + row * 320 + x, color, (size_t)width);
}
void VWB_Hlin(int x1, int x2, int y, w3r_byte color) { if (x1 >= 0 && x2 >= x1 && x2 < 320) VWB_Bar(x1, y, x2 - x1 + 1, 1, color); }
void VWB_Vlin(int y1, int y2, int x, w3r_byte color) { if (y1 >= 0 && y2 >= y1 && y2 < 200) VWB_Bar(x, y1, 1, y2 - y1 + 1, color); }

bool VW_MeasurePropString(const char *text, int *width, int *height)
{
    if (!text || !width || !height || !VW_ValidateFont(fontnumber)) return false;
    const w3r_byte *font = grsegs[STARTFONT + fontnumber];
    *width = 0; *height = (int)LE16(font);
    for (const unsigned char *s = (const unsigned char *)text; *s; ++s) {
        if (*width > 32000) return CA_Fail("Font string too wide");
        *width += font[514 + *s];
    }
    return true;
}

bool VW_DrawPropString(const char *text)
{
    int width, height;
    if (!VW_MeasurePropString(text, &width, &height)) return false;
    if (px < -32000 || px > 32000 || py < -200 || py > 200) return CA_Fail("Text origin out of bounds");
    const w3r_byte *font = grsegs[STARTFONT + fontnumber];
    for (const unsigned char *s = (const unsigned char *)text; *s; ++s) {
        int step = font[514 + *s];
        if (!step) continue;
        const w3r_byte *source = font + LE16(font + 2 + (size_t)*s * 2);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < step; ++x)
                if (px + x >= 0 && px + x < 320 && py + y >= 0 && py + y < 200 && source[y * step + x])
                    framebuffer[(py + y) * 320 + px + x] = fontcolor;
        px += step;
    }
    return true;
}
