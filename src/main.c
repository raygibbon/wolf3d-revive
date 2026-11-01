/* SPDX-License-Identifier: GPL-2.0-only */
/* WOLFSRC/WL_MAIN.C DemoLoop title slice: cache, present, wait for input.
   Gameplay, attract rotation, fades and audio are intentionally not ported yet. */
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void Usage(const char *name)
{
    printf("Usage: %s --data /path/to/WL6 [--check-assets] [--frames N] [--capture local.bmp]\n"
           "Display the original Wolfenstein 3D WL6 title. Escape closes the window.\n"
           "--check-assets decodes without opening a window. --frames limits a smoke test.\n",
           name);
}

int main(int argc, char *argv[])
{
    const char *directory = NULL, *capture = NULL;
    bool check_only = false;
    unsigned long frames = 0;
    int status = 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { Usage(argv[0]); return 0; }
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
    if (!directory || (check_only && capture)) {
        fputs("Supply --data with a WL6 directory; --capture requires a window.\n", stderr);
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
    if (!VL_Startup()) { fprintf(stderr, "SDL3 video: %s\n", VL_Error()); goto video_done; }
    if (capture && !VL_Capture(capture)) {
        fprintf(stderr, "Cannot capture title: %s\n", VL_Error()); goto video_done;
    }
    for (unsigned long frame = 0; !VL_PollQuit(); ++frame) {
        if (!VW_UpdateScreen()) { fprintf(stderr, "SDL3 presentation: %s\n", VL_Error()); goto video_done; }
        if (frames && frame + 1 >= frames) break;
        SDL_Delay(16); /* Presentation only; there is no simulation in this milestone. */
    }
    status = 0;
video_done:
    VL_Shutdown();
done:
    CA_Shutdown();
    MM_Shutdown();
    return status;
}
