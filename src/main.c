/* SPDX-License-Identifier: GPL-2.0-only */
/* Entry point for the original engine UI/attract slice. */
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include "platform/sdl_audio.h"
#include "platform/config_file.h"
#include "engine/ID_SD.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_DEF.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_DISPLAY.h"
#include "engine/WL_FRAME_CLOCK.h"
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_filesystem.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(const char *name)
{
    printf("Usage: %s --data /path/to/WL6 [--check-assets|--check-ui|--check-level] [--menu|--level] [--no-audio] [--frames N] [--capture local.bmp]\n"
           "Original WL6 campaign with native AdLib music and digitized audio. New Game selects episode and difficulty.\n"
           "--check-assets/--check-ui/--check-level validate without a window.\n"
           "--level starts E1M1 directly; arrows move/turn, Alt strafes, Shift runs.\n"
           "Ctrl fires; 1-4 select weapons; Space uses doors, pushwalls and elevators; Escape returns to menu. --frames limits a smoke test.\n",
           name);
}

int main(int argc, char *argv[])
{
    const char *directory = NULL, *capture = NULL;
    bool no_audio=false,config_writable=false;
    char *save_path=NULL;
    bool check_only = false, check_ui = false, check_level = false, menu = false, level = false;
    unsigned long frames = 0;
    int status = 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-audio") == 0) { no_audio=true;continue; }
        if (strcmp(argv[i], "--help") == 0) { Usage(argv[0]); return 0; }
        if (strcmp(argv[i], "--check-level") == 0) { check_level = true; continue; }
        if (strcmp(argv[i], "--level") == 0) { level = true; continue; }
        if (strcmp(argv[i], "--menu") == 0) { menu = true; continue; }
        if (strcmp(argv[i], "--check-ui") == 0) { check_ui = true; continue; }
        if (strcmp(argv[i], "--check-assets") == 0) { check_only = true; continue; }
        if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) { directory = argv[++i]; continue; }
        if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) { capture = argv[++i]; continue; }
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            char *end;
            const char *value = argv[++i];
            errno = 0;
            frames = strtoul(value, &end, 10);
            if (!errno && value[0] >= '0' && value[0] <= '9' && !*end && frames > 0 && frames <= 100000)
                continue;
        }
        fprintf(stderr, "Invalid/incomplete option: %s\n", argv[i]);
        Usage(argv[0]);
        return 1;
    }
    if (!directory || ((unsigned)check_only+(unsigned)check_ui+(unsigned)check_level>1) || (menu && level) || ((check_only || check_ui || check_level) && capture)) {
        fputs("Supply --data; choose one check mode; --capture requires a window.\n", stderr);
        Usage(argv[0]);
        return 1;
    }
    MM_Startup();
    if (!CA_Startup(directory) || !VL_SetPalette(gamepal, sizeof(gamepal)) || !CA_CacheScreen(TITLEPIC)) {
        fprintf(stderr, "Cannot load WL6 title: %s\n", CA_Error());
        goto done;
    }
    puts("Loaded original WL6 TITLEPIC: 320x200 indexed pixels, 256-color VGA palette.");
    if (check_only) { status = 0; goto done; }
    if (check_ui) {
        if (!DemoLoopStartup(0,menu)) { fprintf(stderr,"Cannot load original UI: %s\n",CA_Error()); goto done; }
        puts("Original fonts, menu pictures, credits and high scores validated.");
        status = 0; goto done;
    }
    SD_SetDigitizedMode(true); /* Logical mode also applies with --no-audio. */
    bool ready=GameProbe();
    if (check_level) {
        if (!ready) { fprintf(stderr,"Cannot load E1M1: %s\n",CA_Error()); goto done; }
        puts("E1M1 maps, textures, sprites, objects, actors, HUD and original renderer validated.");
        status=0; goto done;
    }
    MainMenu[newgame].active=ready ? 1 : 0;
    if (ready) MainItems.curpos=newgame;
    else { fprintf(stderr,"New Game unavailable: %s\n",CA_Error()); if (level) goto done; }
    if (!VL_Startup()) { fprintf(stderr, "SDL3 video: %s\n", VL_Error()); goto video_done; }
    if (!no_audio && !Audio_Startup()) fprintf(stderr,"Audio unavailable: %s; continuing silently.\n",Audio_Error());
    Display_Connect(VL_DisplayGet,VL_DisplaySet);
    MainMenu[soundmenu].active=1;
    save_path=SDL_GetPrefPath(NULL,"wolf3d-revive");
    if(save_path) {
        (void)Save_SetDirectory(save_path);config_result config=Config_Load(save_path);
        config_writable=config==CONFIG_MISSING || config==CONFIG_LOADED;
        if(*Config_Error()) fprintf(stderr,"Configuration: %s\n",Config_Error());
    }
    else fputs("Save storage unavailable; gameplay remains available.\n",stderr);
    uint64_t ticks = VL_GetTicks70();
    if (!DemoLoopStartup(ticks,menu)) { fprintf(stderr,"Cannot load original UI: %s\n",CA_Error()); goto video_done; }
    if(!menu && !level && !DemoLoopIntro(ticks)) { fprintf(stderr,"Startup artwork: %s\n",CA_Error());goto video_done; }
    if(!menu && !level) {
        char signon_path[4096];
        int n=snprintf(signon_path,sizeof(signon_path),"%s/SIGNON.RAW",directory);
        if(n<0 || (size_t)n>=sizeof(signon_path) || !DemoLoopSignon(signon_path,ticks)) {
            fprintf(stderr,"Sign-on artwork: %s\n",CA_Error());goto video_done;
        }
    }
    if (level && !DemoLoopStartGame(ticks)) { fprintf(stderr,"E1M1: %s\n",CA_Error()); goto video_done; }
    if (capture) {
        w3r_byte saved[768]; memcpy(saved,palette,sizeof(saved));
        (void)VL_SetPalette(gamepal,sizeof(gamepal));
        bool captured = VL_Capture(capture);
        (void)VL_SetPalette(saved,sizeof(saved));
        if (!captured) { fprintf(stderr,"Cannot capture screen: %s\n",VL_Error()); goto video_done; }
    }
    uint64_t completed_tick=ticks,next_frame=0;
    bool scheduled_before=false;double previous_refresh=0.0;unsigned long frame=0;
    while(!VL_PollQuit() && DemoLoopState()!=UI_DONE) {
        display_settings display={0};bool scheduled=VL_DisplayGet(&display) && display.high_resolution && display.smooth;
        uint64_t now=SDL_GetTicksNS();
        double refresh=scheduled?VL_MonitorRefreshRate():60.0;
        if(scheduled!=scheduled_before || refresh!=previous_refresh) {
            next_frame=0;scheduled_before=scheduled;previous_refresh=refresh;
        }
        /* Simulation uses rational 70 Hz deadlines in every display mode.
           A bounded batch retains debt; only presentation frames may be missed. */
        for(unsigned steps=0;steps<8 && WL_TickNext(&completed_tick,now);++steps) {
            if(!DemoLoop(completed_tick)) { fprintf(stderr,"Original UI: %s\n",CA_Error());goto video_done; }
        }
        Audio_Pump();
        now=SDL_GetTicksNS();
        if(!scheduled || now>=next_frame) {
            if(!VW_UpdateScreen()) { fprintf(stderr,"SDL3 presentation: %s\n",VL_Error());goto video_done; }
            ++frame;if(frames && frame>=frames) break;
            next_frame=WL_PresentationDeadline(now,SDL_GetTicksNS(),refresh,VL_VSyncPaced(refresh));
        }
        if(!scheduled) VL_Sleep(4);
        else {
            now=SDL_GetTicksNS();uint64_t wake=WL_TickNS(completed_tick+1);
            if(next_frame<wake) wake=next_frame;
            if(wake>now) SDL_DelayNS(wake-now>4000000?4000000:wake-now);
        }
    }
    status = 0;
video_done:
    DemoLoopPrepareExit();
    if(save_path) {
        if(status==0 && config_writable && !Config_Save(save_path)) fprintf(stderr,"Configuration: %s\n",Config_Error());
        SDL_free(save_path);
    }
    Display_Connect(NULL,NULL);
    Audio_Shutdown();
    VL_Shutdown();
done:
    GameStop();
    CA_Shutdown();
    MM_Shutdown();
    return status;
}
