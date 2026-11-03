/* SPDX-License-Identifier: GPL-2.0-only */
/* Independent source-composition oracle; all graphics are loaded at runtime. */
#include "engine/WL_MENU.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DISPLAY.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_VL.h"
#include "engine/ID_IN.h"
#include "engine/ID_SD.h"
#include "engine/ID_US_1.h"
#include "platform/sdl_video.h"
#include "platform/config_file.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s / %s)\n",__LINE__,#x,CA_Error(),SDL_GetError());exit(1); } } while(0)
static unsigned char expected[64000];
static void Key(unsigned scan) { IN_KeyEvent(scan,true);IN_KeyEvent(scan,false); }
static void Control(unsigned scan) { Key(scan);CHECK(HandleControlsMenu()==0); }
static void Sound(unsigned scan) { Key(scan);CHECK(HandleSoundSettings()==0); }
static unsigned LE16(const unsigned char *p) { return p[0]+256U*p[1]; }
static void Bar(int x,int y,int w,int h,unsigned char color)
{ for(int row=y;row<y+h;++row) memset(expected+row*320+x,color,(size_t)w); }
static void Panel(int x,int y,int w,int h)
{
    Bar(x,y,w,h,BKGDCOLOR);Bar(x,y,w+1,1,DEACTIVE);Bar(x,y,1,h+1,DEACTIVE);
    Bar(x,y+h,w+1,1,BORD2COLOR);Bar(x+w,y,1,h+1,BORD2COLOR);
}
static void Pic(int x,int y,int chunk)
{
    CHECK(CA_CacheGrChunk(chunk));unsigned w=pictable[chunk-3].width,h=pictable[chunk-3].height;
    CHECK(w%4==0 && grsizes[chunk]==(size_t)w*h);x&=~7;
    const unsigned char *p=grsegs[chunk];
    for(unsigned r=0;r<h;++r) for(unsigned c=0;c<w;++c)
        expected[(y+(int)r)*320+x+(int)c]=p[(c%4)*(w/4)*h+r*(w/4)+c/4];
}
static void FontText(int x,int y,const char *text,unsigned char color,int chunk)
{
    CHECK(CA_CacheGrChunk(chunk));const unsigned char *font=grsegs[chunk];unsigned h=LE16(font);
    for(const unsigned char *s=(const unsigned char *)text;*s;++s) {
        unsigned w=font[514+*s],at=LE16(font+2+*s*2);
        for(unsigned r=0;r<h;++r) for(unsigned c=0;c<w;++c)
            if(font[at+r*w+c]) expected[(y+(int)r)*320+x+(int)c]=color;
        x+=(int)w;
    }
}
static void Text(int x,int y,const char *text,unsigned char color)
{ FontText(x,y,text,color,2); }
static void SoundOracle(void)
{
    memset(expected,BORDCOLOR,sizeof(expected));Pic(112,184,C_MOUSELBACKPIC);
    Panel(40,17,250,45);Panel(40,82,250,45);Panel(40,147,250,32);
    Pic(100,0,C_FXTITLEPIC);Pic(100,65,C_DIGITITLEPIC);Pic(100,130,C_MUSICTITLEPIC);
    const int ys[]={20,33,46,150,163};
    const char *labels[]={"None","AdLib effects","Adjust volumes","None","AdLib music"};
    for(unsigned i=0;i<5;++i) {
        Text(100,ys[i],labels[i],i==0?HIGHLIGHT:TEXTCOLOR);
        if(i!=2) Pic(72,ys[i]+2,(i==1 || i==4)?C_SELECTEDPIC:C_NOTSELECTEDPIC);
    }
    Text(100,85,"With sound effects",TEXTCOLOR);Pic(72,87,C_SELECTEDPIC);
    FontText(80,108,"Original digitized samples",TEXTCOLOR,1);Pic(48,18,C_CURSOR1PIC);
    CHECK(!memcmp(expected,framebuffer,sizeof(expected)));
}
static void ControlOracle(unsigned selected,bool mouse,int cursor)
{
    memset(expected,BORDCOLOR,sizeof(expected));Bar(0,10,320,24,0);Bar(0,32,320,1,STRIPE);
    Pic(80,0,C_CONTROLPIC);Pic(112,184,C_MOUSELBACKPIC);Panel(16,65,284,84);
    static const char *const labels[]={"Mouse Enabled","Joystick Enabled","Use joystick port 2",
        "Gravis GamePad Enabled","Mouse Sensitivity","Customize controls"};
    for(unsigned i=0;i<6;++i) {
        bool active=i==0 || i==5 || (i==4 && mouse);
        Text(80,70+(int)i*13,labels[i],active?(selected==i?HIGHLIGHT:TEXTCOLOR):DEACTIVE);
        if(i<4) Pic(56,73+(int)i*13,i==0 && mouse?C_SELECTEDPIC:C_NOTSELECTEDPIC);
    }
    Pic(24,68+(int)selected*13,cursor);CHECK(!memcmp(expected,framebuffer,sizeof(expected)));
}
static void Capture(const char *dir,const char *name)
{
    char path[4096];CHECK(snprintf(path,sizeof(path),"%s/%s.idx",dir,name)>0);
    FILE *f=fopen(path,"wb");CHECK(f && fwrite(framebuffer,1,64000,f)==64000 && fclose(f)==0);
    CHECK(snprintf(path,sizeof(path),"%s/palette.bin",dir)>0);f=fopen(path,"wb");
    CHECK(f && fwrite(gamepal,1,768,f)==768 && fclose(f)==0);
}
static void Menus(const char *directory)
{
    CHECK(SetupControlPanel(0));Capture(directory,"main");
    CT_Reset(CT_CLASSIC);ResetMenuCursor(0);CHECK(BeginControlsMenu());ControlOracle(0,false,C_CURSOR1PIC);Capture(directory,"control-off");
    Control(sc_DownArrow);ControlOracle(5,false,C_CURSOR1PIC); /* All unavailable device rows skipped. */
    Control(sc_UpArrow);Control(sc_Enter);ControlOracle(0,true,C_CURSOR1PIC);
    Control(sc_DownArrow);ControlOracle(4,true,C_CURSOR1PIC);Control(sc_Enter);
    unsigned sensitivity=CT_Sensitivity();Control(sc_RightArrow);CHECK(CT_Sensitivity()==sensitivity+1);
    Control(sc_Escape);Control(sc_DownArrow);Control(sc_Enter);Capture(directory,"customize");
    Control(sc_Enter);CHECK(CT_Preset()==CT_MODERN);
    for(unsigned i=0;i<3;++i) Control(sc_DownArrow);
    Control(sc_Enter);CHECK(CT_Invert());
    Key(sc_Escape);CHECK(HandleControlsMenu()==0);ControlOracle(5,CT_MouseEnabled(),C_CURSOR1PIC);
    CHECK(BeginControlsMenu());ResetMenuCursor(0);CHECK(!TickMenuCursor(8));CHECK(TickMenuCursor(9));
    CHECK(DrawControlsMenu());ControlOracle(0,true,C_CURSOR2PIC);
    CHECK(TickMenuCursor(18));CHECK(DrawControlsMenu());ControlOracle(0,true,C_CURSOR1PIC);Capture(directory,"control-on");
    unsigned char fine[64000];for(uint64_t t=19;t<1000;++t) (void)TickMenuCursor(t);
    CHECK(DrawControlsMenu());memcpy(fine,framebuffer,sizeof(fine));
    ResetMenuCursor(0);CHECK(TickMenuCursor(999));CHECK(DrawControlsMenu());CHECK(!memcmp(fine,framebuffer,sizeof(fine)));
    SD_SetEnabled(true,true);SD_SetDigitizedMode(true);CHECK(BeginSoundSettings());Capture(directory,"sound");
    SoundOracle();
    Sound(sc_Enter);unsigned m,e;bool mon,eon;SD_GetSettings(&m,&e,&mon,&eon);CHECK(mon && !eon);
    Sound(sc_DownArrow);Sound(sc_Enter);Sound(sc_DownArrow);Sound(sc_Enter);Capture(directory,"volumes");
    SD_SetVolumes(70,85);Sound(sc_LeftArrow);Sound(sc_DownArrow);Sound(sc_RightArrow);
    SD_GetSettings(&m,&e,&mon,&eon);CHECK(m==65 && e==90);
    Sound(sc_Escape);Sound(sc_DownArrow);Sound(sc_Enter);SD_GetSettings(&m,&e,&mon,&eon);CHECK(!mon && eon);
    CHECK(Config_Save(directory));CT_Reset(CT_CLASSIC);SD_SetEnabled(true,false);SD_SetVolumes(0,0);
    CHECK(Config_Load(directory)==CONFIG_LOADED && CT_Preset()==CT_MODERN && CT_Invert());
    SD_GetSettings(&m,&e,&mon,&eon);CHECK(m==65 && e==90 && !mon && eon);
    CT_SelectPreset(CT_CLASSIC);CHECK(CT_Sensitivity()==sensitivity+1 && CT_MouseEnabled());
    CT_SelectPreset(CT_MODERN);
}
static SDL_Renderer *renderer;
static void Geometry(int w,int h)
{
    display_settings d;CHECK(VL_DisplayGet(&d));d.scale=0;d.width=w;d.height=h;d.integer=false;
    CHECK(VL_DisplaySet(&d) && !VL_PollQuit() && VW_UpdateScreen());
    SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
    double expected_w=h*4.0/3.0,expected_h=h;
    if(expected_w>w) { expected_w=w;expected_h=w*3.0/4.0; }
    CHECK(fabs(rect.w-expected_w)<=1 && fabs(rect.h-expected_h)<=1);
    CHECK(fabs(rect.x-(w-expected_w)/2)<=1 && fabs(rect.y-(h-expected_h)/2)<=1);
    int lw,lh;SDL_RendererLogicalPresentation mode;CHECK(SDL_GetRenderLogicalPresentation(renderer,&lw,&lh,&mode));
    CHECK(lw==320 && lh==240 && mode==SDL_LOGICAL_PRESENTATION_LETTERBOX);
    if(w==1920 && h==1200) CHECK(rect.x==160 && rect.y==0 && rect.w==1600 && rect.h==1200);
    CHECK(SDL_SetRenderLogicalPresentation(renderer,0,0,SDL_LOGICAL_PRESENTATION_DISABLED));
    SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);CHECK(surface && surface->w==w && surface->h==h);
    if(rect.x>=1) for(unsigned side=0;side<2;++side) {
        Uint8 r,g,b,a;CHECK(SDL_ReadSurfacePixel(surface,side?w-1:0,h/2,&r,&g,&b,&a));
        CHECK(r==0 && g==0 && b==0 && a==255);
    }
    SDL_DestroySurface(surface);CHECK(SDL_SetRenderLogicalPresentation(renderer,lw,lh,mode));
}
static void Presentation(const char *directory)
{
    CHECK(DemoLoopStartup(0,true));CHECK(DemoLoopStartGame(0));CHECK(DemoLoop(11));
    display_settings d;CHECK(VL_DisplayGet(&d));d.high_resolution=true;d.render_width=1280;d.render_height=720;d.aspect=2;
    CHECK(VL_DisplaySet(&d) && VW_UpdateScreen());
    unsigned rng=US_RndIndex();uint32_t clock=gamestate.TimeCount;
    CHECK(Save_Write(0,"before menus",true));
    Key(sc_Escape);CHECK(DemoLoop(12) && DemoLoop(32));CHECK(DemoLoopState()==UI_MENU);
    const int sizes[][2]={{1920,1200},{1920,1080},{3440,1440},{3840,1080}};
    for(unsigned n=0;n<4;++n) {
        Geometry(sizes[n][0],sizes[n][1]);
        MainItems.curpos=control;Key(sc_Enter);CHECK(DemoLoop(33+n*50) && DemoLoop(53+n*50));
        CHECK(DemoLoopState()==UI_CONTROLS);CHECK(VW_UpdateScreen());Geometry(sizes[n][0],sizes[n][1]);
        Key(sc_Escape);CHECK(DemoLoop(54+n*50) && DemoLoop(74+n*50));CHECK(DemoLoopState()==UI_MENU);
    }
    MainMenu[soundmenu].active=1;MainItems.curpos=soundmenu;Key(sc_Enter);
    CHECK(DemoLoop(225) && DemoLoop(245) && DemoLoopState()==UI_SOUND);Geometry(1920,1200);
    Key(sc_Escape);CHECK(DemoLoop(246) && DemoLoop(266) && DemoLoopState()==UI_MENU);
    CHECK(US_RndIndex()==rng && gamestate.TimeCount==clock && Save_Write(1,"after menus",true));
    CHECK(!VL_PollQuit());CHECK(VL_DisplayGet(&d));d.fullscreen=true;CHECK(VL_DisplaySet(&d));
    CHECK(!VL_PollQuit() && VW_UpdateScreen());CHECK(VL_DisplayGet(&d) && d.fullscreen);d.fullscreen=false;
    CHECK(VL_DisplaySet(&d) && !VL_PollQuit() && VW_UpdateScreen());Geometry(1920,1200);
    MainItems.curpos=backtodemo;Key(sc_Enter);CHECK(DemoLoop(300) && DemoLoop(320));CHECK(DemoLoopState()==UI_GAME);
    CHECK(VW_UpdateScreen());SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));CHECK(fabs(rect.w/rect.h-16.0/9)<0.002);
    GameStop();CHECK(DemoLoopStartup(0,false));bool available=CA_GrChunkPresent(PG13PIC);
    if(available) {
        unsigned width=pictable[PG13PIC-STARTPICS].width;
        pictable[PG13PIC-STARTPICS].width=80;
        CHECK(DemoLoopIntro(0) && DemoLoopState()==UI_TITLE);
        pictable[PG13PIC-STARTPICS].width=(w3r_word)width;
    }
    CHECK(DemoLoopIntro(0));
    if(available) {
        CHECK(DemoLoopState()==UI_RATING);memset(expected,0x82,sizeof(expected));Pic(216,110,PG13PIC);
        CHECK(!memcmp(expected,framebuffer,sizeof(expected)));Capture(directory,"pg13");Geometry(1920,1200);
        CHECK(DemoLoop(30) && DemoLoop(519));CHECK(DemoLoopState()==UI_RATING);
        CHECK(DemoLoop(520) && DemoLoop(550) && DemoLoopState()==UI_TITLE);
        CHECK(DemoLoopStartup(0,false) && DemoLoopIntro(0) && DemoLoop(30));Key(sc_Space);
        CHECK(DemoLoop(31) && DemoLoop(61) && DemoLoopState()==UI_TITLE);
    } else CHECK(DemoLoopState()==UI_TITLE);
    CHECK(DemoLoopStartup(0,false));CHECK(CA_CacheScreen(TITLEPIC));Capture(directory,"title");Geometry(1920,1200);
    CHECK(CA_CacheScreen(CREDITSPIC));Capture(directory,"credits");Geometry(3440,1440);
}
static void StaticGameScreens(void)
{
    CHECK(DemoLoopStartGame(1000) && DemoLoop(1011));
    playstate=ex_completed;CHECK(GameStep(1012) && GameIntermission());
    uint32_t clock=gamestate.TimeCount;unsigned rng=US_RndIndex();
    unsigned char frame[64000];memcpy(frame,framebuffer,sizeof(frame));
    Geometry(1920,1200);CHECK(!memcmp(frame,framebuffer,sizeof(frame)));
    CHECK(gamestate.TimeCount==clock && US_RndIndex()==rng);
    CHECK(DemoLoopStartGame(1100) && DemoLoop(1111));
    playstate=ex_victorious;CHECK(GameStep(1112) && GameVictoryPresentation());
    Geometry(1920,1200);Key(sc_Enter);CHECK(GameStep(1113));
    CHECK(GameVictoryPresentation());memcpy(frame,framebuffer,sizeof(frame));
    clock=gamestate.TimeCount;rng=US_RndIndex();Geometry(3440,1440);
    CHECK(!memcmp(frame,framebuffer,sizeof(frame)) && gamestate.TimeCount==clock && US_RndIndex()==rng);
    CHECK(DemoLoopStartGame(1200) && DemoLoop(1211) && VW_UpdateScreen());
    SDL_FRect rect;CHECK(SDL_GetRenderLogicalPresentationRect(renderer,&rect));
    CHECK(fabs(rect.w/rect.h-16.0/9)<0.002);
}
int main(int argc,char **argv)
{
    CHECK(argc==3);MM_Startup();CHECK(CA_Startup(argv[1]) && VL_SetPalette(gamepal,768) && VL_Startup());
    int n;SDL_Window **windows=SDL_GetWindows(&n);CHECK(windows && n==1);renderer=SDL_GetRenderer(windows[0]);SDL_free(windows);
    Display_Connect(VL_DisplayGet,VL_DisplaySet);CHECK(Save_SetDirectory(argv[2]));Menus(argv[2]);Presentation(argv[2]);StaticGameScreens();
    Display_Connect(NULL,NULL);GameStop();VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("DOS control composition, cursor, supported sound, persistence, intro, 4:3 menus and widescreen return passed.");return 0;
}
