/* SPDX-License-Identifier: GPL-2.0-only */
/* ID_IN.C keyboard make/break state and ASCII lookup, ported from DOS set-1. */
#include "ID_IN.h"
#include "WL_CONTROLS.h"
#include <string.h>
bool Keyboard[128];
static bool pressed[128];
static unsigned ascii_scans[128], ascii_read, ascii_write;
uint8_t LastScan;
char LastASCII;
char IN_ScanASCII(unsigned scan)
{
    static const char row1[] = "qwertyuiop", row2[] = "asdfghjkl", row3[] = "zxcvbnm";
    if (scan >= 0x10 && scan <= 0x19) return row1[scan - 0x10];
    if (scan >= 0x1e && scan <= 0x26) return row2[scan - 0x1e];
    if (scan >= 0x2c && scan <= 0x32) return row3[scan - 0x2c];
    if (scan >= 2 && scan <= 10) return (char)('1' + scan - 2);
    if (scan == 11) return '0';
    if (scan == sc_Space) return ' ';
    return 0;
}
void IN_ClearPressed(void) { memset(pressed, 0, sizeof(pressed)); LastScan = 0; LastASCII = 0; ascii_read = ascii_write = 0;CT_EndTic(); }
void IN_ClearKeysDown(void) { memset(Keyboard, 0, sizeof(Keyboard)); IN_ClearPressed();CT_Clear(); }
void IN_KeyEvent(unsigned scan,bool down) { CT_LegacyKey(scan,down);IN_MenuKeyEvent(scan,down); }
void IN_MenuKeyEvent(unsigned scan, bool down)
{
    if (!scan || scan >= 128) return;
    if (down && !Keyboard[scan]) { pressed[scan] = true; LastScan = (uint8_t)scan; char ascii = IN_ScanASCII(scan); if (ascii) {
        LastASCII = ascii;
        unsigned next = (ascii_write+1)%128;
        if (next == ascii_read) ascii_read = (ascii_read+1)%128;
        ascii_scans[ascii_write] = scan; ascii_write = next;
    } }
    Keyboard[scan] = down;
}
bool IN_ConsumePress(unsigned scan)
{
    if (scan >= 128) return false;
    bool result = pressed[scan]; pressed[scan] = false; return result;
}
bool IN_AnyPressed(void)
{
    for (unsigned i = 1; i < 128; ++i) if (pressed[i]) return true;
    return false;
}

char IN_NextASCII(void)
{
    if (ascii_read == ascii_write) return 0;
    unsigned scan = ascii_scans[ascii_read]; ascii_read = (ascii_read+1)%128;
    char ascii = IN_ScanASCII(scan);
    if (ascii != ' ') pressed[scan] = false;
    return ascii;
}
bool IN_PressPending(unsigned scan) { return scan<128 && pressed[scan]; }
