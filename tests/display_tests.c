/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/ID_VL.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DISPLAY.h"
#include "platform/sdl_input.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,SDL_GetError(),CA_Error());exit(1); } } while(0)
#include "finish_loading.h"
static SDL_Window *window;
static SDL_Renderer *renderer;
static uint64_t ticks;
static const int presets[][2]={
    {640,480},{960,720},{1280,960},{960,600},{1280,800},{1600,1000},{1920,1200},
    {960,540},{1280,720},{1600,900},{1920,1080},{1680,720},{2560,1080},{1920,540},{2560,720}
};
static display_settings Settings(void) { display_settings s;CHECK(VL_DisplayGet(&s));return s; }
static void Pump(void) { CHECK(!VL_PollQuit());CHECK(VW_UpdateScreen()); }
static void AwaitDisplay(void)
{
    Uint64 until=SDL_GetTicks()+4000;
    do { CHECK(SDL_SyncWindow(window));Pump();if(!Settings().pending) return;SDL_Delay(2); } while(SDL_GetTicks()<until);
    CHECK(!Settings().pending);
}
static void Set(display_settings s)
{
    CHECK(VL_DisplaySet(&s));AwaitDisplay();CHECK(!Settings().pending);
}
static void Resize(int w,int h) { CHECK(SDL_SetWindowSize(window,w,h));CHECK(SDL_SyncWindow(window));Pump(); }
static void Key(SDL_Scancode code,bool down,SDL_Keymod mod)
{ SDL_Event e={0};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;e.key.scancode=code;e.key.mod=mod;CHECK(SDL_PushEvent(&e)); }
static void Event(SDL_EventType type) { SDL_Event e={0};e.type=type;CHECK(SDL_PushEvent(&e)); }
static void Shortcut(bool full)
{
    if(full) Key(SDL_SCANCODE_LALT,true,SDL_KMOD_LALT);
    Key(full?SDL_SCANCODE_RETURN:SDL_SCANCODE_F11,true,full?SDL_KMOD_LALT:SDL_KMOD_NONE);
    Key(full?SDL_SCANCODE_RETURN:SDL_SCANCODE_F11,false,full?SDL_KMOD_LALT:SDL_KMOD_NONE);
    if(full) Key(SDL_SCANCODE_LALT,false,SDL_KMOD_NONE);
    CHECK(!VL_PollQuit());AwaitDisplay();
}
static void Pixel(SDL_Surface *s,int x,int y,unsigned r,unsigned g,unsigned b)
{
    Uint8 ar,ag,ab,a;CHECK(SDL_ReadSurfacePixel(s,x,y,&ar,&ag,&ab,&a));CHECK(ar==r && ag==g && ab==b && a==255);
}
static void Geometry(void)
{
    uint8_t colors[768]={0};colors[3]=63;colors[7]=63;CHECK(VL_SetPalette(colors,sizeof(colors)));
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) framebuffer[y*320+x]=(uint8_t)(x<160?1:2);
    Pump();uint64_t uploads=VL_TextureUploads();
    const int sizes[][2]={{1000,600},{600,1000},{1920,1080},{640,480},{319,239},{160,120},{13,9},{1,1},{960,720}};
    for(unsigned mode=0;mode<2;++mode) for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0])+15;++i) {
        display_settings s=Settings();s.integer=mode!=0;Set(s);
        const int *size=i<15?presets[i]:sizes[i-15];Resize(size[0],size[1]);
        int w,h;CHECK(SDL_GetRenderOutputSize(renderer,&w,&h));CHECK(w>0 && h>0);
        int pw,ph;CHECK(SDL_GetWindowSizeInPixels(window,&pw,&ph));CHECK(w==pw && h==ph);
        CHECK(SDL_GetWindowFlags(window)&SDL_WINDOW_HIGH_PIXEL_DENSITY);
        SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
        if(w<4 || h<3) continue;
        double scale=fmin((double)w/320,(double)h/240);
        if(mode && scale>=1) scale=floor(scale);
        CHECK(fabs(rect.w-floor(320*scale))<=1 && fabs(rect.h-floor(240*scale))<=1);
        CHECK(rect.x>=0 && rect.y>=0 && rect.x+rect.w<=w+1 && rect.y+rect.h<=h+1);
        CHECK(fabs(rect.w*3-rect.h*4)<=4); /* At most a pixel of fit rounding. */
        /* Disable logical clipping only for full-output pixel readback. */
        SDL_RendererLogicalPresentation saved;int lw,lh;CHECK(SDL_GetRenderLogicalPresentation(renderer,&lw,&lh,&saved));
        CHECK(SDL_SetRenderLogicalPresentation(renderer,0,0,SDL_LOGICAL_PRESENTATION_DISABLED));
        SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);CHECK(surface && surface->w==w && surface->h==h);
        if(rect.x>=1) Pixel(surface,0,h/2,0,0,0);
        if(rect.y>=1) Pixel(surface,w/2,0,0,0,0);
        Pixel(surface,(int)(rect.x+rect.w/4),(int)(rect.y+rect.h/2),255,0,0);
        Pixel(surface,(int)(rect.x+rect.w*3/4),(int)(rect.y+rect.h/2),0,255,0);
        SDL_DestroySurface(surface);CHECK(SDL_SetRenderLogicalPresentation(renderer,lw,lh,saved));
        display_settings got=Settings();CHECK(got.pixel_width==w && got.pixel_height==h);
    }
    CHECK(VL_TextureUploads()==uploads);Resize(1000,600);
    display_settings s=Settings();s.integer=false;Set(s);
    Key(SDL_SCANCODE_F11,true,SDL_KMOD_NONE);Pump();CHECK(Settings().integer);
    SDL_Event repeat={0};repeat.type=SDL_EVENT_KEY_DOWN;repeat.key.scancode=SDL_SCANCODE_F11;repeat.key.repeat=true;
    CHECK(SDL_PushEvent(&repeat));Pump();CHECK(Settings().integer);
    Key(SDL_SCANCODE_F11,false,SDL_KMOD_NONE);Pump();
    CHECK(Settings().integer && !IN_AnyPressed() && !CT_Down(CT_FIRE));
    Shortcut(false);CHECK(!Settings().integer);
    CHECK(!CT_Bind(CT_FIRE,0,SDL_SCANCODE_F11));
    Shortcut(true);CHECK(Settings().fullscreen==((SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0));
    CHECK(Settings().fullscreen && !CT_Down(CT_ALT) && !IN_AnyPressed() && !Keyboard[sc_Enter]);
    Shortcut(true);s=Settings();CHECK(!s.fullscreen && s.width==1000 && s.height==600);
    Key(SDL_SCANCODE_RALT,true,SDL_KMOD_RALT);Pump();CHECK(CT_Down(CT_ALT));
    Key(SDL_SCANCODE_RALT,false,SDL_KMOD_NONE);Pump();IN_ClearKeysDown();
    Key(SDL_SCANCODE_KP_ENTER,true,SDL_KMOD_RALT);Key(SDL_SCANCODE_KP_ENTER,false,SDL_KMOD_RALT);
    Key(SDL_SCANCODE_RALT,false,SDL_KMOD_NONE);Pump();CHECK(Settings().fullscreen && !IN_AnyPressed());Shortcut(true);
    if(SDL_MinimizeWindow(window)) { Pump();CHECK(SDL_RestoreWindow(window));Pump(); }
    else CHECK(*SDL_GetError()); /* Dummy drivers need not implement minimize. */
    s=Settings();s.fullscreen=true;Set(s);s=Settings();s.scale=2;Set(s);
    CHECK(Settings().fullscreen && Settings().width==640);
    s=Settings();s.fullscreen=false;Set(s);CHECK(Settings().width==640 && Settings().height==480);
    Resize(1000,600);
    /* An OS/external change must not leave a stale application fullscreen bit. */
    CHECK(SDL_SetWindowFullscreen(window,true));CHECK(SDL_SyncWindow(window));Pump();CHECK(Settings().fullscreen);
    Shortcut(true);CHECK(!Settings().fullscreen);
    s=Settings();s.fullscreen=true;VL_TestDisplayFailure(1);CHECK(!VL_DisplaySet(&s));CHECK(!Settings().fullscreen && *Settings().error);
    s=Settings();s.integer=true;VL_TestDisplayFailure(2);CHECK(!VL_DisplaySet(&s));CHECK(!Settings().integer);
    s=Settings();s.scale=4;VL_TestDisplayFailure(3);CHECK(!VL_DisplaySet(&s));CHECK(Settings().width==1000);
    s=Settings();s.fullscreen=true;VL_TestDisplayFailure(4);CHECK(VL_DisplaySet(&s) && Settings().pending);
    VL_TestExpireDisplayRequest();Pump();CHECK(!Settings().pending && !Settings().fullscreen && *Settings().error);
    s=Settings();s.fullscreen=s.integer=false;s.scale=3;Set(s);CHECK(!*Settings().error && Settings().width==960);
    Event(SDL_EVENT_WINDOW_FOCUS_LOST);Pump();Shortcut(false);CHECK(!Settings().integer);
    Event(SDL_EVENT_WINDOW_FOCUS_GAINED);Pump();Shortcut(false);CHECK(Settings().integer);Shortcut(false);
    puts("Display geometry, black bars, scaling, tiny windows, fullscreen, shortcuts and failures passed.");
}
static void Step(unsigned n)
{ while(n--) { CHECK(!VL_PollQuit());CHECK(DemoLoop(++ticks));if(GameLoading()) TestFinishLoading(ticks);CHECK(VW_UpdateScreen()); } }
static void Tap(SDL_Scancode code) { Key(code,true,SDL_KMOD_NONE);Key(code,false,SDL_KMOD_NONE);Step(1);AwaitDisplay(); }
static void Capture(const char *name)
{
    const char *dir=getenv("WOLF3D_CAPTURE_DIR");if(!dir) return;
    char p[4096];(void)snprintf(p,sizeof(p),"%s/%s.bmp",dir,name);CHECK(VL_Capture(p));
}
static void Game(void)
{
    Display_Connect(VL_DisplayGet,VL_DisplaySet);CHECK(DemoLoopStartup(0,true));Step(11);
    MainItems.curpos=control;Tap(SDL_SCANCODE_RETURN);Step(21);
    /* Display is reachable directly from the original Control screen. */
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);Step(21);
    CHECK(DemoLoopState()==UI_DISPLAY);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN); /* Window size */
    for(unsigned direction=0;direction<2;++direction) for(unsigned i=1;i<=15;++i) {
        Tap(direction?SDL_SCANCODE_LEFT:SDL_SCANCODE_RIGHT);
        unsigned index=(1+(direction?15-i:i))%15;
        CHECK(Settings().width==presets[index][0] && Settings().height==presets[index][1]);
        CHECK(!Settings().high_resolution && !Settings().integer && Settings().aspect==0);
    }
    Resize(1001,601);Tap(SDL_SCANCODE_LEFT);CHECK(Settings().width==2560 && Settings().height==720);
    Tap(SDL_SCANCODE_RIGHT);CHECK(Settings().width==640 && Settings().height==480);
    Resize(1001,601);Tap(SDL_SCANCODE_RIGHT);CHECK(Settings().width==640 && Settings().height==480);
    Tap(SDL_SCANCODE_RIGHT);CHECK(Settings().width==960 && Settings().height==720);
    Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_CONTROLS);
    Tap(SDL_SCANCODE_UP);Tap(SDL_SCANCODE_RETURN); /* DOS Customize controls -> modern options. */
    for(unsigned n=0;n<6;++n) Tap(SDL_SCANCODE_DOWN);
    Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_DISPLAY);Capture("display-settings");
    Tap(SDL_SCANCODE_RETURN);CHECK(Settings().fullscreen);Tap(SDL_SCANCODE_RETURN);CHECK(!Settings().fullscreen);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);CHECK(Settings().integer);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RIGHT);CHECK(Settings().width==1280 && Settings().height==960);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);CHECK(Settings().high_resolution);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RIGHT);CHECK(Settings().render_width==1280);
    Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_DOWN);Tap(SDL_SCANCODE_RETURN);CHECK(!Settings().integer && Settings().scale==3 && !Settings().high_resolution);
    Tap(SDL_SCANCODE_ESCAPE);Step(21);CT_Reset(CT_MODERN);CHECK(DemoLoopStartGame(ticks));Step(11);CHECK(IN_SDLCaptured());
    display_settings native=Settings();native.high_resolution=true;native.aspect=2;Set(native);CHECK(IN_SDLCaptured());
    native.high_resolution=false;Set(native);CHECK(IN_SDLCaptured());native.high_resolution=true;Set(native);CHECK(IN_SDLCaptured());
    unsigned rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
    Shortcut(true);CHECK(IN_SDLCaptured() && US_RndIndex()==rng && gamestate.TimeCount==time);
    Shortcut(true);CHECK(IN_SDLCaptured() && !CT_Down(CT_ALT) && !CT_Down(CT_FIRE));
    Shortcut(false);CHECK(IN_SDLCaptured());Shortcut(false);
    Event(SDL_EVENT_WINDOW_FOCUS_LOST);Pump();CHECK(!IN_SDLCaptured());
    Event(SDL_EVENT_WINDOW_FOCUS_GAINED);Pump();CHECK(IN_SDLCaptured());
    Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(!IN_SDLCaptured());
    MainItems.curpos=control;Tap(SDL_SCANCODE_RETURN);Step(21);
    Tap(SDL_SCANCODE_DOWN);if(CT_MouseEnabled()) Tap(SDL_SCANCODE_DOWN);
    Tap(SDL_SCANCODE_RETURN); /* DOS Customize controls -> modern options. */
    for(unsigned n=0;n<6;++n) Tap(SDL_SCANCODE_DOWN);
    Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_DISPLAY);
    time=gamestate.TimeCount;Step(70);CHECK(gamestate.TimeCount==time);
    Tap(SDL_SCANCODE_ESCAPE);Step(21);Tap(SDL_SCANCODE_ESCAPE);Step(21);Tap(SDL_SCANCODE_ESCAPE);Step(21);MainItems.curpos=backtodemo;Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(IN_SDLCaptured());
    GameStop();Display_Connect(NULL,NULL);
    CT_Reset(CT_CLASSIC);
    for(unsigned variant=0;variant<2;++variant) {
        CHECK(GameStart(0));
        for(unsigned t=1;t<=60;++t) {
            if(variant && t%10==0) { Shortcut(false);Resize(t%20?1000:960,t%20?600:720); }
            IN_KeyEvent(sc_UpArrow,t<20);IN_KeyEvent(sc_RightArrow,t%13<4);CHECK(GameStep(t));
        }
        CHECK(Save_Write(variant,variant?"display changes":"continuous",true));
    }
    display_settings before=Settings();CHECK(Save_Load(0,60));display_settings after=Settings();
    CHECK(before.fullscreen==after.fullscreen && before.integer==after.integer && before.width==after.width);
    puts("Display menu, capture/focus, pause/resume, simulation/RNG and save compatibility passed.");
}
/* Watch SDL's window events as well as dimensions: even a transient resize
   during a menu transition violates window/presentation independence. */
