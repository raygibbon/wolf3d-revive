/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_PLAY.C keyboard controls driving the original one-tic ControlMovement.
   Simulation is explicitly 70 Hz; presentation is independent. */
#include "WL_DEF.h"
#include "ID_IN.h"
#include "ID_CA.h"
#include "ID_VH.h"
static uint64_t lasttimecount;
static bool running;
static bool DrawPlayScreen(void)
{
    VWB_Bar(0,0,320,160,127);
    VWB_Bar(VIEWX,VIEWY,VIEWWIDTH,VIEWHEIGHT,0);
    VWB_Hlin(VIEWX-1,VIEWX+VIEWWIDTH,VIEWY-1,0);
    VWB_Hlin(VIEWX-1,VIEWX+VIEWWIDTH,VIEWY+VIEWHEIGHT,125);
    VWB_Vlin(VIEWY-1,VIEWY+VIEWHEIGHT,VIEWX-1,0);
    VWB_Vlin(VIEWY-1,VIEWY+VIEWHEIGHT,VIEWX+VIEWWIDTH,125);
    framebuffer[(VIEWY+VIEWHEIGHT)*320+VIEWX-1]=124;
    return VWB_DrawPic(0,160,STATUSBARPIC);
}
bool GameProbe(void)
{
    bool valid=SetupGameLevel() && DrawPlayScreen() && ThreeDRefresh();
    ShutdownGameLevel(); return valid;
}
bool GameStart(uint64_t ticks)
{
    GameStop();
    if (!SetupGameLevel() || !DrawPlayScreen() || !ThreeDRefresh()) { GameStop(); return false; }
    lasttimecount=ticks;running=true; IN_ClearKeysDown(); return true;
}
void GameStop(void) { running=false; ShutdownGameLevel(); }
static bool Down(unsigned scan) { return Keyboard[scan] || IN_PressPending(scan); }
bool GameStep(uint64_t ticks)
{
    if (!running || ticks<lasttimecount) return CA_Fail("Invalid simulation clock/state");
    uint64_t elapsed=ticks-lasttimecount;
    if (!elapsed) return true; /* Retain quick make/break input until a tic runs. */
    lasttimecount=ticks;
    if (elapsed>MAXTICS) elapsed=MAXTICS; /* Discard long-pause excess, as CalcTics. */
    for (uint64_t tic=0;tic<elapsed;++tic) {
        MoveDoors();
        if (IN_ConsumePress(sc_Space)) Cmd_Use();
        int move=(Down(0x2a) || Down(0x36)) ? 70 : 35;
        int cx=(Down(sc_RightArrow) ? move : 0)-(Down(sc_LeftArrow) ? move : 0);
        int cy=(Down(sc_DownArrow) ? move : 0)-(Down(sc_UpArrow) ? move : 0);
        ControlMovement(cx,cy,Down(0x38));
        IN_ClearPressed();
    }
    return ThreeDRefresh();
}
