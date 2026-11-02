/* SPDX-License-Identifier: GPL-2.0-only */
/* Fixed-schema TOML preferences, deliberately independent of saved games. */
#include "config_file.h"
#include "sdl_video.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DEF.h"
#include "engine/ID_SD.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

static bool Path(char *dst,size_t cap,const char *dir)
{
    if(!dir || !*dir) return false;
    size_t n=strlen(dir);
    int written=snprintf(dst,cap,"%s%sconfig.toml",dir,dir[n-1]=='/' || dir[n-1]=='\\'?"":"/");
    return written>0 && (size_t)written<cap;
}
static char *Trim(char *s)
{
    while(isspace((unsigned char)*s)) ++s;
    size_t n=strlen(s);
    while(n && isspace((unsigned char)s[n-1])) s[--n]=0;
    return s;
}
static bool Number(const char *s,unsigned *v)
{
    if(!*s || !isdigit((unsigned char)*s)) return false;
    errno=0;char *end;unsigned long x=strtoul(s,&end,10);
    if(errno || *end || x>UINT_MAX) return false;
    *v=(unsigned)x;return true;
}
static bool Bool(const char *s,bool *v)
{
    if(!strcmp(s,"true")) {*v=true;return true;}
    if(!strcmp(s,"false")) {*v=false;return true;}
    return false;
}
void Config_Load(const char *dir)
{
    char path[4096];if(!Path(path,sizeof(path),dir)) return;
    FILE *f=fopen(path,"rb");if(!f) return;
    char line[256],section[32]="";
    ct_preset preset=CT_Preset();
    unsigned sensitivity=CT_Sensitivity(),music=0,effects=0,view=ViewSize();
    bool mouse=CT_MouseEnabled(),invert=CT_Invert(),music_on=false,effects_on=false;
    SD_GetSettings(&music,&effects,&music_on,&effects_on);
    display_settings display;bool have_display=VL_DisplayGet(&display);
    unsigned binding[CT_ACTIONS][2];bool binding_seen[CT_ACTIONS][2]={{false}};
    for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned j=0;j<2;++j) binding[a][j]=CT_Binding((ct_action)a,j);
    while(fgets(line,sizeof(line),f)) {
        if(!strchr(line,'\n') && !feof(f)) { int c;while((c=fgetc(f))!=EOF && c!='\n') {}continue; }
        char *s=Trim(line);if(!*s || *s=='#') continue;
        size_t len=strlen(s);
        if(s[0]=='[' && len>2 && s[len-1]==']') {
            s[len-1]=0;(void)snprintf(section,sizeof(section),"%s",s+1);continue;
        }
        char *eq=strchr(s,'=');if(!eq) continue;*eq++=0;
        char *key=Trim(s),*value=Trim(eq);
        char *comment=strchr(value,'#');if(comment) {*comment=0;value=Trim(value);}
        unsigned n;bool v;
        if(!strcmp(section,"controls")) {
            if(!strcmp(key,"preset")) {
                if(!strcmp(value,"\"modern\"")) preset=CT_MODERN;
                else if(!strcmp(value,"\"classic\"")) preset=CT_CLASSIC;
            } else if(!strcmp(key,"mouse_enabled") && Bool(value,&v)) mouse=v;
            else if(!strcmp(key,"invert_mouse_x") && Bool(value,&v)) invert=v;
            else if(!strcmp(key,"mouse_sensitivity") && Number(value,&n) && n>=1 && n<=40) sensitivity=n;
            else if(!strncmp(key,"bind_",5) && Number(value,&n) && n<CT_KEYS) {
                unsigned action,slot;
                if(sscanf(key+5,"%u_%u",&action,&slot)==2 && action<CT_ACTIONS && slot<2)
                    { binding[action][slot]=n;binding_seen[action][slot]=true; }
            }
        } else if(!strcmp(section,"audio")) {
            if(!strcmp(key,"music_volume") && Number(value,&n) && n<=100) music=n;
            else if(!strcmp(key,"effects_volume") && Number(value,&n) && n<=100) effects=n;
            else if(!strcmp(key,"music_enabled") && Bool(value,&v)) music_on=v;
            else if(!strcmp(key,"effects_enabled") && Bool(value,&v)) effects_on=v;
        } else if(!strcmp(section,"display") && have_display) {
            if(!strcmp(key,"fullscreen") && Bool(value,&v)) display.fullscreen=v;
            else if(!strcmp(key,"integer_scaling") && Bool(value,&v)) display.integer=v;
            else if(!strcmp(key,"window_scale") && Number(value,&n) && n>=1 && n<=5) display.scale=n;
        } else if(!strcmp(section,"classic")) {
            if(!strcmp(key,"view_size") && Number(value,&n) && n>=4 && n<=19) view=n;
        }
    }
    (void)fclose(f);
    CT_Reset(preset);
    for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned j=0;j<2;++j)
        if(binding_seen[a][j] && binding[a][j]!=CT_Binding((ct_action)a,j)) (void)CT_Bind((ct_action)a,j,0);
    for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned j=0;j<2;++j)
        if(binding_seen[a][j] && binding[a][j] && binding[a][j]!=CT_Binding((ct_action)a,j))
            (void)CT_Bind((ct_action)a,j,binding[a][j]);
    CT_SetMouse(mouse);CT_SetSensitivity(sensitivity);CT_SetInvert(invert);
    SD_SetVolumes(music,effects);SD_SetEnabled(music_on,effects_on);
    (void)NewViewSize(view);
    if(have_display) (void)VL_DisplaySet(&display);
}
void Config_Save(const char *dir)
{
    char path[4096];if(!Path(path,sizeof(path),dir)) return;
    FILE *f=fopen(path,"wb");if(!f) return;
    unsigned music=0,effects=0;bool music_on=false,effects_on=false;
    SD_GetSettings(&music,&effects,&music_on,&effects_on);
    (void)fprintf(f,"# Wolf3D Revive preferences (original game data is never modified)\n");
    (void)fprintf(f,"[controls]\npreset = \"%s\"\nmouse_enabled = %s\ninvert_mouse_x = %s\nmouse_sensitivity = %u\n",
        CT_Preset()==CT_MODERN?"modern":"classic",CT_MouseEnabled()?"true":"false",
        CT_Invert()?"true":"false",CT_Sensitivity());
    for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned j=0;j<2;++j)
        (void)fprintf(f,"bind_%u_%u = %u\n",a,j,CT_Binding((ct_action)a,j));
    (void)fprintf(f,"\n[audio]\nmusic_volume = %u\neffects_volume = %u\nmusic_enabled = %s\neffects_enabled = %s\n",
        music,effects,music_on?"true":"false",effects_on?"true":"false");
    display_settings d;if(VL_DisplayGet(&d))
        (void)fprintf(f,"\n[display]\nfullscreen = %s\ninteger_scaling = %s\nwindow_scale = %u\n",
            d.fullscreen?"true":"false",d.integer?"true":"false",d.scale?d.scale:3U);
    (void)fprintf(f,"\n[classic]\nview_size = %u\n",ViewSize());
    (void)fclose(f);
}
