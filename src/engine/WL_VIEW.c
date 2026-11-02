/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_MAIN.C NewViewSize / WL_MENU.C CP_ChangeView, without compiled scalers. */
#include "WL_DEF.h"
#include "WL_MENU.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "ID_SD.h"
uint16_t viewwidth=240,viewheight=120,viewxorigin=40,viewyorigin=20;
static unsigned preview;
static uint64_t repeat;
unsigned ViewSize(void) { return (unsigned)viewwidth/16; }
bool NewViewSize(unsigned size)
{
    if(size<4 || size>19) return CA_Fail("View size must be 4 through 19");
    viewwidth=(uint16_t)(size*16);viewheight=(uint16_t)(size*8);
    viewxorigin=(uint16_t)((320-viewwidth)/2);viewyorigin=(uint16_t)((160-viewheight)/2);
    CalcProjection();return true;
}
static bool DrawPreview(void)
{
    unsigned old=ViewSize();
    if(!NewViewSize(preview)) return false;
    DrawPlayBorder();
    if(!NewViewSize(old)) return false;
    VWB_Bar(0,160,320,40,127);fontnumber=1;fontcolor=HIGHLIGHT;
    static const char *const lines[]={"Use arrows to size","ENTER to accept","ESC to cancel"};
    for(unsigned i=0;i<3;++i) {
        int w,h;if(!VW_MeasurePropString(lines[i],&w,&h)) return false;
        PrintX=(320-w)/2;PrintY=161+(int)i*h;if(!US_Print(lines[i])) return false;
    }
    return true;
}
bool BeginViewMenu(void) { preview=ViewSize();repeat=0;IN_ClearKeysDown();return DrawPreview(); }
int HandleViewMenu(uint64_t ticks)
{
    if(IN_ConsumePress(sc_Escape)) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return -1; }
    if(IN_ConsumePress(sc_Enter)) {
        if(!NewViewSize(preview)) return -3;
        (void)SD_PlaySound(SHOOTSND,0,0);return 1;
    }
    bool smaller=IN_ConsumePress(sc_LeftArrow)|IN_ConsumePress(sc_DownArrow);
    bool larger=IN_ConsumePress(sc_RightArrow)|IN_ConsumePress(sc_UpArrow);
    if(!smaller && !larger && ticks>=repeat) {
        smaller=Keyboard[sc_LeftArrow] || Keyboard[sc_DownArrow];
        larger=Keyboard[sc_RightArrow] || Keyboard[sc_UpArrow];
    }
    if(smaller || larger) {
        if(smaller && preview>4) --preview;else if(!smaller && preview<19) ++preview;
        repeat=ticks+10;(void)SD_PlaySound(HITWALLSND,0,0);return DrawPreview()?0:-3;
    }
    return 0;
}
