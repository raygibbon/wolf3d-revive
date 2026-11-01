/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, #x, SDL_GetError()); exit(1); } } while (0)

static void Pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint8 actual_r, actual_g, actual_b, a;
    CHECK(SDL_ReadSurfacePixel(surface, x, y, &actual_r, &actual_g, &actual_b, &a));
    CHECK(actual_r == r && actual_g == g && actual_b == b && a == 255);
}

int main(int argc, char **argv)
{
    w3r_byte colors[768] = {0};
    CHECK(argc == 2);
    colors[3] = 63; colors[7] = 63;
    CHECK(VL_SetPalette(colors, sizeof(colors)));
    for (size_t i = 0; i < VL_PIXELS; ++i) framebuffer[i] = (i % 320 < 160) ? 1 : 2;
    CHECK(VL_Startup());
    CHECK(VW_UpdateScreen());
    CHECK(VL_Capture(argv[1]));
    SDL_Surface *surface = SDL_LoadBMP(argv[1]);
    CHECK(surface && surface->w == 960 && surface->h == 720);
    Pixel(surface, 0, 0, 255, 0, 0);
    Pixel(surface, 959, 719, 0, 255, 0);
    SDL_DestroySurface(surface);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    CHECK(windows && count == 1);
    SDL_Window *window = windows[0];
    CHECK(SDL_SetWindowSize(windows[0], 1000, 600));
    CHECK(SDL_SyncWindow(windows[0]));
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    CHECK(renderer);
    SDL_free(windows);
    (void)VL_PollQuit();
    CHECK(VW_UpdateScreen());
    CHECK(VL_Capture(argv[1]));
    surface = SDL_LoadBMP(argv[1]);
    /* SDL readback clips to the logical viewport, excluding letterbox bars. */
    CHECK(surface && surface->w == 800 && surface->h == 600);
    SDL_FRect rect;
    CHECK(SDL_GetRenderLogicalPresentationRect(renderer, &rect));
    CHECK(rect.x == 100 && rect.y == 0 && rect.w == 800 && rect.h == 600);
    for (int x = 0; x < 800; ++x)
        Pixel(surface, x, 300, x < 400 ? 255 : 0, x < 400 ? 0 : 255, 0);
    SDL_DestroySurface(surface);
    /* A palette-only update must recolor an unchanged indexed framebuffer. */
    colors[3] = 0; colors[5] = 63;
    CHECK(VL_SetPalette(colors, sizeof(colors)) && VL_Capture(argv[1]));
    surface = SDL_LoadBMP(argv[1]); CHECK(surface);
    Pixel(surface, 0, 300, 0, 0, 255);
    SDL_DestroySurface(surface);
    CHECK(SDL_SetWindowSize(window, 600, 1000));
    CHECK(SDL_SyncWindow(window));
    (void)VL_PollQuit();
    CHECK(VW_UpdateScreen() && VL_Capture(argv[1]));
    CHECK(SDL_GetRenderLogicalPresentationRect(renderer, &rect));
    CHECK(rect.x == 0 && rect.y == 275 && rect.w == 600 && rect.h == 450);
    surface = SDL_LoadBMP(argv[1]);
    CHECK(surface && surface->w == 600 && surface->h == 450);
    Pixel(surface, 0, 0, 0, 0, 255);
    Pixel(surface, 599, 449, 0, 255, 0);
    SDL_DestroySurface(surface);
    SDL_Event event; memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_ESCAPE;
    CHECK(SDL_PushEvent(&event)); CHECK(VL_PollQuit());
    VL_Shutdown();
    puts("SDL3 render readback, 4:3 resize/letterbox, nearest colors, palette update and Escape passed.");
    return 0;
}
