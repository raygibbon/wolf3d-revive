/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_MAIN.h"
#include "engine/WL_DEF.h"
#include "engine/WL_SAVE.h"
#include "engine/WL_DISPLAY.h"
#include "engine/ID_CA.h"
#include "engine/ID_IN.h"
#include "engine/ID_US_1.h"
#include "engine/ID_SD.h"
#include "engine/ID_VH.h"
#include "platform/sdl_video.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s / %s)\n",__LINE__,#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
static SDL_Window *window;
static SDL_Renderer *renderer;
static unsigned resizes;
static unsigned char raw[64000],expected[64000],before[64000];
static display_settings Settings(void) { display_settings d;CHECK(VL_DisplayGet(&d));return d; }
static void Pump(void) { CHECK(!VL_PollQuit() && VW_UpdateScreen()); }
static void Set(display_settings d)
{
    CHECK(VL_DisplaySet(&d));Uint64 deadline=SDL_GetTicks()+4000;
    do { CHECK(SDL_SyncWindow(window));Pump();if(!Settings().pending) break;SDL_Delay(5); } while(SDL_GetTicks()<deadline);
    CHECK(!Settings().pending);
}
static bool SDLCALL Watch(void *unused,SDL_Event *event)
{
    (void)unused;
    if(event->type==SDL_EVENT_WINDOW_RESIZED || event->type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) ++resizes;
    return true;
}
static void Key(bool down,bool repeat)
{
    SDL_Event event={0};event.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    event.key.scancode=SDL_SCANCODE_SPACE;event.key.repeat=repeat;CHECK(SDL_PushEvent(&event));CHECK(!VL_PollQuit());
}
/* Independent font composition oracle, matching DOS FinishSignon's coordinates. */
static void Prompt(const char *text,unsigned char color)
{
    memcpy(expected,raw,sizeof(raw));
    for(unsigned y=189;y<200;++y) memset(expected+y*320,raw[0],300);
    CHECK(CA_CacheGrChunk(STARTFONT));const unsigned char *font=grsegs[STARTFONT];
    unsigned height=font[0]+256U*font[1],width=0;
    for(const unsigned char *s=(const unsigned char *)text;*s;++s) width+=font[514+*s];
    CHECK(width<=320 && height<=10);unsigned x=(320-width)/2;
    for(const unsigned char *s=(const unsigned char *)text;*s;++s) {
        unsigned w=font[514+*s],at=font[2+*s*2]+256U*font[3+*s*2];
        for(unsigned y=0;y<height;++y) for(unsigned c=0;c<w;++c)
            if(font[at+y*w+c]) expected[(190+y)*320+x+c]=color;
        x+=w;
    }
    CHECK(!memcmp(expected,framebuffer,sizeof(expected)));
}
static void Capture(const char *directory,const char *name)
{
    char path[4096];CHECK(snprintf(path,sizeof(path),"%s/%s.bmp",directory,name)>0);CHECK(VL_Capture(path));
    SDL_FRect rect;int w,h;CHECK(SDL_GetRenderOutputSize(renderer,&w,&h));
    CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
    double scale=fmin((double)w/320,(double)h/240);
    CHECK(fabs(rect.w-320*scale)<=1 && fabs(rect.h-240*scale)<=1);
    CHECK(fabs(rect.x-((double)w-rect.w)/2)<=1 && fabs(rect.y-((double)h-rect.h)/2)<=1);
    int lw,lh;SDL_RendererLogicalPresentation mode;
    CHECK(SDL_GetRenderLogicalPresentation(renderer,&lw,&lh,&mode));
    CHECK(lw==320 && lh==240 && mode==SDL_LOGICAL_PRESENTATION_LETTERBOX);
    CHECK(SDL_SetRenderLogicalPresentation(renderer,0,0,SDL_LOGICAL_PRESENTATION_DISABLED));
    SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);CHECK(surface && surface->w==w && surface->h==h);
    bool exact=w==1920 && h==1200;
    if(exact) CHECK(rect.x==160 && rect.y==0 && rect.w==1600 && rect.h==1200);
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        bool border=x<(int)floor(rect.x) || x>=(int)ceil(rect.x+rect.w) ||
            y<(int)floor(rect.y) || y>=(int)ceil(rect.y+rect.h);
        if(!border && !exact) continue;
        Uint8 r,g,b,a;CHECK(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a) && a==255);
        if(border) CHECK(r==0 && g==0 && b==0);
        else {
            unsigned index=framebuffer[(y/6)*320+(x-160)/5];unsigned char colors[3];
            for(unsigned c=0;c<3;++c) { unsigned v=gamepal[index*3+c];colors[c]=(unsigned char)((v<<2)|(v>>4)); }
            CHECK(r==colors[0] && g==colors[1] && b==colors[2]);
        }
    }
    CHECK(SDL_SaveBMP(surface,path));SDL_DestroySurface(surface);CHECK(SDL_SetRenderLogicalPresentation(renderer,lw,lh,mode));
    printf("%s: %dx%d output, %.2fx%.2f centred DOS canvas; black borders verified%s\n",
        name,w,h,(double)rect.w,(double)rect.h,exact?", every 5x6 source texel matches":"");
}
int main(int argc,char **argv)
{
    CHECK(argc==5);MM_Startup();CHECK(CA_Startup(argv[1]) && SD_Startup(48000) && VL_Startup() && VL_SetPalette(gamepal,768));
    int count;SDL_Window **windows=SDL_GetWindows(&count);CHECK(windows && count==1);window=windows[0];SDL_free(windows);
    renderer=SDL_GetRenderer(window);CHECK(renderer);CHECK(SDL_AddEventWatch(Watch,NULL));
    CHECK(DemoLoopStartup(0,false) && DemoLoopIntro(0));ui_state next=DemoLoopState();
    CHECK(next==UI_TITLE || next==UI_RATING);
    unsigned rng=US_RndIndex();uint32_t time=gamestate.TimeCount;
    memcpy(before,framebuffer,sizeof(before));unsigned char saved_palette[768];memcpy(saved_palette,palette,768);
    if(!strcmp(argv[4],"missing") || !strcmp(argv[4],"reject")) {
        bool ok=DemoLoopSignon(argv[3],0);
        if(!strcmp(argv[4],"missing")) CHECK(ok);else CHECK(!ok && strstr(CA_Error(),"Sign-on image"));
        CHECK(DemoLoopState()==next && !memcmp(before,framebuffer,sizeof(before)) && !memcmp(saved_palette,palette,768));
    } else {
        FILE *file=fopen(argv[3],"rb");CHECK(file && fread(raw,1,sizeof(raw),file)==sizeof(raw));CHECK(fclose(file)==0);
        IN_KeyEvent(sc_Enter,true);CHECK(DemoLoopSignon(argv[3],0) && DemoLoopState()==UI_SIGNON && !IN_AnyPressed());
        Prompt("Press a key",14);CHECK(DemoLoop(10) && !memcmp(palette,gamepal,768));
        CHECK(DemoLoop(10000) && DemoLoopState()==UI_SIGNON);sd_status audio;SD_GetStatus(&audio);CHECK(audio.music==-1);
        const int sizes[][2]={{640,480},{1920,1200},{1920,1080},{1280,800},{3440,1440},{3840,1080}};
        bool native=!strcmp(argv[4],"native");
        for(unsigned i=0;i<(native?1U:6U);++i) {
            display_settings d=Settings();d.scale=0;d.width=native?1920:sizes[i][0];d.height=native?1200:sizes[i][1];
            d.fullscreen=native;d.integer=false;d.high_resolution=true;d.aspect=5;Set(d);
            CHECK(!memcmp(palette,gamepal,768));Prompt("Press a key",14);
            char name[32];CHECK(snprintf(name,sizeof(name),"signon-%u",i)>0);Capture(argv[2],name);
        }
        int width,height;CHECK(SDL_GetWindowSize(window,&width,&height));resizes=0;
        Key(true,false);CHECK(DemoLoop(10001));Prompt("Working...",10);CHECK(!IN_AnyPressed());
        CHECK(DemoLoop(10011) && DemoLoopState()==next);
        for(unsigned c=0;c<768;++c) CHECK(palette[c]==0); /* Fade out to black, not menu red. */
        Key(true,true);CHECK(DemoLoop(10041) && DemoLoopState()==next && !IN_AnyPressed());Key(false,false);
        CHECK(!Keyboard[sc_Space] && !IN_NextASCII());SD_GetStatus(&audio);CHECK(audio.music==7);
        int after_w,after_h;CHECK(SDL_GetWindowSize(window,&after_w,&after_h));CHECK(width==after_w && height==after_h && resizes==0);
        if(next==UI_RATING) {
            CHECK(DemoLoop(10530) && DemoLoopState()==UI_RATING);
            CHECK(DemoLoop(10531) && DemoLoop(10561) && DemoLoopState()==UI_TITLE);
        }
        CHECK(US_RndIndex()==rng && gamestate.TimeCount==time && !GameRunning());
        /* Optional loader must not insert the historical page into menus. */
        CHECK(DemoLoopStartup(11000,true));memcpy(before,framebuffer,sizeof(before));
        CHECK(DemoLoopSignon(argv[3],11000) && DemoLoopState()==UI_MENU && !memcmp(before,framebuffer,sizeof(before)));
        CHECK(DemoLoopStartGame(11000));memcpy(before,framebuffer,sizeof(before));
        CHECK(DemoLoopSignon(argv[3],11000) && DemoLoopState()==UI_GAME && !memcmp(before,framebuffer,sizeof(before)));GameStop();
    }
    SDL_RemoveEventWatch(Watch,NULL);VL_Shutdown();SD_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("Sign-on pixels, palette, prompt, startup order, input isolation and timing passed.");return 0;
}
