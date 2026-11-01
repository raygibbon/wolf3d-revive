/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_VH_H
#define ID_VH_H
#include "ID_VL.h"
extern int px, py, fontnumber;
extern w3r_byte fontcolor;
bool VW_ValidateFont(int number);
bool VWB_DrawPic(int x, int y, int chunk);
bool VW_MeasurePropString(const char *text, int *width, int *height);
bool VW_DrawPropString(const char *text);
void VWB_Bar(int x, int y, int width, int height, w3r_byte color);
void VWB_Hlin(int x1, int x2, int y, w3r_byte color);
void VWB_Vlin(int y1, int y2, int x, w3r_byte color);
#endif
