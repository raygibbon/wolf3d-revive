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
#include "WL_HIGHSCORES.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
static ui_state state, next_state, signon_return;
static bool leaving;
static bool startup_mouse,startup_synth,startup_digi,allow_debug,debug_unlocked,godmode;
static unsigned jukebox_bank,jukebox_choice;
static char warp_input[3];
static int jukebox_playing=-1;
void DemoLoopStartupDevices(bool mouse,bool synthesis,bool digitized)
{ startup_mouse=mouse;startup_synth=synthesis;startup_digi=digitized; }
void DemoLoopAllowDebug(bool allow) { allow_debug=allow;debug_unlocked=false; }
bool GameGodMode(void) { return godmode && !Demo_Active(); }
static int score_rank=-1;
static bool score_error;
static uint64_t deadline, transition_end;
static bool resuming;
static uint32_t ui_random=0x574f4c46U;
static const char *QuitMessage(uint64_t ticks)
{
    ui_random^=(uint32_t)ticks;ui_random=ui_random*1664525U+1013904223U;
    unsigned first=ui_random>>24;ui_random=ui_random*1664525U+1013904223U;
    return MenuQuitMessage(first,ui_random>>24);
}
static unsigned episode,difficulty=2,demo_number;
static ui_state panel_return=UI_MENU,display_return=UI_CONTROLS,quick_return;
static bool boss_music,boss_effects,choosequick,pause_previous;
static int quickslot=-1;
static bool disk_saving,disk_done;
static uint64_t disk_at;
static char disk_name[SAVE_DESCRIPTION];
static enum { QUICK_END,QUICK_LOAD,QUICK_QUIT,QUICK_NEW,QUICK_ERROR } quick_action;
static bool Attract(void) { return state <= UI_HIGHSCORES || state==UI_DEMO || state==UI_RATING; }
ui_state DemoLoopState(void) { return state; }
void DemoLoopPrepareExit(void) { if(state==UI_BOSS) SD_SetEnabled(boss_music,boss_effects);if(state==UI_PAUSE) SD_SetMusicPaused(pause_previous); }

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
        if(GamePreloadPending()) return StartPreloadGraphics(ticks) || GameError();
    }
    if ((screen==UI_SAVE || screen==UI_LOAD) && !BeginSaveMenu(screen==UI_SAVE)) return false;
    if (screen == UI_TITLE || (screen==UI_RATING && previous==UI_SIGNON)) SD_StartMusic(7);
    if (screen == UI_MENU) SD_StartMusic(14);
    if (screen == UI_SCORES || screen == UI_HIGHSCORES || screen==UI_RESULTS) SD_StartMusic(23);
    if(screen==UI_RESULTS) {
        score_error=false;
        if(score_rank>=0) { if(!BeginHighScoreEntry(score_rank,ticks+30)) return false; }
        else if(!DrawHighScores()) return false;
    }
    if(screen==UI_CONTROLS || screen==UI_SOUND || screen==UI_SAVE || screen==UI_LOAD) ResetMenuCursor(ticks);
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
    unsigned steps = Attract() || screen==UI_RESULTS ? 30 : 10;
    if (!VL_FadeIn(gamepal,steps,ticks)) return false;
    deadline = ticks + steps + (screen==UI_RATING?7U*70U:screen == UI_TITLE ? 15U*70U : 10U*70U);
    if(screen==UI_RESULTS) deadline=ticks+30+500;
    return true;
}
static bool Transition(ui_state next, uint64_t ticks, unsigned steps)
{
    leaving = true; next_state = next; transition_end = ticks + steps;
    IN_ClearKeysDown();
    /* CP_Quit/CP_CheckQuick silence both streams before MenuFadeOut. */
    if(next==UI_DONE) { SD_StartMusic(-1);SD_StopSound(); }
    /* MenuFadeOut is dark red, VW_FadeOut is black. */
    return VL_FadeOut(Attract() || state==UI_SIGNON || state==UI_RESULTS || next==UI_RESULTS ? 0 : 43,0,0,steps,ticks);
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
    /* WL_MENU.C IntroScreen hardware boxes: fill supported virtual devices
       at the original coordinates. No conventional/EMS/XMS inventory exists. */
    if(startup_mouse) VWB_Bar(164,82,12,2,14);
    if(startup_synth && !startup_digi) VWB_Bar(164,128,12,2,14);
    if(startup_digi) VWB_Bar(164,151,12,2,14);
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
    if(GameRunning() && state==UI_GAME && !GameDrawPaused()) return false;
    state=UI_QUICK;IN_ClearKeysDown();
    const char *text=action==QUICK_END?"Are you sure you want\nto end the game you\nare playing? (Y or N):":
        action==QUICK_QUIT?QuitMessage(ticks):
        action==QUICK_NEW?"You are currently in\na game. Continuing will\nerase old game. Ok?":
        action==QUICK_ERROR?"Quick operation failed.\nPress a key to return.":NULL;
    if(!text) {
        save_info info;Save_ReadInfo((unsigned)quickslot,&info);
        char message[128];(void)snprintf(message,sizeof(message),"Load Game called\n\"%s\"? (Y/N)",info.description);
        return BeginConfirmationAtHeight(message,ticks,quick_return==UI_GAME?160U:200U);
    }
    return action==QUICK_ERROR?MessageAtHeight(text,quick_return==UI_GAME?160U:200U):BeginConfirmationAtHeight(text,ticks,quick_return==UI_GAME?160U:200U);
}
static bool QuickActivity(bool saving,const char *name,uint64_t ticks)
{
    disk_saving=saving;disk_done=false;disk_at=ticks+1;
    (void)snprintf(disk_name,sizeof(disk_name),"%s",name?name:"");
    state=UI_DISK;IN_ClearKeysDown();return DrawSaveActivity(saving,false);
}
static bool QuickSave(uint64_t ticks)
{
    save_info info;if(quickslot>=0) { quickslot=(int)SaveMenuSelected();Save_ReadInfo((unsigned)quickslot,&info); }
    if(quickslot<0 || info.status!=SAVE_VALID) {
        quickslot=-1;choosequick=true;panel_return=UI_GAME;return Transition(UI_SAVE,ticks,10);
    }
    IN_ClearKeysDown();GamePauseClock(ticks);
    return QuickActivity(true,info.description,ticks);
}
static bool GameMessage(const char *text)
{
    FinishPaletteShifts();if(!GameDrawPaused() || !MessageAtHeight(text,160)) return false;
    state=UI_MESSAGE;IN_ClearKeysDown();return true;
}
/* WL_MAIN.C DoJukebox: registered WL6 has three time-selected six-song banks. */
static const unsigned jukebox_tracks[]={3,11,9,12,2,0,8,18,17,4,1,19,6,20,22,21,19,26};
static const char *const jukebox_names[]={
    "Get Them!","Searching","P.O.W.","Suspense","War March","Around The Corner!",
    "Nazi Anthem","Lurking...","Going After Hitler","Pounding Headache","Into the Dungeons","Ultimate Conquest",
    "Kill the S.O.B.","The Nazi Rap","Twelfth Hour","Zero Hour","Ultimate Conquest","Wolfpack"
};
static bool DrawJukebox(void)
{
    ClearMScreen();DrawStripes(10);DrawWindow(22,64,280,91,BKGDCOLOR);
    if(!VWB_DrawPic(112,184,C_MOUSELBACKPIC)) return false;
    fontnumber=1;fontcolor=0x47;int w,h;
    if(!VW_MeasurePropString("Robert's Jukebox",&w,&h)) return false;
    PrintX=(320-w)/2;PrintY=15;if(!US_Print("Robert's Jukebox")) return false;
    for(unsigned i=0;i<6;++i) {
        fontcolor=i==jukebox_choice?HIGHLIGHT:(int)i==jukebox_playing?0x4a:TEXTCOLOR;
        PrintX=56;PrintY=70+(int)i*13;if(!US_Print(jukebox_names[jukebox_bank*6+i])) return false;
    }
    return VWB_DrawPic(24,68+(int)jukebox_choice*13,MenuCursorPicture());
}
bool DemoLoopJukebox(uint64_t ticks,unsigned bank)
{
    if(GameRunning() || bank>2) return CA_Fail("Jukebox unavailable");
    jukebox_bank=bank;jukebox_choice=0;jukebox_playing=-1;
    state=UI_JUKEBOX;leaving=false;ResetMenuCursor(ticks);IN_ClearKeysDown();
    return DrawJukebox() && VL_FadeIn(gamepal,10,ticks);
}
static bool HiddenKeys(uint64_t ticks,bool *handled)
{
    *handled=true;
    if(Keyboard[0x32] && Keyboard[0x26] && Keyboard[0x17]) {
        /* WL_PLAY.C MLI: score reset and ten-minute time penalty are vital. */
        gamestate.health=100;gamestate.ammo=99;gamestate.keys=3;gamestate.score=0;
        gamestate.TimeCount+=42000U;GiveWeapon(wp_chaingun);
        return GameMessage("You now have 100% Health,\n99 Ammo and both Keys!\n\nNote that you have basically\neliminated your chances of\ngetting a high score!");
    }
    if(allow_debug && Keyboard[0x0e] && Keyboard[0x2a] && Keyboard[0x38]) {
        debug_unlocked=true;return GameMessage("Debugging keys are\nnow available!");
    }
    if(Keyboard[0x30] && Keyboard[0x1e] && Keyboard[0x14])
        return GameMessage("Commander Keen is also\navailable from Apogee, but\nthen, you already know\nthat - right, Cheatmeister?!");
    if(debug_unlocked && Keyboard[0x0f]) {
        char text[160];
        if(Keyboard[0x22]) { godmode=!godmode;return GameMessage(godmode?"God mode ON":"God mode OFF"); }
        if(Keyboard[0x23]) { IN_ClearKeysDown();TakeDamage(16,NULL);return true; }
        if(Keyboard[0x17]) {
            GivePoints(100000);HealSelf(99);if(gamestate.bestweapon<wp_chaingun) GiveWeapon(gamestate.bestweapon+1);
            GiveAmmo(50);return GameMessage("Free items!");
        }
        if(Keyboard[0x12]) { IN_ClearKeysDown();playstate=ex_completed;return true; }
        if(Keyboard[0x21]) {
            (void)snprintf(text,sizeof(text),"X:%ld\nY:%ld\nA:%d",(long)player.x,(long)player.y,player.angle);
            return GameMessage(text);
        }
        if(Keyboard[0x2e]) {
            unsigned active=0,inactive=0,used=0;
            for(objtype *ob=player.next;ob;ob=ob->next) { if(ob->active) ++active;else ++inactive; }
            for(unsigned i=0;i<statcount;++i) if(statobjlist[i].shapenum!=-1) ++used;
            (void)snprintf(text,sizeof(text),"Total statics :%u\nIn use statics:%u\nDoors         :%u\nTotal actors  :%u\nActive actors :%u",statcount,used,doornum,active+inactive,active);
            return GameMessage(text);
        }
        if(Keyboard[0x2d]) return GameMessage("Extra stuff!");
        if(Keyboard[0x11]) {
            warp_input[0]=0;
            if(!GameMessage("Warp to which level(1-10):")) return false;
            state=UI_WARP;return true;
        }
        if(Keyboard[0x19]) { state=UI_MESSAGE;IN_ClearKeysDown();return true; }
        if(Keyboard[0x10]) return Transition(UI_DONE,ticks,10);
    }
    *handled=false;return true;
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
            if(IN_PressPending(0x32)) return DemoLoopJukebox(ticks,(unsigned)((ticks/7U)%3U));
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
    if(state==UI_GAME && GameLoading()) return StepPreloadGraphics(ticks) || GameError();
    if(VL_Fading()) return true;
    if(state==UI_DEMO) {
        if(!GameStep(ticks) || Demo_Finished()) { GameStop();return Transition(UI_TITLE,ticks,30); }
        return true;
    }
    if(state==UI_SIGNON || Attract()) return true;
    if((state==UI_CONTROLS || state==UI_SOUND || state==UI_SAVE || state==UI_LOAD) && TickMenuCursor(ticks)) {
        if(state==UI_CONTROLS && !DrawControlsMenu()) return false;
        if(state==UI_SOUND && !DrawSoundSettings()) return false;
    }
    if((state==UI_MENU || state==UI_SOUND || state==UI_CONTROLS || state==UI_DISPLAY || state==UI_VIEW || state==UI_EPISODE || state==UI_DIFFICULTY) && IN_ConsumePress(sc_Pause))
        SD_SetMusicPaused(!SD_MusicPaused());
    if (state == UI_GAME) {
        if(GamePreloadPending()) return StartPreloadGraphics(ticks) || GameError();
        /* DOS US_ControlPanel/CP_CheckQuick: panel entry freezes gameplay,
           direct F-key panels return to the game, not the main menu. */
        if(GameCanSave()) {
            bool handled;if(!HiddenKeys(ticks,&handled)) return false;if(handled) return true;
        }
        if(GameCanSave() && IN_ConsumePress(sc_Pause)) {
            FinishPaletteShifts();
            if(!GameDrawPaused() || !VWB_DrawPic(128,64,PAUSEDPIC)) return false;
            pause_previous=SD_MusicPaused();SD_SetMusicPaused(true);
            IN_ClearKeysDown();state=UI_PAUSE;return true;
        }
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
        if(GamePreloadPending()) return StartPreloadGraphics(ticks) || GameError();
        if (GameFinished()) {
            score_rank=HighScores_Insert((uint32_t)gamestate.score,(unsigned)gamestate.mapon+1,(unsigned)gamestate.episode);
            GameStop();return Transition(UI_RESULTS,ticks,30);
        }
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
        if (action == newgame) return Transition(UI_EPISODE,ticks,10);
        if (action == viewscores) return GameRunning()?QuickDialog(QUICK_END,ticks):Transition(UI_SCORES,ticks,10);
        if (action == backtodemo) { resuming=GameRunning();return Transition(resuming?UI_GAME:UI_TITLE,ticks,10); }
        if (action == quit || action == -1) {
            state = UI_CONFIRM;
            return BeginConfirmation(QuitMessage(ticks),ticks);
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
        int answer=quick_action==QUICK_ERROR?(IN_AnyPressed()?-1:0):HandleConfirmation(ticks);
        if(answer==-3) return false;
        if(answer) {
            bool yes=answer>0;
            if(yes && quick_action==QUICK_QUIT) return Transition(UI_DONE,ticks,10);
            if(quick_action==QUICK_NEW) return Transition(yes?UI_DIFFICULTY:UI_MENU,ticks,10);
            if(yes && quick_action==QUICK_END) {
                /* CP_EndGame returns ex_died with zero lives to GameLoop. */
                gamestate.lives=0;gamestate.died=true;killerobj=NULL;
                quickslot=-1;resuming=true;return Transition(UI_GAME,ticks,10);
            }
            if(yes && quick_action==QUICK_LOAD) return QuickActivity(false,NULL,ticks);
            resuming=quick_return==UI_GAME;return Transition(quick_return,ticks,10);
        }
    } else if(state==UI_DISK) {
        if(ticks<disk_at) return true;
        if(disk_done) {
            IN_ClearKeysDown();GamePauseClock(ticks);state=UI_GAME;
            if(!GameDrawPaused()) return GameError();
            return !GamePreloadPending() || StartPreloadGraphics(ticks) || GameError();
        }
        if(!DrawSaveActivity(disk_saving,true)) return false;
        bool ok=disk_saving?Save_Write((unsigned)quickslot,disk_name,true):Save_Load((unsigned)quickslot,ticks);
        if(!ok) { state=UI_GAME;return QuickDialog(QUICK_ERROR,ticks); }
        /* DOS CP_LoadGame(1) clears loadedgame and resumes the existing
           PlayLoop; only an ordinary load re-enters GameLoop/PreloadGraphics. */
        if(!disk_saving) GamePreloadConsumed();
        disk_done=true;disk_at=ticks+1;IN_ClearKeysDown();
    } else if(state==UI_JUKEBOX) {
        bool redraw=TickMenuCursor(ticks);
        int hit=IN_MenuPointerRow(22,70,280,13,6);
        if(hit>=0 && (unsigned)hit!=jukebox_choice) {
            jukebox_choice=(unsigned)hit;MenuCursorMoved();redraw=true;(void)SD_PlaySound(MOVEGUN1SND,0,0);
        }
        int direction=MenuNavigation();
        if(direction) { jukebox_choice=(jukebox_choice+(direction<0?5U:1U))%6;MenuCursorMoved();redraw=true;(void)SD_PlaySound(MOVEGUN1SND,0,0); }
        if(IN_ConsumePress(sc_Enter) || IN_ConsumePress(sc_Space)) {
            jukebox_playing=(int)jukebox_choice;SD_StartMusic((int)jukebox_tracks[jukebox_bank*6+jukebox_choice]);
            (void)SD_PlaySound(SHOOTSND,0,0);redraw=true;
        }
        if(IN_ConsumePress(sc_Escape)) { (void)SD_PlaySound(ESCPRESSEDSND,0,0);return Transition(UI_TITLE,ticks,10); }
        if(redraw && !DrawJukebox()) return false;
    } else if(state==UI_WARP) {
        if(IN_ConsumePress(sc_Escape)) { IN_ClearKeysDown();GamePauseClock(ticks);state=UI_GAME;return GameDrawPaused(); }
        if(IN_ConsumePress(sc_Enter)) {
            unsigned floor=0;for(const char *p=warp_input;*p;++p) floor=floor*10U+(unsigned)(*p-'0');
            if(floor>=1 && floor<=10) { if(!GameWarp(ticks,floor)) return GameError(); }
            else { IN_ClearKeysDown();GamePauseClock(ticks); }
            state=UI_GAME;
            if(GamePreloadPending()) return StartPreloadGraphics(ticks) || GameError();
            return GameDrawPaused();
        }
        size_t n=strlen(warp_input);bool redraw=false;
        if(IN_ConsumePress(0x0e) && n) { warp_input[--n]=0;redraw=true; }
        char ch;while((ch=IN_NextASCII())!=0) if(ch>='0' && ch<='9' && n<2) { warp_input[n++]=ch;warp_input[n]=0;redraw=true; }
        if(redraw) {
            char prompt[80];(void)snprintf(prompt,sizeof(prompt),"Warp to which level(1-10):\n%s",warp_input);
            if(!GameDrawPaused() || !MessageAtHeight(prompt,160)) return false;
        }
    } else if(state==UI_MESSAGE) {
        if(IN_AnyPressed()) { IN_ClearKeysDown();GamePauseClock(ticks);state=UI_GAME;return GameDrawPaused(); }
    } else if(state==UI_PAUSE) {
        if(IN_AnyPressed()) {
            SD_SetMusicPaused(pause_previous);IN_ClearKeysDown();
            GamePauseClock(ticks);state=UI_GAME;return GameDrawPaused();
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
        if(action==1 && choosing_episode && GameRunning()) return QuickDialog(QUICK_NEW,ticks);
        if(action==1 && !choosing_episode) quickslot=-1;
        if (action==1) return Transition(choosing_episode ? UI_DIFFICULTY : UI_GAME,ticks,10);
    } else if(state==UI_RESULTS) {
        if(score_error) { if(IN_AnyPressed()) return Transition(UI_TITLE,ticks,30); }
        else if(score_rank>=0) {
            int result=HandleHighScoreEntry(ticks);if(result<0) return false;
            if(result) {
                score_rank=-1;
                if(!HighScores_Save()) {
                    score_error=true;IN_ClearKeysDown();
                    return Message("Cannot save high scores.\nRanking retained this session.\nPress a key to continue.");
                }
                return Transition(UI_TITLE,ticks,30);
            }
        } else if(IN_AnyPressed() || ticks>=deadline) return Transition(UI_TITLE,ticks,30);
    } else if (state == UI_ERROR && IN_AnyPressed()) {
        return Transition(UI_MENU,ticks,10);
    } else if (state == UI_SCORES && IN_AnyPressed()) {
        return Transition(UI_MENU,ticks,10);
    } else if (state == UI_CONFIRM) {
        int answer=HandleConfirmation(ticks);
        if(answer==-3) return false;
        if(answer>0) return Transition(UI_DONE,ticks,10);
        if(answer<0) {
            state=UI_MENU;
            if(!SetupControlPanel(ticks)) return false;
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
