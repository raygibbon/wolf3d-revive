/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_MAIN.C DemoLoop and WL_MENU.C control panel, resumed by monotonic tics.
   Title 15s -> credits 10s -> scores 10s; stop at menu instead of faking demos. */
#include "WL_MAIN.h"
#include "WL_MENU.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "WL_DEF.h"
#include "ID_SD.h"
#include <string.h>
static ui_state state, next_state;
static bool leaving;
static uint64_t deadline, transition_end, blink_next;
static int cursor_x, cursor_y;
static bool blink;
static unsigned episode,difficulty=2;
static bool Attract(void) { return state <= UI_HIGHSCORES; }
ui_state DemoLoopState(void) { return state; }

static bool GameError(void)
{
    char text[384]="Cannot continue this game:\n",error[256];
    (void)strncpy(error,CA_Error(),sizeof(error)-1);error[sizeof(error)-1]=0;
    size_t at=strlen(text);unsigned column=0;
    for (const char *p=error;*p && at<sizeof(text)-32;++p) {
        if (column>=34 && *p==' ') { text[at++]='\n';column=0; }
        else { text[at++]=*p;++column; }
    }
    (void)strcpy(text+at,"\nPress a key for menu.");
    GameStop();state=UI_ERROR;leaving=false;IN_ClearKeysDown();
    ClearMScreen();return Message(text);
}
static bool ChangeScreen(ui_state screen, uint64_t ticks)
{
    state = screen; leaving = false;
    IN_ClearKeysDown();
    if (screen == UI_DONE) return true;
    if (screen == UI_GAME && !GameStartSelected(ticks+10,episode,difficulty)) return GameError() && VL_FadeIn(gamepal,10,ticks);
    if (screen == UI_TITLE) SD_StartMusic(7);
    if (screen == UI_MENU) SD_StartMusic(14);
    if (screen == UI_SCORES || screen == UI_HIGHSCORES) SD_StartMusic(23);
    if (screen == UI_SOUND && !DrawSoundSettings()) return false;
    if (screen == UI_TITLE && !CA_CacheScreen(TITLEPIC)) return false;
    if (screen == UI_CREDITS && !CA_CacheScreen(CREDITSPIC)) return false;
    if ((screen == UI_HIGHSCORES || screen == UI_SCORES) && !DrawHighScores()) return false;
    if (screen == UI_MENU && !SetupControlPanel(ticks)) return false;
    if (screen == UI_EPISODE && !DrawNewSelection(true,episode)) return false;
    if (screen == UI_DIFFICULTY && !DrawNewSelection(false,difficulty)) return false;
    unsigned steps = Attract() ? 30 : 10;
    if (!VL_FadeIn(gamepal,steps,ticks)) return false;
    deadline = ticks + steps + (screen == UI_TITLE ? 15U*70U : 10U*70U);
    return true;
}
static bool Transition(ui_state next, uint64_t ticks, unsigned steps)
{
    leaving = true; next_state = next; transition_end = ticks + steps;
    IN_ClearKeysDown();
    /* MenuFadeOut is dark red, VW_FadeOut is black. */
    return VL_FadeOut(Attract() ? 0 : 43,0,0,steps,ticks);
}
bool DemoLoopStartGame(uint64_t ticks) { episode=0;difficulty=2;return ChangeScreen(UI_GAME,ticks); }
bool DemoLoopStartup(uint64_t ticks, bool menu)
{
    GameStop();
    /* Validate the complete UI slice before starting the event loop. */
    if (!VW_ValidateFont(0) || !VW_ValidateFont(1) || !DrawHighScores() ||
        !SetupControlPanel(ticks) || !CA_CacheScreen(CREDITSPIC) || !CA_CacheScreen(TITLEPIC)) return false;
    if (!VL_FadeOut(0,0,0,0,ticks)) return false;
    return ChangeScreen(menu ? UI_MENU : UI_TITLE,ticks);
}
bool DemoLoop(uint64_t ticks)
{
    /* Catch up to scheduled screen deadlines; never derive time from frames. */
    for (unsigned transitions = 0; transitions < 16; ++transitions) {
        VL_FadeStep(ticks);
        if (leaving) {
            if (VL_Fading()) return true;
            if (!ChangeScreen(next_state,transition_end)) return false;
            continue;
        }
        if (Attract() && IN_AnyPressed()) return Transition(UI_MENU,ticks,30);
        if (Attract() && ticks >= deadline) {
            ui_state next = state == UI_TITLE ? UI_CREDITS : state == UI_CREDITS ? UI_HIGHSCORES : UI_MENU;
            if (!Transition(next,deadline,30)) return false;
            continue;
        }
        break;
    }
    if (VL_Fading() || Attract()) return true;
    if (state == UI_GAME) {
        if (!GameVictoryPresentation() && IN_ConsumePress(sc_Escape)) { GameStop(); return Transition(UI_MENU,ticks,10); }
        if (!GameStep(ticks)) return GameError();
        if (GameFinished()) { GameStop(); return Transition(UI_SCORES,ticks,10); }
        return true;
    }
    if (state == UI_MENU) {
        int action = HandleMenu(ticks);
        if (action == -3) return false;
        if (action == soundmenu) return Transition(UI_SOUND,ticks,10);
        if (action == newgame) return Transition(UI_EPISODE,ticks,10);
        if (action == viewscores) return Transition(UI_SCORES,ticks,10);
        if (action == backtodemo) return Transition(UI_TITLE,ticks,10);
        if (action == quit || action == -1) {
            state = UI_CONFIRM; IN_ClearKeysDown();
            /* One original quit phrase, until the original RNG is ported. */
            if (!Message("Dost thou wish to\nleave with such hasty\nabandon?")) return false;
            cursor_x = PrintX; cursor_y = PrintY; blink = false; blink_next = ticks + 10;
        }
    } else if (state == UI_SOUND) {
        int action=HandleSoundSettings();
        if(action==-3) return false;
        if(action==-1) return Transition(UI_MENU,ticks,10);
    } else if (state == UI_EPISODE || state == UI_DIFFICULTY) {
        bool choosing_episode=state==UI_EPISODE;
        int action=HandleNewSelection(ticks,choosing_episode,choosing_episode ? &episode : &difficulty);
        if (action==-3) return false;
        if (action==-1) return Transition(choosing_episode ? UI_MENU : UI_EPISODE,ticks,10);
        if (action==1) return Transition(choosing_episode ? UI_DIFFICULTY : UI_GAME,ticks,10);
    } else if (state == UI_ERROR && IN_AnyPressed()) {
        return Transition(UI_MENU,ticks,10);
    } else if (state == UI_SCORES && IN_AnyPressed()) {
        return Transition(UI_MENU,ticks,10);
    } else if (state == UI_CONFIRM) {
        if (IN_ConsumePress(sc_Y)) return Transition(UI_DONE,ticks,10);
        else if (IN_ConsumePress(sc_N) || IN_ConsumePress(sc_Escape)) {
            state = UI_MENU; IN_ClearKeysDown();
            if (!SetupControlPanel(ticks)) return false;
        } else if (ticks >= blink_next) {
            uint64_t count = (ticks - blink_next) / 10 + 1;
            blink_next += count * 10;
            if (count % 2) blink = !blink;
            VWB_Bar(cursor_x,cursor_y,8,13,TEXTCOLOR);
            if (blink) { PrintX = cursor_x; PrintY = cursor_y; fontcolor = 0; if (!US_Print("_")) return false; }
        }
    }
    IN_ClearPressed();
    return true;
}
