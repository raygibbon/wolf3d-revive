/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_MAIN.C DemoLoop and WL_MENU.C control panel, resumed by monotonic tics.
   Title 15s -> credits 10s -> scores 10s -> validated original demo. */
#include "WL_MAIN.h"
#include "WL_MENU.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "WL_DEF.h"
#include "WL_SAVE.h"
#include "ID_SD.h"
#include "WL_CONTROLS.h"
#include "WL_DISPLAY.h"
#include "WL_DEMO.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
static ui_state state, next_state, signon_return;
static bool leaving;
static uint64_t deadline, transition_end, blink_next;
static int cursor_x, cursor_y;
static bool blink,resuming;
static unsigned episode,difficulty=2,demo_number;
static ui_state panel_return=UI_MENU,display_return=UI_CONTROLS,quick_return;
static bool boss_music,boss_effects,choosequick;
static int quickslot=-1;
static enum { QUICK_END,QUICK_LOAD,QUICK_QUIT,QUICK_ERROR } quick_action;
static bool Attract(void) { return state <= UI_HIGHSCORES || state==UI_DEMO || state==UI_RATING; }
ui_state DemoLoopState(void) { return state; }
void DemoLoopPrepareExit(void) { if(state==UI_BOSS) SD_SetEnabled(boss_music,boss_effects); }

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
    ui_state previous=state;
    state = screen; leaving = false;
    IN_ClearKeysDown();
    if (screen == UI_DONE) return true;
    if(screen==UI_DEMO) {
        bool started=false;
        for(unsigned n=0;n<4 && !started;++n) started=Demo_Start(demo_number++%4,ticks+30);
        if(!started) { GameStop();return ChangeScreen(UI_TITLE,ticks); }
    }
    if(screen==UI_VIEW && !BeginViewMenu()) return false;
    if(screen==UI_BOSS) {
        unsigned music,effects;SD_GetSettings(&music,&effects,&boss_music,&boss_effects);
        SD_SetEnabled(false,boss_effects);VWB_Bar(0,0,320,200,0);
        fontnumber=0;fontcolor=15;PrintX=WindowX=PrintY=0;if(!US_Print("C>")) return false;
    }
    if (screen == UI_GAME) {
        bool ok;
        if(resuming) { GamePauseClock(ticks+10);ok=GameDrawPaused();resuming=false;
            sd_status audio;SD_GetStatus(&audio);int song=SD_LevelMusic((unsigned)gamestate.episode,(unsigned)gamestate.mapon);
            if(audio.music!=song) SD_StartMusic(song); }
        else ok=GameStartSelected(ticks+10,episode,difficulty);
        if(!ok) return GameError() && VL_FadeIn(gamepal,10,ticks);
    }
    if ((screen==UI_SAVE || screen==UI_LOAD) && !BeginSaveMenu(screen==UI_SAVE)) return false;
    if (screen == UI_TITLE || (screen==UI_RATING && previous==UI_SIGNON)) SD_StartMusic(7);
    if (screen == UI_MENU) SD_StartMusic(14);
    if (screen == UI_SCORES || screen == UI_HIGHSCORES) SD_StartMusic(23);
    if(screen==UI_CONTROLS || screen==UI_SOUND) ResetMenuCursor(ticks);
    if(screen==UI_DISPLAY && !BeginDisplayMenu()) return false;
    if(screen==UI_CONTROLS && !(previous==UI_DISPLAY?ResumeControlsMenu():BeginControlsMenu())) return false;
    if (screen == UI_SOUND && !BeginSoundSettings()) return false;
    if (screen == UI_TITLE && !CA_CacheScreen(TITLEPIC)) return false;
    if(screen==UI_RATING) {
        VWB_Bar(0,0,320,200,0x82);
        if(!VWB_DrawPic(216,110,PG13PIC)) return false;
    }
    if (screen == UI_CREDITS && !CA_CacheScreen(CREDITSPIC)) return false;
    if ((screen == UI_HIGHSCORES || screen == UI_SCORES) && !DrawHighScores()) return false;
    if (screen == UI_MENU && !SetupControlPanel(ticks)) return false;
    if (screen == UI_EPISODE && !DrawNewSelection(true,episode)) return false;
    if (screen == UI_DIFFICULTY && !DrawNewSelection(false,difficulty)) return false;
    unsigned steps = Attract() ? 30 : 10;
    if (!VL_FadeIn(gamepal,steps,ticks)) return false;
    deadline = ticks + steps + (screen==UI_RATING?7U*70U:screen == UI_TITLE ? 15U*70U : 10U*70U);
    return true;
}
static bool Transition(ui_state next, uint64_t ticks, unsigned steps)
{
    leaving = true; next_state = next; transition_end = ticks + steps;
    IN_ClearKeysDown();
    /* MenuFadeOut is dark red, VW_FadeOut is black. */
    return VL_FadeOut(Attract() || state==UI_SIGNON ? 0 : 43,0,0,steps,ticks);
}
bool DemoLoopStartGame(uint64_t ticks) { quickslot=-1;episode=0;difficulty=2;resuming=false;return ChangeScreen(UI_GAME,ticks); }
bool DemoLoopStartup(uint64_t ticks, bool menu)
{
    GameStop();resuming=false;quickslot=-1;demo_number=0;panel_return=UI_MENU;choosequick=false;
    /* Validate the complete UI slice before starting the event loop. */
    if (!VW_ValidateFont(0) || !VW_ValidateFont(1) || !DrawHighScores() ||
        !SetupControlPanel(ticks) || !CA_CacheScreen(CREDITSPIC) || !CA_CacheScreen(TITLEPIC)) return false;
    if (!VL_FadeOut(0,0,0,0,ticks)) return false;
    return ChangeScreen(menu ? UI_MENU : UI_TITLE,ticks);
}
/* FinishSignon uses the normal font, centred at y=190. Leave the original
   memory/card indicators unfilled: DOS resources are not modern measurements. */
