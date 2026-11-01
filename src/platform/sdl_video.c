/* SPDX-License-Identifier: GPL-2.0-only */
/* SDL3 presentation boundary. Original graphics decoding lives in ID_CA/VL. */
#include "sdl_video.h"
#include "sdl_input.h"
#include "engine/ID_TIME.h"
#include "engine/ID_VL.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static w3r_byte rgba[VL_PIXELS * 4], last_frame[VL_PIXELS], last_palette[768];
static bool uploaded;
static uint64_t uploads;

const char *VL_Error(void) { return SDL_GetError(); }

void VL_Shutdown(void)
{
    SDL_DestroyTexture(texture); texture = NULL;
    SDL_DestroyRenderer(renderer); renderer = NULL;
    SDL_DestroyWindow(window); window = NULL;
    SDL_Quit();
    uploaded = false; IN_SDLFocusLost();
}

bool VL_Startup(void)
{
    char error[512];
    VL_Shutdown();
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) goto failed;
    window = SDL_CreateWindow("Wolf3D Revive", 960, 720, SDL_WINDOW_RESIZABLE);
    if (!window) goto failed;
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) goto failed;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING, W3R_SCREEN_WIDTH, W3R_SCREEN_HEIGHT);
    if (!texture || !SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
        !SDL_SetRenderLogicalPresentation(renderer,320,240,SDL_LOGICAL_PRESENTATION_LETTERBOX)) goto failed;
    uploads = 0;
    return true;
failed:
    (void)SDL_strlcpy(error,SDL_GetError(),sizeof(error));
    VL_Shutdown();
    (void)SDL_SetError("%s",error);
    return false;
}
uint64_t VL_GetTicks70(void) { return TimeCount_FromNS(SDL_GetTicksNS()); }
void VL_Sleep(unsigned ms) { SDL_Delay(ms); }
uint64_t VL_TextureUploads(void) { return uploads; }

static bool VL_DrawScreen(void)
{
    if (!renderer || !texture) return false;
    if (!uploaded || memcmp(last_frame,framebuffer,VL_PIXELS) || memcmp(last_palette,palette,768)) {
        if (!VL_ConvertRGBA(rgba,sizeof(rgba)) ||
            !SDL_UpdateTexture(texture,NULL,rgba,W3R_SCREEN_WIDTH*4)) return false;
        memcpy(last_frame,framebuffer,VL_PIXELS); memcpy(last_palette,palette,768);
        uploaded = true; ++uploads;
    }
    return SDL_SetRenderDrawColor(renderer,0,0,0,255) && SDL_RenderClear(renderer) &&
           SDL_RenderTexture(renderer,texture,NULL,NULL);
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
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) quit = true;
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) IN_SDLFocusLost();
        else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) IN_SDLKey((unsigned)event.key.scancode,true);
        else if (event.type == SDL_EVENT_KEY_UP) IN_SDLKey((unsigned)event.key.scancode,false);
    }
    return quit;
}
