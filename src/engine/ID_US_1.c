/* SPDX-License-Identifier: GPL-2.0-only */
/* ID_US_1.C, original user manager by Jason Blochowiak.
   US_Print now accepts immutable strings without modifying them. */
#include "ID_US_1.h"
#include "ID_VH.h"
#include "ID_CA.h"
#include <string.h>
int PrintX, PrintY, WindowX;
bool US_Print(const char *text)
{
    if (!text) return CA_Fail("Missing UI text");
    char line[256];
    while (*text) {
        const char *end = strchr(text, '\n');
        size_t size = end ? (size_t)(end - text) : strlen(text);
        int width, height;
        if (size >= sizeof(line)) return CA_Fail("UI text line too long");
        memcpy(line, text, size); line[size] = 0;
        if (!VW_MeasurePropString(line, &width, &height)) return false;
        px = PrintX; py = PrintY;
        if (!VW_DrawPropString(line)) return false;
        if (!end) { PrintX += width; break; }
        PrintX = WindowX; PrintY += height; text = end + 1;
    }
    return true;
}