static bool SignonPrompt(const char *text,w3r_byte color)
{
    int old_font=fontnumber,old_x=px,old_y=py,width,height;
    w3r_byte old_color=fontcolor;
    fontnumber=0;fontcolor=color;
    bool ok=VW_MeasurePropString(text,&width,&height);
    if(ok) {
        VWB_Bar(0,189,300,11,framebuffer[0]);
        px=(320-width)/2;py=190;ok=VW_DrawPropString(text);
    }
    fontnumber=old_font;fontcolor=old_color;px=old_x;py=old_y;return ok;
}
bool DemoLoopSignon(const char *path,uint64_t ticks)
{
    /* Original DOS introscn was linked into the executable, not WL6.
       A user-supplied 320x200 raw indexed image can restore its appearance.
       Never synthesize DOS hardware detection data. */
    if(!path || GameRunning() || (state!=UI_TITLE && state!=UI_RATING)) return true;
    FILE *file=fopen(path,"rb");
    if(!file) return errno==ENOENT ? true : CA_Fail("Cannot open optional SIGNON.RAW");
    unsigned char pixels[320U*200U];
    size_t count=fread(pixels,1,sizeof(pixels),file);
    int extra=fgetc(file),read_error=ferror(file);
    if(fclose(file)!=0) read_error=1;
    if(read_error || count!=sizeof(pixels) || extra!=EOF)
        return CA_Fail("Sign-on image must contain exactly 64000 indexed pixels");
    signon_return=state;
    memcpy(framebuffer,pixels,sizeof(pixels));
    if(!SignonPrompt("Press a key",14)) return false;
    state=UI_SIGNON;leaving=false;IN_ClearKeysDown();
    SD_StartMusic(-1);
    return VL_FadeIn(gamepal,10,ticks);
}
bool DemoLoopIntro(uint64_t ticks)
{
    /* WL_INTER.C PG13: the sign-on image linked into DOS is a different asset.
       Never fabricate it or mistake a setup-program screen for archive art. */
    if(!CA_GrChunkPresent(PG13PIC) || pictable[PG13PIC-STARTPICS].width!=88 ||
       pictable[PG13PIC-STARTPICS].height!=64) return true;
    if(state!=UI_TITLE || GameRunning()) return true;
    return ChangeScreen(UI_RATING,ticks);
}
static bool ReturnPanel(uint64_t ticks)
{
    resuming=panel_return==UI_GAME;return Transition(panel_return,ticks,10);
}
static bool QuickDialog(int action,uint64_t ticks)
{
    quick_action=action;if(state!=UI_QUICK) quick_return=state;FinishPaletteShifts();
    if(GameRunning() && !GameDrawPaused()) return false;
    state=UI_QUICK;IN_ClearKeysDown();
    const char *text=action==QUICK_END?"Are you sure you want\nto end the game? (Y/N)":
        action==QUICK_QUIT?"Dost thou wish to\nleave with such hasty\nabandon? (Y/N)":
        action==QUICK_ERROR?"Quick operation failed.\nPress a key to return.":NULL;
    if(!text) {
        save_info info;Save_ReadInfo((unsigned)quickslot,&info);
        char message[128];(void)snprintf(message,sizeof(message),"Load Game called\n\"%s\"? (Y/N)",info.description);
        return Message(message);
    }
    (void)ticks;return Message(text);
}
static bool QuickSave(uint64_t ticks)
{
    save_info info;if(quickslot>=0) { quickslot=(int)SaveMenuSelected();Save_ReadInfo((unsigned)quickslot,&info); }
    if(quickslot<0 || info.status!=SAVE_VALID) {
        quickslot=-1;choosequick=true;panel_return=UI_GAME;return Transition(UI_SAVE,ticks,10);
    }
    IN_ClearKeysDown();GamePauseClock(ticks);
    if(!Save_Write((unsigned)quickslot,info.description,true)) return QuickDialog(QUICK_ERROR,ticks);
    return true;
}
static bool StepDemoLoop(uint64_t ticks)
{
    /* Catch up to scheduled screen deadlines; never derive time from frames. */
    for (unsigned transitions = 0; transitions < 16; ++transitions) {
        VL_FadeStep(ticks);
        if (leaving) {
            if (VL_Fading()) return true;
            if (!ChangeScreen(next_state,transition_end)) return false;
            continue;
        }
        if(state==UI_SIGNON) {
            if(IN_AnyPressed()) {
                if(!SignonPrompt("Working...",10)) return false;
                if(!Transition(signon_return,ticks,10)) return false;
                continue;
            }
            break;
        }
        if(state==UI_RATING && (IN_AnyPressed() || ticks>=deadline)) {
            if(!Transition(UI_TITLE,ticks>=deadline?deadline:ticks,30)) return false;
            continue;
        }
        if (Attract() && IN_AnyPressed()) { if(state==UI_DEMO) GameStop();return Transition(UI_MENU,ticks,30); }
        if (Attract() && state!=UI_DEMO && ticks >= deadline) {
            ui_state next = state == UI_TITLE ? UI_CREDITS : state == UI_CREDITS ? UI_HIGHSCORES : UI_DEMO;
            if (!Transition(next,deadline,30)) return false;
            continue;
        }
        break;
    }
    if(VL_Fading()) return true;
    if(state==UI_DEMO) {
        if(!GameStep(ticks) || Demo_Finished()) { GameStop();return Transition(UI_TITLE,ticks,30); }
        return true;
    }
    if(state==UI_SIGNON || Attract()) return true;
    if((state==UI_CONTROLS || state==UI_SOUND) && TickMenuCursor(ticks)) {
        if(!(state==UI_CONTROLS?DrawControlsMenu():DrawSoundSettings())) return false;
    }
    if (state == UI_GAME) {
        /* DOS US_ControlPanel/CP_CheckQuick: panel entry freezes gameplay,
           direct F-key panels return to the game, not the main menu. */
        if(GameCanSave()) {
            panel_return=UI_GAME;choosequick=false;
            if(IN_ConsumePress(0x3b)) { FinishPaletteShifts();return Transition(UI_BOSS,ticks,10); }
            if(IN_ConsumePress(0x3c) && *Save_Directory()) { FinishPaletteShifts();return Transition(UI_SAVE,ticks,10); }
            if(IN_ConsumePress(0x3d) && *Save_Directory()) { FinishPaletteShifts();return Transition(UI_LOAD,ticks,10); }
            if(IN_ConsumePress(0x3e)) { FinishPaletteShifts();return Transition(UI_SOUND,ticks,10); }
            if(IN_ConsumePress(0x3f)) { FinishPaletteShifts();return Transition(UI_VIEW,ticks,10); }
            if(IN_ConsumePress(0x40)) { FinishPaletteShifts();return Transition(UI_CONTROLS,ticks,10); }
            if(IN_ConsumePress(0x41)) return QuickDialog(QUICK_END,ticks);
            if(IN_ConsumePress(0x42) && *Save_Directory()) return QuickSave(ticks);
            if(IN_ConsumePress(0x43) && *Save_Directory()) {
                save_info info;if(quickslot>=0) { quickslot=(int)SaveMenuSelected();Save_ReadInfo((unsigned)quickslot,&info); }
                if(quickslot>=0 && info.status==SAVE_VALID) return QuickDialog(QUICK_LOAD,ticks);
                quickslot=-1;choosequick=true;return Transition(UI_LOAD,ticks,10);
            }
            if(IN_ConsumePress(0x44)) return QuickDialog(QUICK_QUIT,ticks);
        }
        if (GameCanSave() && (CT_Pressed(CT_MENU) || IN_ConsumePress(sc_Escape))) { FinishPaletteShifts();return Transition(UI_MENU,ticks,10); }
        if (!GameStep(ticks)) return GameError();
        if (GameFinished()) { GameStop(); return Transition(UI_SCORES,ticks,10); }
        return true;
    }
    if (state == UI_MENU) {
        panel_return=UI_MENU;choosequick=false;
        if(IN_ConsumePress(0x3b)) return Transition(UI_BOSS,ticks,10);
        int action = HandleMenu(ticks);
        if (action == -3) return false;
        if (action == savegame || action == loadgame) return Transition(action==savegame?UI_SAVE:UI_LOAD,ticks,10);
        if (action == changeview) return Transition(UI_VIEW,ticks,10);
        if (action == control) return Transition(UI_CONTROLS,ticks,10);
        if (action == soundmenu) return Transition(UI_SOUND,ticks,10);
        if (action == newgame) { quickslot=-1;return Transition(UI_EPISODE,ticks,10); }
        if (action == viewscores) return GameRunning()?QuickDialog(QUICK_END,ticks):Transition(UI_SCORES,ticks,10);
        if (action == backtodemo) { resuming=GameRunning();return Transition(resuming?UI_GAME:UI_TITLE,ticks,10); }
        if (action == quit || action == -1) {
            state = UI_CONFIRM; IN_ClearKeysDown();
            /* Keep dialog choice outside gameplay RNG. */
            if (!Message("Dost thou wish to\nleave with such hasty\nabandon?")) return false;
            cursor_x = PrintX; cursor_y = PrintY; blink = false; blink_next = ticks + 10;
        }
    } else if (state==UI_SAVE || state==UI_LOAD) {
        int action=HandleSaveMenu(ticks);
        if(action==-3) return false;
        if(action==-1 || action==1) {
            bool loaded=action==1 && state==UI_LOAD;
            if(action==1 && choosequick) quickslot=(int)SaveMenuSelected();
            choosequick=false;
            if(loaded) { resuming=true;return Transition(UI_GAME,ticks,10); }
            return ReturnPanel(ticks);
        }
    } else if(state==UI_DISPLAY) {
        int action=HandleDisplayMenu();if(action==-3) return false;
        if(action==-1) return Transition(display_return,ticks,10);
    } else if(state==UI_CONTROLS) {
        int action=HandleControlsMenu();if(action==-3) return false;
        if(action==2) { display_return=UI_CONTROLS;return Transition(UI_DISPLAY,ticks,10); }
        if(action==-1) return ReturnPanel(ticks);
    } else if(state==UI_VIEW) {
        int action=HandleViewMenu(ticks);if(action==-3) return false;
        if(action==-1 || action==1) return ReturnPanel(ticks);
    } else if(state==UI_BOSS) {
        if(IN_ConsumePress(sc_Escape)) { SD_SetEnabled(boss_music,boss_effects);return ReturnPanel(ticks); }
    } else if(state==UI_QUICK) {
        bool yes=IN_ConsumePress(sc_Y),no=IN_ConsumePress(sc_N)|IN_ConsumePress(sc_Escape);
        if(yes || no || (quick_action==QUICK_ERROR && IN_AnyPressed())) {
            if(yes && quick_action==QUICK_QUIT) return Transition(UI_DONE,ticks,10);
            if(yes && quick_action==QUICK_END) { GameStop();gamestate.lives=0;quickslot=-1;return Transition(UI_HIGHSCORES,ticks,10); }
            if(yes && quick_action==QUICK_LOAD && !Save_Load((unsigned)quickslot,ticks)) return QuickDialog(QUICK_ERROR,ticks);
            resuming=quick_return==UI_GAME;return Transition(quick_return,ticks,10);
        }
    } else if (state == UI_SOUND) {
        int action=HandleSoundSettings();
        if(action==-3) return false;
        if(action==-1) return ReturnPanel(ticks);
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
    IN_ClearPressed();CT_EndTic();
    return true;
}

bool DemoLoop(uint64_t ticks)
{
    bool ok=StepDemoLoop(ticks);
    CT_SetGameplay(ok && state==UI_GAME && GameCanSave() && !VL_Fading());
    return ok;
}
