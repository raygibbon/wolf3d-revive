/* SPDX-License-Identifier: GPL-2.0-only */
/* Build/dependency probe, not the game entry point. No assets are accessed. */
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "w3r_types.h"

int main(int argc, char *argv[])
{
    int version;
    Uint64 before;
    Uint64 after;
    (void)argc;
    (void)argv;

    if (!SDL_Init(0)) {
        fprintf(stderr, "SDL3 initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    version = SDL_GetVersion();
    before = SDL_GetTicksNS();
    SDL_Delay(1);
    after = SDL_GetTicksNS();
    SDL_Quit();
    if (after < before) {
        fputs("SDL3 monotonic clock moved backwards\n", stderr);
        return 1;
    }
    printf("Wolf3D Redux milestone 1: C11 / SDL %d.%d.%d dependency probe\n",
           SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version),
           SDL_VERSIONNUM_MICRO(version));
    printf("Target: %dx%d indexed video, %d Hz gameplay. Engine not linked yet.\n",
           W3R_SCREEN_WIDTH, W3R_SCREEN_HEIGHT, W3R_TICK_HZ);
    return 0;
}
