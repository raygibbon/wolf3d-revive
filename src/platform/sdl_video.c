/* SPDX-License-Identifier: GPL-2.0-only */
/* SDL3 presentation boundary. Original graphics decoding lives in ID_CA/VL. */
#include "sdl_video.h"
#include "sdl_audio.h"
#include "sdl_input.h"
#include "engine/ID_TIME.h"
#include "engine/WL_CONTROLS.h"
#include "engine/ID_VL.h"
#include "engine/WL_RENDER.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_DEMO.h"
#include <stdlib.h>
#include <stdio.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static w3r_byte rgba[VL_PIXELS * 4], last_frame[VL_PIXELS], last_palette[768];
static bool Fail(unsigned operation);
typedef struct { SDL_Texture *texture;w3r_byte *pixels,*rgba;wl_render_target target; } high_surface;
static high_surface high;
static bool high_resolution,render_auto,vsync,linear,smooth;
static unsigned presentation_fps=144;
static bool high_smooth;
static wl_present_snapshot presentation;
static unsigned aspect;
static Uint64 auto_retry;
static wl_rectangle high_scene;
static unsigned render_width=640,render_height=480;
static int32_t high_depth[WL_TARGET_MAX_WIDTH];
static uint32_t high_time,high_death_cursor;
static unsigned high_death_phase;
static bool high_dying;
static uint64_t high_serial,high_uploads;
static bool high_world,high_uploaded;
static void FreeHigh(high_surface *s)
{ if(s->texture) SDL_DestroyTexture(s->texture);free(s->pixels);free(s->rgba);memset(s,0,sizeof(*s)); }
static bool RenderSize(bool automatic,unsigned *w,unsigned *h)
{
    if(!automatic) return true;
    const SDL_DisplayMode *desktop=SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(window));
    if(!desktop || desktop->w<320 || desktop->h<200) return SDL_SetError("Desktop rendering dimensions unavailable");
    wl_rectangle fit;
    if(!WL_AspectFit(WL_TARGET_MAX_WIDTH,WL_TARGET_MAX_HEIGHT,(unsigned)desktop->w,(unsigned)desktop->h,&fit)) return false;
    *w=(unsigned)desktop->w;*h=(unsigned)desktop->h;
    if(*w>fit.width || *h>fit.height) { *w=fit.width;*h=fit.height; }
    return true;
}
static bool CreateHigh(high_surface *s,unsigned w,unsigned h)
{
    memset(s,0,sizeof(*s));
    if(!WL_RenderResolution(w,h)) return SDL_SetError("Unsupported rendering resolution");
    size_t bytes=(size_t)w*h;
    if(Fail(5)) return false;
    s->pixels=malloc(bytes);s->rgba=malloc(bytes*4);
    if(!s->pixels || !s->rgba || !WL_TargetInit(&s->target,s->pixels,bytes,w,h,w)) {
        FreeHigh(s);return SDL_SetError("Cannot allocate rendering surface");
    }
    if(Fail(6)) { FreeHigh(s);return false; }
    s->texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,(int)w,(int)h);
    if(!s->texture || !SDL_SetTextureScaleMode(s->texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST)) { FreeHigh(s);return false; }
    return true;
}
static bool uploaded;
static uint64_t uploads;
/* Actual fullscreen state always comes from SDL flags. Only an outstanding
   asynchronous request has a target; it never masquerades as actual state. */
