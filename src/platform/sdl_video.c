/* SPDX-License-Identifier: GPL-2.0-only */
/* SDL3 presentation boundary. Original graphics decoding lives in ID_CA/VL. */
#include "sdl_video.h"
#include "sdl_input.h"
#include "engine/ID_TIME.h"
#include "engine/WL_CONTROLS.h"
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
/* Actual fullscreen state always comes from SDL flags. Only an outstanding
   asynchronous request has a target; it never masquerades as actual state. */
static bool integer_scale,pending,target,restore_size,size_pending;
static int window_w=960,window_h=720;
static Uint64 request_deadline;
static SDL_RendererLogicalPresentation applied_mode=SDL_LOGICAL_PRESENTATION_DISABLED;
static char display_error[160];
static bool consumed[SDL_SCANCODE_COUNT];
#ifdef WOLF3D_DISPLAY_TEST
static unsigned fault;
void VL_TestDisplayFailure(unsigned operation) { fault=operation; }
void VL_TestExpireDisplayRequest(void) { request_deadline=0; }
static bool Fail(unsigned operation) { if(fault!=operation) return false;fault=0;SDL_SetError("Injected display failure");return true; }
#else
static bool Fail(unsigned operation) { (void)operation;return false; }
#endif
static bool Fullscreen(void) { return window && (SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0; }
static bool DisplayError(void)
{
    (void)SDL_strlcpy(display_error,SDL_GetError(),sizeof(display_error));
    SDL_Log("Display change failed: %s",display_error);return false;
}
static bool ApplyPresentation(void)
{
    int w,h;if(!renderer || !SDL_GetRenderOutputSize(renderer,&w,&h)) return false;
    /* SDL integer mode enforces at least 1x and otherwise crops tiny windows.
       Fall back to fit below the corrected canvas's minimum drawable size. */
    SDL_RendererLogicalPresentation mode=integer_scale && w>=320 && h>=240?
        SDL_LOGICAL_PRESENTATION_INTEGER_SCALE:SDL_LOGICAL_PRESENTATION_LETTERBOX;
    if(applied_mode==mode) return true;
    if(Fail(2) || !SDL_SetRenderLogicalPresentation(renderer,320,240,mode)) return false;
    applied_mode=mode;return true;
}
static void RememberWindow(void)
{
    if(!window || pending || (SDL_GetWindowFlags(window)&(SDL_WINDOW_FULLSCREEN|SDL_WINDOW_MINIMIZED|SDL_WINDOW_MAXIMIZED))) return;
    int w,h;if(SDL_GetWindowSize(window,&w,&h) && w>0 && h>0) { window_w=w;window_h=h; }
}
static bool WindowSize(int w,int h) { return !Fail(3) && SDL_SetWindowSize(window,w,h); }
static void RefreshDisplay(void)
{
    if(!window) return;
    if(pending && Fullscreen()==target) {
        if(!target && restore_size) {
            restore_size=false;
            if(!WindowSize(window_w,window_h)) { (void)DisplayError();pending=false; }
            else size_pending=true;
        }
        int w=0,h=0;
        if(size_pending && SDL_GetWindowSize(window,&w,&h) && w==window_w && h==window_h) size_pending=false;
        if(!size_pending) { pending=false;IN_SDLDiscardInput(); }
    }
    if(pending && SDL_GetTicks()>=request_deadline) {
        pending=size_pending=restore_size=false;
        SDL_SetError("Window manager did not apply the display request");(void)DisplayError();IN_SDLDiscardInput();
    }
    RememberWindow();
}
bool VL_DisplayGet(display_settings *s)
{
    if(!window || !renderer || !s) return false;
    RefreshDisplay();memset(s,0,sizeof(*s));s->fullscreen=Fullscreen();s->integer=integer_scale;s->pending=pending;
    s->width=window_w;s->height=window_h;
    for(unsigned i=1;i<=5;++i) if(window_w==(int)i*320 && window_h==(int)i*240) s->scale=i;
    if(!SDL_GetRenderOutputSize(renderer,&s->pixel_width,&s->pixel_height)) return false;
    (void)SDL_strlcpy(s->error,display_error,sizeof(s->error));return true;
}
bool VL_DisplaySet(const display_settings *s)
{
    if(!window || !renderer || !s || s->scale>5) return SDL_SetError("Invalid display settings");
    RefreshDisplay();if(pending) return SDL_SetError("Fullscreen request still pending");
    RememberWindow();bool old_integer=integer_scale;
    integer_scale=s->integer;
    if(!ApplyPresentation()) { integer_scale=old_integer;return DisplayError(); }
    int old_w=window_w,old_h=window_h;
    if(s->scale) { window_w=(int)s->scale*320;window_h=(int)s->scale*240; }
    if(s->fullscreen!=Fullscreen()) {
        IN_SDLCapture(window,false);IN_SDLDiscardInput();
        bool denied=Fail(4);
        if(Fail(1) || (!denied && ((s->fullscreen && !SDL_SetWindowFullscreenMode(window,NULL)) || !SDL_SetWindowFullscreen(window,s->fullscreen)))) {
            window_w=old_w;window_h=old_h;integer_scale=old_integer;
            char error[160];(void)SDL_strlcpy(error,SDL_GetError(),sizeof(error));
            (void)ApplyPresentation();(void)SDL_SetError("%s",error);return DisplayError();
        }
        pending=true;target=s->fullscreen;restore_size=!target;request_deadline=SDL_GetTicks()+3000;
    } else if(!Fullscreen() && (window_w!=old_w || window_h!=old_h)) {
        if(!WindowSize(window_w,window_h)) {
            window_w=old_w;window_h=old_h;integer_scale=old_integer;
            char error[160];(void)SDL_strlcpy(error,SDL_GetError(),sizeof(error));
            (void)ApplyPresentation();(void)SDL_SetError("%s",error);return DisplayError();
        }
        IN_SDLCapture(window,false);IN_SDLDiscardInput();
        pending=size_pending=true;target=false;request_deadline=SDL_GetTicks()+3000;
    }
    display_error[0]=0;return true;
}
static bool Shortcut(const SDL_KeyboardEvent *key)
{
    unsigned code=(unsigned)key->scancode;
    if(code>=SDL_SCANCODE_COUNT) return false;
    if(consumed[code]) { if(key->type==SDL_EVENT_KEY_UP) consumed[code]=false;return true; }
    if(key->type!=SDL_EVENT_KEY_DOWN || key->repeat || !IN_SDLFocused()) return false;
    bool full=(code==SDL_SCANCODE_RETURN || code==SDL_SCANCODE_KP_ENTER) && (key->mod&SDL_KMOD_ALT);
    if(!full && code!=SDL_SCANCODE_F11) return false;
    consumed[code]=true;
    if(full) {
        if(key->mod&SDL_KMOD_LALT) consumed[SDL_SCANCODE_LALT]=true;
        if(key->mod&SDL_KMOD_RALT) consumed[SDL_SCANCODE_RALT]=true;
        IN_SDLDiscardInput();
    }
    display_settings s;if(!VL_DisplayGet(&s) || s.pending) return true;
    if(full) s.fullscreen=!s.fullscreen;else s.integer=!s.integer;
    (void)VL_DisplaySet(&s);return true;
}

const char *VL_Error(void) { return SDL_GetError(); }

void VL_Shutdown(void)
{
    IN_SDLFocusLost();
    SDL_DestroyTexture(texture); texture = NULL;
    SDL_DestroyRenderer(renderer); renderer = NULL;
    SDL_DestroyWindow(window); window = NULL;
    SDL_Quit();
    uploaded=false;integer_scale=pending=target=restore_size=size_pending=false;
    window_w=960;window_h=720;applied_mode=SDL_LOGICAL_PRESENTATION_DISABLED;
    display_error[0]=0;memset(consumed,0,sizeof(consumed));IN_SDLFocusLost();
}

bool VL_Startup(void)
{
    char error[512];
    VL_Shutdown();
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) goto failed;
    window = SDL_CreateWindow("Wolf3D Revive", 960, 720, SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) goto failed;
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) goto failed;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING, W3R_SCREEN_WIDTH, W3R_SCREEN_HEIGHT);
    if (!texture || !SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
        !ApplyPresentation()) goto failed;
    IN_SDLReset();uploads = 0;
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
    if (!renderer || !texture || !ApplyPresentation()) return false;
    if (!uploaded || memcmp(last_frame,framebuffer,VL_PIXELS) || memcmp(last_palette,palette,768)) {
        if (!VL_ConvertRGBA(rgba,sizeof(rgba)) ||
            !SDL_UpdateTexture(texture,NULL,rgba,W3R_SCREEN_WIDTH*4)) return false;
        memcpy(last_frame,framebuffer,VL_PIXELS); memcpy(last_palette,palette,768);
        uploaded = true; ++uploads;
    }
    int w,h;if(!SDL_GetRenderOutputSize(renderer,&w,&h)) return false;
    if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer)) return false;
    return (w<4 || h<3) || SDL_RenderTexture(renderer,texture,NULL,NULL);
}

static void SyncCapture(void) { RefreshDisplay();IN_SDLCapture(window,CT_Gameplay() && !pending); }
bool VW_UpdateScreen(void)
{
    SyncCapture();
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
    SyncCapture();
    SDL_Event event;
    bool quit = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) quit = true;
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) { memset(consumed,0,sizeof(consumed));IN_SDLFocusLost(); }
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) IN_SDLFocusGained();
        else if (event.type == SDL_EVENT_MOUSE_MOTION) IN_SDLMouseMotion(event.motion.xrel);
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            IN_SDLMouseButton(event.button.button,event.type==SDL_EVENT_MOUSE_BUTTON_DOWN);
        else if(event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) {
            if(!Shortcut(&event.key) && (event.type==SDL_EVENT_KEY_UP || !event.key.repeat)) IN_SDLKey((unsigned)event.key.scancode,event.type==SDL_EVENT_KEY_DOWN);
        }

    }
    SyncCapture();return quit;
}