static unsigned resize_events;
static bool SDLCALL WatchResize(void *unused,SDL_Event *event)
{
    (void)unused;
    if(event->type==SDL_EVENT_WINDOW_RESIZED || event->type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) ++resize_events;
    return true;
}
static void Canvas(bool world,unsigned ratio)
{
    int lw,lh,w,h;SDL_RendererLogicalPresentation mode;
    CHECK(SDL_GetRenderLogicalPresentation(renderer,&lw,&lh,&mode));
    CHECK(SDL_GetRenderOutputSize(renderer,&w,&h));
    if(!world) {
        CHECK(lw==320 && lh==240);
        CHECK(mode==SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    } else {
        static const double ratios[]={0,4.0/3,16.0/9,16.0/10,21.0/9,32.0/9};
        double expected=ratio?ratios[ratio]:fmax(4.0/3,fmin(32.0/9,(double)w/h));
        CHECK(fabs((double)lw/lh-expected)<0.02);
        CHECK(mode==SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
        CHECK(fabs((double)rect.w-w)<=2 || fabs((double)rect.h-h)<=2);
    }
}
static void Transitions(void)
{
    Display_Connect(VL_DisplayGet,VL_DisplaySet);CHECK(DemoLoopStartup(0,true));Step(11);
    CHECK(DemoLoopStartGame(ticks));Step(11);
    const int sizes[][3]={{960,720,1},{960,540,2},{960,600,3},{1680,720,4},{2560,720,5},{1001,601,3}};
    CHECK(SDL_AddEventWatch(WatchResize,NULL));
    for(unsigned i=0;i<6;++i) for(unsigned automatic=0;automatic<2;++automatic) {
        display_settings d=Settings();d.scale=0;d.width=sizes[i][0];d.height=sizes[i][1];
        d.high_resolution=d.integer=true;d.aspect=automatic?0:(unsigned)sizes[i][2];Set(d);
        CHECK(SDL_SyncWindow(window));Pump();
        int w,h;CHECK(SDL_GetWindowSize(window,&w,&h));
        printf("Transition window requested %dx%d, actual %dx%d, aspect %u\n",d.width,d.height,w,h,d.aspect);
        /* WM limits may clamp native windows. Dummy SDL must apply exactly. */
        if(!strcmp(SDL_GetCurrentVideoDriver(),"dummy")) CHECK(w==d.width && h==d.height);
        Canvas(true,d.aspect);resize_events=0;
        Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_MENU);Canvas(false,0);
        MainItems.curpos=control;Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_CONTROLS);Canvas(false,0);
        Tap(SDL_SCANCODE_UP);Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_DISPLAY);Canvas(false,0);
        Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_CONTROLS);
        Tap(SDL_SCANCODE_ESCAPE);Step(21);CHECK(DemoLoopState()==UI_MENU);
        MainItems.curpos=backtodemo;Tap(SDL_SCANCODE_RETURN);Step(21);CHECK(DemoLoopState()==UI_GAME);Canvas(true,d.aspect);
        int after_w,after_h;CHECK(SDL_GetWindowSize(window,&after_w,&after_h));
        CHECK(w==after_w && h==after_h && resize_events==0);
        d=Settings();CHECK(d.width==w && d.height==h && d.integer && d.high_resolution);
        /* Fullscreen keeps the independent windowed dimensions and settings. */
        d.fullscreen=true;Set(d);CHECK(Settings().width==w && Settings().height==h);
        d=Settings();d.fullscreen=false;Set(d);CHECK(Settings().width==w && Settings().height==h);
        Resize(w-1,h-1);CHECK(Settings().width==w-1 && Settings().height==h-1);
    }
    SDL_RemoveEventWatch(WatchResize,NULL);GameStop();Display_Connect(NULL,NULL);
    puts("All aspects, Auto, menu integer scaling, resize-free transitions, manual resize and fullscreen restoration passed.");
}
int main(int argc,char **argv)
{
    CHECK(argc==4);MM_Startup();CHECK(VL_Startup());
    int n;SDL_Window **windows=SDL_GetWindows(&n);CHECK(windows && n==1);window=windows[0];SDL_free(windows);renderer=SDL_GetRenderer(window);CHECK(renderer);
    if(!strcmp(argv[3],"geometry")) Geometry();
    else { CHECK(CA_Startup(argv[1]));CHECK(VL_SetPalette(gamepal,768));CHECK(Save_SetDirectory(argv[2]));if(!strcmp(argv[3],"transitions")) Transitions();else Game();GameStop();CA_Shutdown(); }
    VL_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