static bool integer_scale,pending,target,restore_size,size_pending;
static int window_w=960,window_h=720;
static Uint64 request_deadline;
static int applied_w,applied_h;
static SDL_RendererLogicalPresentation applied_mode=SDL_LOGICAL_PRESENTATION_DISABLED;
static char display_error[160];
static bool consumed[SDL_SCANCODE_COUNT];
#ifdef WOLF3D_DISPLAY_TEST
static unsigned fault;
uint64_t VL_TestSceneUploads(void) { return high_uploads; }
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
    int lw=320,lh=240;
    bool world=high_resolution && GameRunning() && (DemoLoopState()==UI_GAME || DemoLoopState()==UI_DEMO);
    if(world && high_scene.width) { lw=(int)high_scene.width;lh=(int)high_scene.height; }
    SDL_RendererLogicalPresentation mode=integer_scale && w>=lw && h>=lh?
        SDL_LOGICAL_PRESENTATION_INTEGER_SCALE:SDL_LOGICAL_PRESENTATION_LETTERBOX;
    if(applied_mode==mode && applied_w==lw && applied_h==lh) return true;
    if(Fail(2) || !SDL_SetRenderLogicalPresentation(renderer,lw,lh,mode)) return false;
    applied_mode=mode;applied_w=lw;applied_h=lh;return true;
}
static bool SettingsFailure(high_surface *candidate)
{
    char saved[160];(void)SDL_strlcpy(saved,SDL_GetError(),sizeof(saved));
    (void)SDL_SetRenderVSync(renderer,vsync?1:0);
    (void)SDL_SetTextureScaleMode(texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
    if(high.texture) (void)SDL_SetTextureScaleMode(high.texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
    FreeHigh(candidate);(void)SDL_SetError("%s",saved);return DisplayError();
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
    s->high_resolution=high_resolution;s->render_width=render_width;s->render_height=render_height;
    s->render_auto=render_auto;s->aspect=aspect;s->vsync=vsync;s->linear=linear;s->smooth=smooth;s->presentation_fps=presentation_fps;
    for(unsigned i=1;i<=5;++i) if(window_w==(int)i*320 && window_h==(int)i*240) s->scale=i;
    if(!SDL_GetRenderOutputSize(renderer,&s->pixel_width,&s->pixel_height)) return false;
    (void)SDL_strlcpy(s->error,display_error,sizeof(s->error));return true;
}
bool VL_DisplayPreferences(display_settings *s)
{
    if(!VL_DisplayGet(s)) return false;
    if(pending) s->fullscreen=target;
    return true;
}
bool VL_DisplaySet(const display_settings *s)
{
    if(!window || !renderer || !s || s->scale>5 || (!s->scale && (s->width<1 || s->height<1 || s->width>16384 || s->height>16384))) return SDL_SetError("Invalid display settings");
    RefreshDisplay();if(pending) return SDL_SetError("Fullscreen request still pending");
    unsigned rw=s->render_width?s->render_width:640,rh=s->render_height?s->render_height:480;
    if(!((rw==640 && rh==480) || (rw==1280 && rh==720) || (rw==1920 && rh==1080)) || s->aspect>5)
        return SDL_SetError("Unsupported rendering resolution/aspect");
    unsigned actual_w=rw,actual_h=rh;
    if(s->high_resolution && !RenderSize(s->render_auto,&actual_w,&actual_h)) return DisplayError();
    unsigned fps=s->presentation_fps?s->presentation_fps:144;
    if(fps!=60 && fps!=120 && fps!=144 && fps!=165 && fps!=240) return SDL_SetError("Unsupported presentation FPS");
    high_surface candidate={0};
    bool replace=s->high_resolution && (!high.texture || actual_w!=high.target.width || actual_h!=high.target.height);
    if(replace && !CreateHigh(&candidate,actual_w,actual_h)) return DisplayError();
    if(s->vsync!=vsync && (Fail(7) || !SDL_SetRenderVSync(renderer,s->vsync?1:0))) { return SettingsFailure(&candidate); }
    SDL_ScaleMode filter=s->linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST;
    if(Fail(8) || !SDL_SetTextureScaleMode(texture,filter) ||
       (high.texture && !SDL_SetTextureScaleMode(high.texture,filter)) ||
       (candidate.texture && !SDL_SetTextureScaleMode(candidate.texture,filter))) {
        (void)SDL_SetRenderVSync(renderer,vsync?1:0);
        (void)SDL_SetTextureScaleMode(texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
        if(high.texture) (void)SDL_SetTextureScaleMode(high.texture,linear?SDL_SCALEMODE_LINEAR:SDL_SCALEMODE_NEAREST);
        return SettingsFailure(&candidate);
    }
    RememberWindow();bool old_integer=integer_scale;
    integer_scale=s->integer;
    if(!ApplyPresentation()) { integer_scale=old_integer;return SettingsFailure(&candidate); }
    int old_w=window_w,old_h=window_h;
    if(s->scale) { window_w=(int)s->scale*320;window_h=(int)s->scale*240; }
    else { window_w=s->width;window_h=s->height; }
    if(s->fullscreen!=Fullscreen()) {
        IN_SDLCapture(window,false);IN_SDLDiscardInput();
        bool denied=Fail(4);
        if(Fail(1) || (!denied && ((s->fullscreen && !SDL_SetWindowFullscreenMode(window,NULL)) || !SDL_SetWindowFullscreen(window,s->fullscreen)))) {
            window_w=old_w;window_h=old_h;integer_scale=old_integer;
            char error[160];(void)SDL_strlcpy(error,SDL_GetError(),sizeof(error));
            (void)ApplyPresentation();(void)SDL_SetError("%s",error);return SettingsFailure(&candidate);
        }
        pending=true;target=s->fullscreen;restore_size=!target;request_deadline=SDL_GetTicks()+3000;
    } else if(!Fullscreen() && (window_w!=old_w || window_h!=old_h)) {
        if(!WindowSize(window_w,window_h)) {
            window_w=old_w;window_h=old_h;integer_scale=old_integer;
            char error[160];(void)SDL_strlcpy(error,SDL_GetError(),sizeof(error));
            (void)ApplyPresentation();(void)SDL_SetError("%s",error);return SettingsFailure(&candidate);
        }
        IN_SDLCapture(window,false);IN_SDLDiscardInput();
        pending=size_pending=true;target=false;request_deadline=SDL_GetTicks()+3000;
    }
    if(replace) { FreeHigh(&high);high=candidate; }
    if(!s->high_resolution) FreeHigh(&high);
    if(replace || high_resolution!=s->high_resolution || aspect!=s->aspect || render_auto!=s->render_auto || smooth!=s->smooth) high_uploaded=false;
    high_resolution=s->high_resolution;render_width=rw;render_height=rh;
    aspect=s->aspect;render_auto=s->render_auto;vsync=s->vsync;linear=s->linear;smooth=s->smooth;presentation_fps=fps;auto_retry=0;
    display_error[0]=0;return true;
}
static bool Shortcut(const SDL_KeyboardEvent *key)
{
    unsigned code=(unsigned)key->scancode;
    if(code>=SDL_SCANCODE_COUNT) return false;
    if(consumed[code]) { if(key->type==SDL_EVENT_KEY_UP) consumed[code]=false;return true; }
    if(key->type!=SDL_EVENT_KEY_DOWN || key->repeat || !IN_SDLFocused()) return false;
    bool full=(code==SDL_SCANCODE_RETURN || code==SDL_SCANCODE_KP_ENTER) && (key->mod&SDL_KMOD_ALT);
    if(!full && code!=SDL_SCANCODE_F11 && code!=SDL_SCANCODE_F12) return false;
    if(code==SDL_SCANCODE_F12) {
        consumed[code]=true;
        if(!VL_Screenshot()) { (void)SDL_strlcpy(display_error,SDL_GetError(),sizeof(display_error));SDL_Log("Screenshot failed: %s",display_error); }
        return true;
    }
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
    high_uploads=0;smooth=high_smooth=false;presentation_fps=144;FreeHigh(&high);high_resolution=high_uploaded=render_auto=vsync=linear=false;aspect=0;high_scene=(wl_rectangle){0};render_width=640;render_height=480;
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
    if (!renderer || !texture) return false;
    bool changed=!uploaded || memcmp(last_frame,framebuffer,VL_PIXELS) || memcmp(last_palette,palette,768);
    bool world=GameRunning() && (DemoLoopState()==UI_GAME || DemoLoopState()==UI_DEMO);
    if(high_resolution && world) {
        unsigned tw=render_width,th=render_height;
        if(!RenderSize(render_auto,&tw,&th)) { (void)DisplayError();tw=high.target.width;th=high.target.height; }
        if(!high.texture || high.target.width!=tw || high.target.height!=th) {
            high_surface candidate;
            if(SDL_GetTicks()<auto_retry || !CreateHigh(&candidate,tw,th)) {
                if(SDL_GetTicks()>=auto_retry) { (void)DisplayError();auto_retry=SDL_GetTicks()+1000; }
                if(!high.texture) return false;
                tw=high.target.width;th=high.target.height;
            } else { FreeHigh(&high);high=candidate;high_uploaded=false;auto_retry=0; }
        }
        unsigned aw=4,ah=3;
        static const unsigned ratios[][2]={{4,3},{4,3},{16,9},{16,10},{21,9},{32,9}};
        if(aspect) { aw=ratios[aspect][0];ah=ratios[aspect][1]; }
        else {
            int pw,ph;if(!SDL_GetRenderOutputSize(renderer,&pw,&ph) || pw<1 || ph<1) return false;
            aw=(unsigned)pw;ah=(unsigned)ph;
            if((uint64_t)aw*3<(uint64_t)ah*4) { aw=4;ah=3; }
            if((uint64_t)aw*9>(uint64_t)ah*32) { aw=32;ah=9; }
        }
        wl_rectangle scene;
        if(!WL_AspectFit(tw,th,aw,ah,&scene)) return SDL_SetError("Invalid aspect geometry");
        if(memcmp(&scene,&high_scene,sizeof(scene))) high_uploaded=false;
        high_scene=scene;
    }
    if(!ApplyPresentation()) return false;
    uint32_t death_cursor=0;unsigned death_phase=GameDeathPresentation(&death_cursor);
    bool dying=world && gamestate.died;
    bool interpolate=high_resolution && smooth && world && (GameCanSave() || Demo_Active() || (dying && death_phase==1)) && (!dying || death_phase==1) && !gamestate.victoryflag;
    bool sampled=interpolate && WL_PresentSample(SDL_GetTicksNS(),&presentation);
    if(high_resolution && world && (!high_uploaded || changed || world!=high_world || sampled || sampled!=high_smooth || dying!=high_dying || (dying && (death_phase!=high_death_phase || death_cursor!=high_death_cursor)) || (world && (high_time!=gamestate.TimeCount || high_serial!=WL_RenderSerial())))) {
        if(!WL_RenderSnapshot(&high.target,high_depth,WL_TARGET_MAX_WIDTH,world,high_scene.width,high_scene.height,sampled?&presentation:NULL,Audio_Pump)) return SDL_SetError("High-resolution scene rendering failed");
        w3r_byte colors[256][4];
        for(unsigned i=0;i<256;++i) {
            for(unsigned c=0;c<3;++c) { unsigned value=palette[i*3+c];colors[i][c]=(w3r_byte)((value<<2)|(value>>4)); }
            colors[i][3]=255;
        }
        for(unsigned y=0;y<high.target.height;++y) {
            if(y%32==0) Audio_Pump();
            for(unsigned x=0;x<high.target.width;++x) {
                size_t i=(size_t)y*high.target.width+x;
                memcpy(high.rgba+i*4,colors[high.pixels[i]],4);
            }
        }
        if(!SDL_UpdateTexture(high.texture,NULL,high.rgba,(int)high.target.width*4)) return false;
        high_smooth=sampled;high_death_cursor=death_cursor;high_death_phase=death_phase;high_dying=dying;
        high_uploaded=true;high_time=gamestate.TimeCount;high_world=world;high_serial=WL_RenderSerial();++high_uploads;
    }
    if (changed) {
        if (!VL_ConvertRGBA(rgba,sizeof(rgba)) ||
            !SDL_UpdateTexture(texture,NULL,rgba,W3R_SCREEN_WIDTH*4)) return false;
        memcpy(last_frame,framebuffer,VL_PIXELS); memcpy(last_palette,palette,768);
        uploaded = true; ++uploads;
    }
    int w,h;if(!SDL_GetRenderOutputSize(renderer,&w,&h)) return false;
    if(!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer)) return false;
    if(w<4 || h<3) return true;
    if(high_resolution && world) {
        SDL_FRect src={(float)high_scene.x,(float)high_scene.y,(float)high_scene.width,(float)high_scene.height};
        return SDL_RenderTexture(renderer,high.texture,&src,NULL);
    }
    return SDL_RenderTexture(renderer,texture,NULL,NULL);
}

static void SyncCapture(void) { RefreshDisplay();IN_SDLCapture(window,CT_Gameplay() && !pending); }
bool VW_UpdateScreen(void)
{
    SyncCapture();Audio_Pump();
    if(!VL_DrawScreen()) return false;
    Audio_Pump();return SDL_RenderPresent(renderer);
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

bool VL_Screenshot(void)
{
    char *dir=SDL_GetPrefPath(NULL,"wolf3d-revive");if(!dir) return false;
    char path[4096];bool ok=false;
    /* Monotonic suffix plus existence check prevents overwriting prior shots. */
    for(unsigned i=0;i<1000;++i) {
        int n=snprintf(path,sizeof(path),"%sscreenshot-%llu-%u.bmp",dir,(unsigned long long)SDL_GetTicksNS(),i);
        if(n<=0 || (size_t)n>=sizeof(path)) { SDL_SetError("Screenshot path too long");break; }
        SDL_PathInfo info;
        if(SDL_GetPathInfo(path,&info)) continue;
        ok=VL_Capture(path);break;
    }
    SDL_free(dir);return ok;
}
