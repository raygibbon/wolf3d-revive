/* SPDX-License-Identifier: GPL-2.0-only */
/* SDL3 presentation boundary. Original graphics decoding lives in ID_CA/VL. */
#include "sdl_video.h"
#include "engine/ID_VL.h"
#include <SDL3/SDL.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static w3r_byte rgba[VL_PIXELS * 4];

const char *VL_Error(void) { return SDL_GetError(); }

void VL_Shutdown(void)
{
    SDL_DestroyTexture(texture); texture = NULL;
    SDL_DestroyRenderer(renderer); renderer = NULL;
    SDL_DestroyWindow(window); window = NULL;
    SDL_Quit();
}

bool VL_Startup(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    window = SDL_CreateWindow("Wolf3D Redux — original WL6 title", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) return false;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING, W3R_SCREEN_WIDTH, W3R_SCREEN_HEIGHT);
    if (!texture) return false;
    /* VGA's 320x200 image fills a 4:3 display, not a square-pixel 8:5 viewport. */
    return SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) &&
           SDL_SetRenderLogicalPresentation(renderer, 320, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

static bool VL_DrawScreen(void)
{
    return renderer && texture && VL_ConvertRGBA(rgba, sizeof(rgba)) &&
           SDL_UpdateTexture(texture, NULL, rgba, W3R_SCREEN_WIDTH * 4) &&
           SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) &&
           SDL_RenderClear(renderer) && SDL_RenderTexture(renderer, texture, NULL, NULL);
}

bool VW_UpdateScreen(void)
{
    return VL_DrawScreen() && SDL_RenderPresent(renderer);
}

bool VL_Capture(const char *path)
{
    SDL_Surface *surface;
    bool ok;
    if (!path || !VL_DrawScreen()) return false;
    surface = SDL_RenderReadPixels(renderer, NULL);
    if (!surface) return false;
    ok = SDL_SaveBMP(surface, path);
    SDL_DestroySurface(surface);
    return ok;
}

bool VL_PollQuit(void)
{
    SDL_Event event;
    bool quit = false;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
            (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE))
            quit = true;
    return quit;
}
