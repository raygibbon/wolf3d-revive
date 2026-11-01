/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_VH.h"
#include "engine/ID_US_1.h"
#include "engine/ID_IN.h"
#include "engine/ID_TIME.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,CA_Error()); exit(1); } } while (0)
static void Key(unsigned scan) { IN_KeyEvent(scan,true); IN_KeyEvent(scan,false); }
static void TestClockAndFade(void)
{
    CHECK(TimeCount_FromNS(0) == 0);
    CHECK(TimeCount_FromNS(1000000000) == 70);
    CHECK(TimeCount_FromNS(14285714) == 0 && TimeCount_FromNS(14285715) == 1);
    CHECK(TimeCount_FromNS(UINT64_MAX) == UINT64_C(1291272085159));
    w3r_byte gray[768]; memset(gray,31,sizeof(gray));
    CHECK(VL_SetPalette(gray,768));
    CHECK(VL_FadeOut(0,0,0,3,100));
    VL_FadeStep(99); CHECK(palette[0] == 31);
    VL_FadeStep(101); CHECK(palette[0] == 21 && VL_Fading());
    VL_FadeStep(102); CHECK(palette[0] == 11);
    VL_FadeStep(103); CHECK(palette[0] == 0 && !VL_Fading());
    CHECK(VL_FadeIn(gray,3,103));
    VL_FadeStep(104); CHECK(palette[0] == 10);
    VL_FadeStep(106); CHECK(palette[0] == 31 && !VL_Fading());
    CHECK(!VL_FadeOut(64,0,0,3,0) && !VL_FadeIn(gray,1001,0));
    CHECK(VL_FadeOut(0,0,0,0,0));
}
static void TestInput(void)
{
    IN_ClearKeysDown(); IN_KeyEvent(0x1e,true);
    CHECK(Keyboard[0x1e] && LastScan == 0x1e && LastASCII == 'a');
    CHECK(IN_ConsumePress(0x1e));
    IN_KeyEvent(0x1e,true); CHECK(!IN_ConsumePress(0x1e));
    IN_KeyEvent(0x1e,false); CHECK(!Keyboard[0x1e]);
    Key(sc_Enter); CHECK(!Keyboard[sc_Enter] && IN_ConsumePress(sc_Enter));
    IN_KeyEvent(999,true); CHECK(!IN_ConsumePress(999));
    IN_ClearKeysDown(); CHECK(!IN_AnyPressed());
}
static void TestDraw(bool synthetic)
{
    CHECK(VW_ValidateFont(0) && VW_ValidateFont(1));
    MainItems.curpos = viewscores;
    CHECK(SetupControlPanel(0));
    CHECK(framebuffer[199*320] == BORDCOLOR && framebuffer[12*320] == 0);
    CHECK(framebuffer[32*320] == STRIPE && framebuffer[54*320+69] == BKGDCOLOR);
    if (synthetic) {
        CHECK(framebuffer[80] == C_OPTIONSPIC);
        CHECK(framebuffer[(53+6*13)*320+72] == C_CURSOR1PIC);
    }
    fontnumber = 1; fontcolor = 19;
    int w,h; CHECK(VW_MeasurePropString("A",&w,&h));
    VWB_Bar(0,0,320,200,45); px = 3; py = 4;
    CHECK(VW_DrawPropString("A"));
    if (synthetic) {
        CHECK(w == 5 && h == 13);
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
            CHECK(framebuffer[(4+y)*320+3+x] == ((x+y+65)%3 ? 19 : 45));
    }
    PrintX = WindowX = 3; PrintY = 4;
    CHECK(US_Print("A\nA")); CHECK(PrintX == 3+w && PrintY == 4+h);
    w3r_byte *font = grsegs[2];
    w3r_byte low = font[0], high = font[1];
    font[0] = font[1] = 0; CHECK(!VW_ValidateFont(1)); font[0] = low; font[1] = high;
    size_t size = grsizes[2]; grsizes[2] = 769; CHECK(!VW_ValidateFont(1)); grsizes[2] = size;
    size_t at = 2+65*2; low = font[at]; high = font[at+1];
    font[at] = font[at+1] = 255; CHECK(!VW_ValidateFont(1)); font[at] = low; font[at+1] = high;
    CHECK(DrawHighScores() && DrawMainMenu());
}
static void TestAttract(void)
{
    CHECK(DemoLoopStartup(0,false)); CHECK(DemoLoopState() == UI_TITLE && palette[5] == 0);
    CHECK(DemoLoop(30)); CHECK(!VL_Fading());
    CHECK(DemoLoop(1079)); CHECK(DemoLoopState() == UI_TITLE);
    CHECK(DemoLoop(1080)); CHECK(DemoLoopState() == UI_TITLE && VL_Fading());
    CHECK(DemoLoop(1110)); CHECK(DemoLoopState() == UI_CREDITS);
    CHECK(DemoLoop(1870)); CHECK(DemoLoopState() == UI_HIGHSCORES);
    CHECK(DemoLoop(2640)); CHECK(DemoLoopState() == UI_MENU && !VL_Fading());
    MainItems.curpos = viewscores;
    IN_KeyEvent(sc_DownArrow,true); CHECK(DemoLoop(2640)); CHECK(MainItems.curpos == backtodemo);
    CHECK(DemoLoop(2659)); CHECK(MainItems.curpos == backtodemo);
    CHECK(DemoLoop(2660)); CHECK(MainItems.curpos == quit);
    IN_KeyEvent(sc_DownArrow,false); CHECK(DemoLoop(2661));
    Key(sc_DownArrow); CHECK(DemoLoop(2662)); CHECK(MainItems.curpos == viewscores);
    Key(sc_UpArrow); CHECK(DemoLoop(2663)); CHECK(MainItems.curpos == quit);
    Key(0x2f); CHECK(DemoLoop(2664)); CHECK(MainItems.curpos == viewscores); /* V shortcut */
    Key(sc_Enter); CHECK(DemoLoop(2665)); CHECK(DemoLoop(2685)); CHECK(DemoLoopState() == UI_SCORES);
    Key(sc_Escape); CHECK(DemoLoop(2686)); CHECK(DemoLoop(2706)); CHECK(DemoLoopState() == UI_MENU);
    Key(sc_Escape); CHECK(DemoLoop(2707)); CHECK(DemoLoopState() == UI_CONFIRM);
    Key(sc_N); CHECK(DemoLoop(2708)); CHECK(DemoLoopState() == UI_MENU);
    Key(0x30); CHECK(DemoLoop(2709)); CHECK(MainItems.curpos == backtodemo); /* B shortcut */
    Key(sc_Enter); CHECK(DemoLoop(2710)); CHECK(DemoLoop(2720)); CHECK(DemoLoopState() == UI_TITLE);
    Key(sc_Enter); CHECK(DemoLoop(2721)); CHECK(DemoLoop(2761)); CHECK(DemoLoopState() == UI_MENU);
    Key(0x10); CHECK(DemoLoop(2762)); CHECK(MainItems.curpos == quit); /* Q shortcut */
    Key(sc_Enter); CHECK(DemoLoop(2763)); CHECK(DemoLoopState() == UI_CONFIRM);
    Key(sc_Y); CHECK(DemoLoop(2764)); CHECK(DemoLoop(2774)); CHECK(DemoLoopState() == UI_DONE);
    /* A single coarse clock update reaches the same menu; no frame-count timing. */
    CHECK(DemoLoopStartup(0,false) && DemoLoop(2640)); CHECK(DemoLoopState() == UI_MENU);
    MainItems.curpos = viewscores; CHECK(SetupControlPanel(0)); IN_ClearKeysDown();
    IN_KeyEvent(sc_DownArrow,true); CHECK(HandleMenu(0) == -2); IN_ClearPressed();
    CHECK(HandleMenu(60) == -2); int coarse = MainItems.curpos;
    MainItems.curpos = viewscores; CHECK(SetupControlPanel(0)); IN_ClearKeysDown();
    IN_KeyEvent(sc_DownArrow,true); CHECK(HandleMenu(0) == -2); IN_ClearPressed();
    for (uint64_t t = 20; t <= 60; t += 20) CHECK(HandleMenu(t) == -2);
    CHECK(MainItems.curpos == coarse); IN_ClearKeysDown();
}
int main(int argc, char **argv)
{
    CHECK(argc >= 2); TestClockAndFade(); TestInput(); MM_Startup();
    CHECK(CA_Startup(argv[1])); CHECK(VL_SetPalette(gamepal,768));
    TestDraw(argc == 2); TestAttract();
    CA_Shutdown(); MM_Shutdown(); CHECK(!MM_TotalBytes());
    puts("Original UI drawing, malformed fonts, input, fades and 70 Hz attract/menu tests passed.");
    return 0;
}
