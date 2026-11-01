/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_INTER.C DrawHighScores: original default table from ID_US_1.C.
   No save/config import or score-entry gameplay has been ported. */
#include "WL_MENU.h"
#include "ID_VH.h"
#include "ID_CA.h"
#include "ID_US_1.h"
bool DrawHighScores(void)
{
    static const char *names[] = {"id software-'92", "Adrian Carmack", "John Carmack", "Kevin Cloud", "Tom Hall", "John Romero", "Jay Wilbur"};
    const char level[] = {(char)130,0};
    const char score[] = {(char)130,(char)129,(char)129,(char)129,(char)129,0};
    ClearMScreen(); DrawStripes(10);
    if (!VWB_DrawPic(48,0,HIGHSCORESPIC) || !VWB_DrawPic(32,68,C_NAMEPIC) ||
        !VWB_DrawPic(160,68,C_LEVELPIC) || !VWB_DrawPic(224,68,C_SCOREPIC)) return false;
    fontnumber = 0; fontcolor = 15;
    for (int i = 0; i < 7; ++i) {
        int width, height;
        PrintX = 32; PrintY = 76 + 16*i;
        if (!US_Print(names[i]) || !VW_MeasurePropString(level,&width,&height)) return false;
        PrintX = 22*8-width-6;
        if (!US_Print("E1/L") || !US_Print(level) || !VW_MeasurePropString(score,&width,&height)) return false;
        PrintX = 34*8-8-width;
        if (!US_Print(score)) return false;
    }
    return true;
}
