/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform/config_file.h"
#include "platform/save_file.h"
#include "platform/sdl_video.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_MAIN.h"
#include "engine/WL_MENU.h"
#include "engine/WL_DISPLAY.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_CA.h"
#include "engine/ID_SD.h"
#include "engine/ID_US_1.h"
#include "engine/ID_IN.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s; %s)\n",__LINE__,#x,Config_Error(),SDL_GetError());exit(1); } } while(0)
#include "finish_loading.h"
static const char *directory;
static SDL_Window *window;
static void Pump(void) { CHECK(!VL_PollQuit());CHECK(VW_UpdateScreen()); }
static void Await(void)
{
    Uint64 until=SDL_GetTicks()+4000;display_settings d;
    do { Pump();CHECK(VL_DisplayGet(&d));if(!d.pending) break;SDL_Delay(5); } while(SDL_GetTicks()<until);
    CHECK(!d.pending && !*d.error);
}
static void Reset(void)
{
    CT_Reset(CT_MODERN);CT_Reset(CT_CLASSIC);SD_SetVolumes(70,85);SD_SetEnabled(true,true);CHECK(NewViewSize(15));
    display_settings d;CHECK(VL_DisplayGet(&d));d.fullscreen=d.integer=false;d.scale=3;CHECK(VL_DisplaySet(&d));Await();
}
static void Path(char p[4096]) { CHECK(snprintf(p,4096,"%s/config.toml",directory)>0); }
static void Write(const void *data,size_t length)
{ char p[4096];Path(p);FILE *f=fopen(p,"wb");CHECK(f);CHECK(fwrite(data,1,length,f)==length);CHECK(fclose(f)==0); }
static size_t Read(char out[8192])
{ char p[4096];Path(p);FILE *f=SaveFile_Open(p);CHECK(f);size_t n=fread(out,1,8192,f);CHECK(!ferror(f));CHECK(fclose(f)==0);return n; }
static void JSON(config_result result)
{
    unsigned m,e;bool mon,eon;SD_GetSettings(&m,&e,&mon,&eon);display_settings d;CHECK(VL_DisplayPreferences(&d));
    printf("{\"result\":%d,\"preset\":%d,\"music\":%u,\"effects\":%u,\"music_on\":%d,\"effects_on\":%d,\"view\":%u,\"fullscreen\":%d,\"integer\":%d,\"width\":%d,\"height\":%d,\"high_resolution\":%d,\"render_auto\":%d,\"render_width\":%u,\"render_height\":%u,\"aspect\":%u,\"vsync\":%d,\"linear\":%d,\"smooth\":%d,\"profiles\":[",result,(int)CT_Preset(),m,e,mon,eon,ViewSize(),d.fullscreen,d.integer,d.width,d.height,d.high_resolution,d.render_auto,d.render_width,d.render_height,d.aspect,d.vsync,d.linear,d.smooth);
    for(unsigned i=0;i<2;++i) {
        ct_profile p;CT_GetProfile((ct_preset)i,&p);printf("%s{\"mouse\":%d,\"invert\":%d,\"sensitivity\":%u,\"bindings\":[",i?",":"",p.mouse,p.invert,p.sensitivity);
        for(unsigned a=0;a<CT_ACTIONS;++a) printf("%s[%u,%u]",a?",":"",p.binding[a][0],p.binding[a][1]);
        printf("]}");
    }
    puts("]}");
}
static void Seed(void)
{
    ct_profile c,m;CT_DefaultProfile(CT_CLASSIC,&c);CT_DefaultProfile(CT_MODERN,&m);
    c.binding[CT_FORWARD][0]=8;c.binding[CT_BACK][0]=20;c.mouse=true;c.sensitivity=7;
    m.binding[CT_FORWARD][0]=22;m.binding[CT_BACK][0]=26;m.mouse=false;m.invert=true;m.sensitivity=17;
    CHECK(CT_SetProfile(CT_CLASSIC,&c) && CT_SetProfile(CT_MODERN,&m));CT_SelectPreset(CT_MODERN);
    SD_SetVolumes(23,67);SD_SetEnabled(false,true);CHECK(NewViewSize(19));
    display_settings d;CHECK(VL_DisplayGet(&d));d.fullscreen=d.integer=d.smooth=true;d.scale=0;d.width=1000;d.height=600;CHECK(VL_DisplaySet(&d));
}
static void Roundtrip(void)
{
    Seed();CHECK(Config_Save(directory));char first[8192],next[8192];size_t n=Read(first);
    for(unsigned i=0;i<3;++i) {
        VL_Shutdown();CHECK(VL_Startup());SDL_Window **windows;int count;windows=SDL_GetWindows(&count);CHECK(windows && count==1);window=windows[0];SDL_free(windows);
        Reset();CHECK(Config_Load(directory)==CONFIG_LOADED);Await();CHECK(Config_Save(directory));size_t k=Read(next);CHECK(k==n && !memcmp(first,next,n));
        display_settings d;CHECK(VL_DisplayGet(&d));d.fullscreen=false;CHECK(VL_DisplaySet(&d));Await();
        int w,h;CHECK(SDL_GetWindowSize(window,&w,&h) && w==1000 && h==600);
        d.fullscreen=true;CHECK(VL_DisplaySet(&d));Await();
        CHECK(CT_Preset()==CT_MODERN && CT_Binding(CT_FORWARD,0)==22 && !CT_MouseEnabled() && CT_Invert() && CT_Sensitivity()==17);
        CT_SelectPreset(CT_CLASSIC);CHECK(CT_Binding(CT_FORWARD,0)==8 && CT_MouseEnabled() && CT_Sensitivity()==7);
        CT_SelectPreset(CT_MODERN);CHECK(CT_Binding(CT_FORWARD,0)==22);
    }
    JSON(CONFIG_LOADED);
}
static void Atomic(void)
{
    CHECK(Config_Save(directory));char before[8192],after[8192];size_t n=Read(before);SD_SetVolumes(99,1);
    for(unsigned fault=1;fault<=6;++fault) {
        SaveFile_TestFailure(fault);CHECK(!Config_Save(directory));size_t k=Read(after);CHECK(k==n && !memcmp(before,after,n));
    }
    CHECK(Config_Save(directory));CHECK(Config_Load(directory)==CONFIG_LOADED);unsigned m;SD_GetSettings(&m,NULL,NULL,NULL);CHECK(m==99);JSON(CONFIG_LOADED);
}
static void Profiles(void)
{
    CHECK(CT_Bind(CT_FORWARD,0,8));CT_SetMouse(true);CT_SetSensitivity(9);
    CT_SelectPreset(CT_MODERN);CHECK(CT_Binding(CT_FORWARD,0)==26 && CT_MouseEnabled());CHECK(CT_Bind(CT_FORWARD,0,20));CT_SetInvert(true);
    CT_SelectPreset(CT_CLASSIC);CHECK(CT_Binding(CT_FORWARD,0)==8 && CT_Sensitivity()==9);
    CT_SelectPreset(CT_MODERN);CHECK(CT_Binding(CT_FORWARD,0)==20 && CT_Invert());
    CT_Reset(CT_MODERN);CHECK(CT_Binding(CT_FORWARD,0)==26 && !CT_Invert());CT_SelectPreset(CT_CLASSIC);CHECK(CT_Binding(CT_FORWARD,0)==8);
    ct_profile bad;CT_GetProfile(CT_CLASSIC,&bad);bad.binding[CT_FIRE][0]=8;CHECK(!CT_SetProfile(CT_CLASSIC,&bad));CHECK(CT_Binding(CT_FIRE,0)==224);
    CHECK(Config_Save(directory));JSON(CONFIG_LOADED);
}
static void Display(void)
{
    const char *text="[display]\nfullscreen=true\ninteger_scaling=true\nwindow_width=1000\nwindow_height=600\n";Write(text,strlen(text));
    VL_TestDisplayFailure(1);CHECK(Config_Load(directory)==CONFIG_LOADED && *Config_Error());display_settings d;CHECK(VL_DisplayGet(&d) && !d.fullscreen && !d.pending);
    CHECK(Config_Load(directory)==CONFIG_LOADED);Await();CHECK(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN);
    d.fullscreen=false;d.scale=0;d.width=1000;d.height=600;CHECK(VL_DisplaySet(&d));Await();
    VL_TestDisplayFailure(4);CHECK(Config_Load(directory)==CONFIG_LOADED);CHECK(VL_DisplayGet(&d) && d.pending && !d.fullscreen);
    CHECK(Config_Save(directory));char bytes[8192];size_t n=Read(bytes);CHECK(n<sizeof(bytes));bytes[n]=0;CHECK(strstr(bytes,"fullscreen = true"));
    VL_TestExpireDisplayRequest();Pump();CHECK(VL_DisplayGet(&d) && !d.pending && !d.fullscreen);CHECK(Config_Save(directory));n=Read(bytes);CHECK(n<sizeof(bytes));bytes[n]=0;CHECK(strstr(bytes,"fullscreen = false"));
    JSON(CONFIG_LOADED);
}
static void Fuzz(void)
{
    uint32_t random=12345;unsigned char bytes[300];unsigned rng=US_RndIndex();
    for(unsigned trial=0;trial<1000;++trial) {
        size_t n=trial%sizeof(bytes);
        for(size_t i=0;i<n;++i) { random=random*1664525U+1013904223U;bytes[i]=(unsigned char)(random>>24); }
        Write(bytes,n);(void)Config_Load(directory);CHECK(US_RndIndex()==rng);
    }
    JSON(CONFIG_INVALID);
}
static void Game(const char *data)
{
    CHECK(CA_Startup(data));CHECK(Save_SetDirectory(directory));Display_Connect(VL_DisplayGet,VL_DisplaySet);
    CHECK(DemoLoopStartup(0,true));CHECK(DemoLoopStartGame(0));CHECK(DemoLoop(10));TestFinishLoading(10);
    CHECK(Save_Write(0,"before config",true));unsigned rng=US_RndIndex();uint32_t t=gamestate.TimeCount;
    CHECK(Config_Save(directory));CHECK(Config_Load(directory)==CONFIG_LOADED);Await();CHECK(GameDrawPaused());CHECK(Save_Write(1,"after config",true));
    CHECK(US_RndIndex()==rng && gamestate.TimeCount==t);
    IN_KeyEvent(0x3b,true);IN_KeyEvent(0x3b,false);CHECK(DemoLoop(11));CHECK(DemoLoop(31));CHECK(DemoLoopState()==UI_BOSS);
    DemoLoopPrepareExit();unsigned m,e;bool on,fx;SD_GetSettings(&m,&e,&on,&fx);CHECK(on && fx);CHECK(Config_Save(directory));
    GameStop();Display_Connect(NULL,NULL);JSON(CONFIG_LOADED);
}
int main(int argc,char **argv)
{
    CHECK(argc==4);directory=argv[1];MM_Startup();CHECK(VL_Startup());int count;SDL_Window **windows=SDL_GetWindows(&count);CHECK(windows && count==1);window=windows[0];SDL_free(windows);Reset();
    const char *mode=argv[3];
    if(!strcmp(mode,"inspect")) { config_result r=Config_Load(directory);Await();JSON(r); }
    else if(!strcmp(mode,"migrate")) { CHECK(Config_Load(directory)==CONFIG_LOADED);CHECK(Config_Save(directory));JSON(CONFIG_LOADED); }
    else if(!strcmp(mode,"roundtrip")) Roundtrip();
    else if(!strcmp(mode,"seed")) { Seed();CHECK(Config_Save(directory));JSON(CONFIG_LOADED); }
    else if(!strcmp(mode,"atomic")) Atomic();
    else if(!strcmp(mode,"profiles")) Profiles();
    else if(!strcmp(mode,"display")) Display();
    else if(!strcmp(mode,"fuzz")) Fuzz();
    else if(!strcmp(mode,"paths")) {
        CHECK(Config_Load(NULL)==CONFIG_IO_ERROR && !Config_Save(NULL));char longpath[5000];memset(longpath,'a',sizeof(longpath)-1);longpath[sizeof(longpath)-1]=0;
        CHECK(Config_Load(longpath)==CONFIG_IO_ERROR && !Config_Save(longpath));JSON(CONFIG_IO_ERROR);
    }
    else if(!strcmp(mode,"game")) Game(argv[2]);
    else if(!strcmp(mode,"save")) { bool ok=Config_Save(directory);printf("{\"saved\":%d}\n",ok); }
    else if(!strcmp(mode,"prefpath")) { char *p=SDL_GetPrefPath(NULL,"wolf3d-revive");CHECK(p);directory=p;CHECK(Config_Save(p));SDL_free(p);JSON(CONFIG_LOADED); }
    else if(!strcmp(mode,"unicode")) {
        static char path[4096];CHECK(snprintf(path,sizeof(path),"%s/caf\xc3\xa9-\xe8\xa8\xad\xe5\xae\x9a",directory)>0);CHECK(SDL_CreateDirectory(path));directory=path;
        CHECK(Config_Save(path));CHECK(Config_Load(path)==CONFIG_LOADED);JSON(CONFIG_LOADED);
    } else CHECK(false);
    VL_Shutdown();CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());return 0;
}
