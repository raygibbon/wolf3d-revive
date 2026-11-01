/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/ID_VL.h"
#include "platform/sdl_video.h"
#include "platform/sdl_input.h"
#include "engine/ID_IN.h"
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
    CHECK(VL_TextureUploads() == 1);
    CHECK(VW_UpdateScreen() && VL_TextureUploads() == 1);
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
    CHECK(SDL_PushEvent(&event)); CHECK(!VL_PollQuit());
    CHECK(Keyboard[sc_Escape] && IN_ConsumePress(sc_Escape));
    event.type = SDL_EVENT_KEY_UP;
    CHECK(SDL_PushEvent(&event)); CHECK(!VL_PollQuit()); CHECK(!Keyboard[sc_Escape]);
    CHECK(IN_MapSDLScancode(SDL_SCANCODE_A) == 0x1e);
    CHECK(IN_MapSDLScancode(SDL_SCANCODE_Z) == 0x2c);
    CHECK(IN_MapSDLScancode(SDL_SCANCODE_UP) == sc_UpArrow);
    CHECK(IN_MapSDLScancode(SDL_SCANCODE_KP_ENTER) == sc_Enter);
    CHECK(IN_MapSDLScancode(99999) == 0);
    IN_SDLKey(SDL_SCANCODE_LCTRL,true); IN_SDLKey(SDL_SCANCODE_RCTRL,true);
    IN_SDLKey(SDL_SCANCODE_LCTRL,false); CHECK(Keyboard[0x1d]);
    IN_SDLKey(SDL_SCANCODE_RCTRL,false); CHECK(!Keyboard[0x1d]);
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_DOWN;
    CHECK(SDL_PushEvent(&event)); CHECK(!VL_PollQuit());
    CHECK(Keyboard[sc_DownArrow] && IN_ConsumePress(sc_DownArrow));
    event.key.repeat = true;
    CHECK(SDL_PushEvent(&event)); CHECK(!VL_PollQuit()); CHECK(!IN_ConsumePress(sc_DownArrow));
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    CHECK(SDL_PushEvent(&event)); CHECK(!VL_PollQuit()); CHECK(!Keyboard[sc_DownArrow]);
    CHECK(!IN_AnyPressed());
    event.type = SDL_EVENT_QUIT;
    CHECK(SDL_PushEvent(&event)); CHECK(VL_PollQuit());
    CHECK(VL_TextureUploads() == 2); /* Resize needs no new upload; palette change does. */
    VL_Shutdown();
    for (int n = 0; n < 3; ++n) {
        CHECK(VL_Startup()); CHECK(VW_UpdateScreen()); VL_Shutdown(); VL_Shutdown();
        CHECK(SDL_WasInit(SDL_INIT_VIDEO) == 0);
    }
    CHECK(SDL_setenv_unsafe("SDL_RENDER_DRIVER","no-such-renderer",1) == 0);
    for (int n = 0; n < 2; ++n) {
        CHECK(!VL_Startup()); CHECK(VL_Error()[0]);
        CHECK(SDL_WasInit(SDL_INIT_VIDEO) == 0);
        windows = SDL_GetWindows(&count); CHECK(count == 0); SDL_free(windows);
    }
    CHECK(SDL_setenv_unsafe("SDL_RENDER_DRIVER","software",1) == 0);
    CHECK(VL_Startup()); CHECK(VW_UpdateScreen()); VL_Shutdown();
    puts("SDL3 render readback, 4:3 resize/letterbox, nearest colors, palette update and Escape passed.");
    return 0;
}
