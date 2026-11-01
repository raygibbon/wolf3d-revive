/* SPDX-License-Identifier: GPL-2.0-only */
/* Entry point for the original engine UI/attract slice. */
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_DEF.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(const char *name)
{
    printf("Usage: %s --data /path/to/WL6 [--check-assets|--check-ui|--check-level] [--menu|--level] [--frames N] [--capture local.bmp]\n"
           "Original WL6 menus and normal-level gameplay. New Game selects episode and difficulty.\n"
           "--check-assets/--check-ui/--check-level validate without a window.\n"
           "--level starts E1M1 directly; arrows move/turn, Alt strafes, Shift runs.\n"
           "Ctrl fires; 1-4 select weapons; Space uses doors, pushwalls and elevators; Escape returns to menu. --frames limits a smoke test.\n",
           name);
}

int main(int argc, char *argv[])
{
    const char *directory = NULL, *capture = NULL;
    bool check_only = false, check_ui = false, check_level = false, menu = false, level = false;
    unsigned long frames = 0;
    int status = 1;
    for (int i = 1; i < argc; ++i) {
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
    uint64_t ticks = VL_GetTicks70();
    if (!DemoLoopStartup(ticks,menu)) { fprintf(stderr,"Cannot load original UI: %s\n",CA_Error()); goto video_done; }
    if (level && !DemoLoopStartGame(ticks)) { fprintf(stderr,"E1M1: %s\n",CA_Error()); goto video_done; }
    if (capture) {
        w3r_byte saved[768]; memcpy(saved,palette,sizeof(saved));
        (void)VL_SetPalette(gamepal,sizeof(gamepal));
        bool captured = VL_Capture(capture);
        (void)VL_SetPalette(saved,sizeof(saved));
        if (!captured) { fprintf(stderr,"Cannot capture screen: %s\n",VL_Error()); goto video_done; }
    }
    for (unsigned long frame = 0; !VL_PollQuit() && DemoLoopState() != UI_DONE; ++frame) {
        if (!DemoLoop(VL_GetTicks70())) { fprintf(stderr,"Original UI: %s\n",CA_Error()); goto video_done; }
        if (!VW_UpdateScreen()) { fprintf(stderr, "SDL3 presentation: %s\n", VL_Error()); goto video_done; }
        if (frames && frame + 1 >= frames) break;
        VL_Sleep(4); /* Input stays responsive; deadlines use 70 Hz monotonic ticks. */
    }
    status = 0;
video_done:
    VL_Shutdown();
done:
    GameStop();
    CA_Shutdown();
    MM_Shutdown();
    return status;
}
