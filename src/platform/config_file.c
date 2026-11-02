/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded fixed-schema TOML subset. Parse completely before changing settings;
   output uses the shared same-directory, synced atomic replacement primitive. */
#include "config_file.h"
#include "save_file.h"
#include "sdl_video.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_DEF.h"
#include "engine/WL_RENDER.h"
#include "engine/ID_SD.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#define MAX_FILE 65536U
#define MAX_RECORDS 256U
#define NAME_SIZE 64U
static char error[256];
const char *Config_Error(void) { return error; }
static bool Error(const char *message) { (void)snprintf(error,sizeof(error),"%s",message);return false; }
static void Warn(const char *section,const char *key)
{ if(!*error) (void)snprintf(error,sizeof(error),"Ignored unsupported/conflicting %s.%s",section,key); }
static bool Path(char dst[4096],const char *dir)
{
    if(!dir || !*dir) return Error("Configuration directory unavailable");
    size_t n=strlen(dir);if(n>4000) return Error("Configuration path too long");
    int written=snprintf(dst,4096,"%s%sconfig.toml",dir,dir[n-1]=='/' || dir[n-1]=='\\'?"":"/");
    return (written>0 && written<4050) || Error("Configuration path too long");
}
typedef enum { INTEGER,BOOLEAN,STRING } value_type;
typedef struct { char section[NAME_SIZE],key[NAME_SIZE],text[NAME_SIZE];value_type type;int64_t number; } record;
static char *Space(char *s) { while(*s==' ' || *s=='\t') ++s;return s; }
static bool End(char *s) { s=Space(s);return !*s || *s=='#'; }
static bool Bare(char **source,char name[NAME_SIZE])
{
    char *s=*source;size_t n=0;
    while((*s>='a' && *s<='z') || (*s>='A' && *s<='Z') || (*s>='0' && *s<='9') || *s=='_' || *s=='-') {
        if(n==NAME_SIZE-1) return false;
        name[n++]=*s++;
    }
    name[n]=0;*source=s;return n!=0;
}
static bool UTF8(const unsigned char *s,size_t length)
{
    for(size_t i=0;i<length;) {
        unsigned c=s[i++],need,value,minimum;
        if(c<128) continue;
        if(c>=0xc2 && c<=0xdf) { need=1;value=c&31U;minimum=128; }
        else if(c>=0xe0 && c<=0xef) { need=2;value=c&15U;minimum=2048; }
        else if(c>=0xf0 && c<=0xf4) { need=3;value=c&7U;minimum=65536; }
        else return false;
        if(need>length-i) return false;
        while(need--) { c=s[i++];if((c&0xc0U)!=0x80) return false;value=(value<<6)|(c&63U); }
        if(value<minimum || value>0x10ffff || (value>=0xd800 && value<=0xdfff)) return false;
    }
    return true;
}
static int Hex(unsigned c) { return c>='0' && c<='9'?(int)(c-'0'):c>='a' && c<='f'?(int)(c-'a'+10):c>='A' && c<='F'?(int)(c-'A'+10):-1; }
static bool String(char **source,char out[NAME_SIZE])
{
    char *s=*source,quote=*s++;size_t n=0;
    while(*s && *s!=quote) {
        unsigned c=(unsigned char)*s++;
        if(c<32 || c==127) return false;
        if(c=='\\' && quote=='"') {
            c=(unsigned char)*s++;if(!c) return false;
            const char *esc="\"\\bfnrt",*replacement="\"\\\b\f\n\r\t",*at=strchr(esc,(int)c);
            if(at) c=(unsigned char)replacement[at-esc];
            else if(c=='u' || c=='U') {
                unsigned count=c=='u'?4U:8U,value=0;
                for(unsigned i=0;i<count;++i) { int digit=Hex((unsigned char)*s);if(digit<0) return false;value=value*16+(unsigned)digit;++s; }
                if(value>0x10ffff || (value>=0xd800 && value<=0xdfff) || !value) return false;
                unsigned bytes=value<128?1U:value<2048?2U:value<65536?3U:4U;
                if(n+bytes>=NAME_SIZE) return false;
                if(bytes==1) out[n++]=(char)value;
                else {
                    out[n++]=(char)((bytes==2?0xc0U:bytes==3?0xe0U:0xf0U)|(value>>(6*(bytes-1))));
                    for(unsigned j=bytes-1;j>0;--j) out[n++]=(char)(0x80U|((value>>(6*(j-1)))&63U));
                }
                continue;
            } else return false;
        }
        if(n==NAME_SIZE-1) return false;
        out[n++]=(char)c;
    }
    if(*s!=quote) return false;
    out[n]=0;*source=s+1;return true;
}
static bool Integer(char **source,int64_t *number)
{
    char *s=*source;bool negative=false,sign=*s=='+' || *s=='-';
    if(sign) negative=*s++=='-';
    unsigned base=10;
    if(s[0]=='0' && (s[1]=='x' || s[1]=='o' || s[1]=='b')) {
        if(sign) return false;
        base=s[1]=='x'?16U:s[1]=='o'?8U:2U;s+=2;
    }
    bool digit_seen=false,underscore=false,leading_zero=*s=='0';unsigned digits=0;
    uint64_t value=0,limit=negative?UINT64_C(9223372036854775808):INT64_MAX;
    for(;;) {
        if(*s=='_') { if(!digit_seen || underscore) return false;underscore=true;++s;continue; }
        int digit=Hex((unsigned char)*s);if(digit<0 || (unsigned)digit>=base) break;
        if(value>(limit-(unsigned)digit)/base) return false;
        value=value*base+(unsigned)digit;++s;++digits;digit_seen=true;underscore=false;
    }
    if(!digit_seen || underscore || (base==10 && leading_zero && digits>1)) return false;
    *number=negative?(value==UINT64_C(9223372036854775808)?INT64_MIN:-(int64_t)value):(int64_t)value;
    *source=s;return true;
}
static bool Value(char *s,record *r)
{
    if(*s=='"' || *s=='\'') { r->type=STRING;return String(&s,r->text) && End(s); }
    if(!strncmp(s,"true",4) && End(s+4)) { r->type=BOOLEAN;r->number=1;return true; }
    if(!strncmp(s,"false",5) && End(s+5)) { r->type=BOOLEAN;r->number=0;return true; }
    r->type=INTEGER;return Integer(&s,&r->number) && End(s);
}
static bool Parse(char *data,record records[MAX_RECORDS],unsigned *count)
{
    char section[NAME_SIZE]="",tables[64][NAME_SIZE];unsigned ntables=0,line=0;
    for(char *s=data;*s;) {
        char *next=strchr(s,'\n');if(next) *next++=0;
        size_t length=strlen(s);if(length && s[length-1]=='\r') s[--length]=0;
        ++line;if(length>1024) goto invalid;
        for(size_t i=0;i<length;++i) if(((unsigned char)s[i]<32 && s[i]!='\t') || (unsigned char)s[i]==127) goto invalid;
        s=Space(s);
        if(!*s || *s=='#') { s=next?next:s+strlen(s);continue; }
        if(*s=='[') {
            ++s;s=Space(s);if(!Bare(&s,section)) goto invalid;
            s=Space(s);if(*s++!=']' || !End(s) || ntables==64) goto invalid;
            for(unsigned i=0;i<ntables;++i) if(!strcmp(tables[i],section)) goto invalid;
            for(unsigned i=0;i<*count;++i) if(!*records[i].section && !strcmp(records[i].key,section)) goto invalid;
            (void)strcpy(tables[ntables++],section);
        } else {
            if(*count==MAX_RECORDS) goto invalid;
            record *r=&records[*count];(void)strcpy(r->section,section);
            if(!Bare(&s,r->key)) goto invalid;
            s=Space(s);if(*s++!='=' || !Value(Space(s),r)) goto invalid;
            for(unsigned i=0;i<*count;++i) if(!strcmp(records[i].section,section) && !strcmp(records[i].key,r->key)) goto invalid;
            ++*count;
        }
        s=next?next:s+strlen(s);
    }
    return true;
invalid:
    (void)snprintf(error,sizeof(error),"Malformed/unsupported TOML at line %u; configuration unchanged",line);return false;
}
static const record *Find(const record *records,unsigned count,const char *section,const char *key)
{ for(unsigned i=0;i<count;++i) if(!strcmp(records[i].section,section) && !strcmp(records[i].key,key)) return &records[i];return NULL; }
static unsigned Number(const record *r,unsigned fallback,unsigned low,unsigned high)
{
    if(!r) return fallback;
    if(r->type==INTEGER && r->number>=(int64_t)low && r->number<=(int64_t)high) return (unsigned)r->number;
    Warn(r->section,r->key);return fallback;
}
static bool Bool(const record *r,bool fallback)
{ if(!r) return fallback;if(r->type==BOOLEAN) return r->number!=0;Warn(r->section,r->key);return fallback; }
static bool BindingKey(const char *key,unsigned *action,unsigned *slot)
{
    if(strncmp(key,"bind_",5)) return false;
    const char *s=key+5;unsigned a=0;
    if(*s<'0' || *s>'9' || (*s=='0' && s[1]>='0' && s[1]<='9')) return false;
    while(*s>='0' && *s<='9') { unsigned d=(unsigned)(*s++-'0');if(a>(CT_ACTIONS-1-d)/10 || d>=CT_ACTIONS) return false;a=a*10+d; }
    if(*s++!='_' || (*s!='0' && *s!='1') || s[1]) return false;
    *action=a;*slot=(unsigned)(*s-'0');return a<CT_ACTIONS;
}
static void Profile(const record *records,unsigned count,const char *section,ct_profile *p,bool *invalid)
{
    p->mouse=Bool(Find(records,count,section,"mouse_enabled"),p->mouse);
    p->invert=Bool(Find(records,count,section,"invert_mouse_x"),p->invert);
    p->sensitivity=Number(Find(records,count,section,"mouse_sensitivity"),p->sensitivity,1,40);
    for(unsigned i=0;i<count;++i) {
        const record *r=&records[i];if(strcmp(r->section,section) || strncmp(r->key,"bind_",5)) continue;
        unsigned a,slot;
        if(!BindingKey(r->key,&a,&slot) || r->type!=INTEGER || r->number<0 || r->number>=CT_KEYS ||
           (r->number && !*SDL_GetScancodeName((SDL_Scancode)r->number))) { *invalid=true;continue; }
        p->binding[a][slot]=(unsigned)r->number;
    }
}
static void Apply(const record *records,unsigned count)
{
    ct_preset preset=CT_CLASSIC;const record *r=Find(records,count,"controls","preset");
    if(r) { if(r->type==STRING && !strcmp(r->text,"modern")) preset=CT_MODERN;
        else if(r->type!=STRING || strcmp(r->text,"classic")) Warn("controls","preset"); }
    ct_profile profiles[2];
    for(unsigned i=0;i<2;++i) {
        ct_profile defaults;CT_DefaultProfile((ct_preset)i,&defaults);profiles[i]=defaults;bool invalid=false;
        const char *section=i==0?"classic_controls":"modern_controls";Profile(records,count,section,&profiles[i],&invalid);
        if(i==(unsigned)preset) Profile(records,count,"controls",&profiles[i],&invalid); /* Legacy active-profile keys. */
        if(invalid || !CT_ValidProfile(&profiles[i])) { memcpy(profiles[i].binding,defaults.binding,sizeof(defaults.binding));Warn(section,"bindings"); }
        (void)CT_SetProfile((ct_preset)i,&profiles[i]);
    }
    CT_SelectPreset(preset);
    unsigned music=Number(Find(records,count,"audio","music_volume"),70,0,100),effects=Number(Find(records,count,"audio","effects_volume"),85,0,100);
    SD_SetVolumes(music,effects);SD_SetEnabled(Bool(Find(records,count,"audio","music_enabled"),true),Bool(Find(records,count,"audio","effects_enabled"),true));
    (void)NewViewSize(Number(Find(records,count,"classic","view_size"),15,4,19));
    display_settings d;if(VL_DisplayGet(&d)) {
        d.fullscreen=Bool(Find(records,count,"display","fullscreen"),false);d.integer=Bool(Find(records,count,"display","integer_scaling"),false);
        unsigned scale=Number(Find(records,count,"display","window_scale"),3,1,5);
        d.width=(int)Number(Find(records,count,"display","window_width"),scale*320,1,16384);
        d.height=(int)Number(Find(records,count,"display","window_height"),scale*240,1,16384);d.scale=0;
        d.high_resolution=false;
        const record *render=Find(records,count,"display","renderer");
        if(render) {
            if(render->type==STRING && !strcmp(render->text,"high_resolution")) d.high_resolution=true;
            else if(render->type!=STRING || strcmp(render->text,"original")) Warn("display","renderer");
        }
        d.render_width=Number(Find(records,count,"display","render_width"),640,640,1920);
        d.render_height=Number(Find(records,count,"display","render_height"),480,480,1080);
        if(!((d.render_width==640 && d.render_height==480) || (d.render_width==1280 && d.render_height==720) || (d.render_width==1920 && d.render_height==1080))) { d.render_width=640;d.render_height=480;Warn("display","render_width/render_height"); }
        d.render_auto=Bool(Find(records,count,"display","render_auto"),false);
        d.smooth=Bool(Find(records,count,"display","smooth_rendering"),false);
        d.presentation_fps=Number(Find(records,count,"display","presentation_fps"),144,60,240);
        if(d.presentation_fps!=60 && d.presentation_fps!=120 && d.presentation_fps!=144 && d.presentation_fps!=165 && d.presentation_fps!=240) { d.presentation_fps=144;Warn("display","presentation_fps"); }
        d.vsync=Bool(Find(records,count,"display","vsync"),false);
        d.linear=false;const record *filter=Find(records,count,"display","filter");
        if(filter) { if(filter->type==STRING && !strcmp(filter->text,"linear")) d.linear=true;
            else if(filter->type!=STRING || strcmp(filter->text,"nearest")) Warn("display","filter"); }
        d.aspect=0;const record *ratio=Find(records,count,"display","aspect_ratio");
        if(ratio) {
            static const char *const names[]={"auto","4:3","16:9","16:10","21:9","32:9"};
            bool found=false;for(unsigned i=0;i<6;++i) if(ratio->type==STRING && !strcmp(ratio->text,names[i])) { d.aspect=i;found=true; }
            if(!found) Warn("display","aspect_ratio");
        }
        if(!VL_DisplaySet(&d)) (void)Error("SDL rejected restored display settings; other preferences loaded");
    }
}
config_result Config_Load(const char *dir)
{
    error[0]=0;char path[4096];if(!Path(path,dir)) return CONFIG_IO_ERROR;
    FILE *file=SaveFile_Open(path);
    if(!file) { if(SaveFile_Missing(path)) return CONFIG_MISSING;(void)Error("Cannot read configuration");return CONFIG_IO_ERROR; }
    char *data=malloc(MAX_FILE+1);record *records=calloc(MAX_RECORDS,sizeof(*records));
    if(!data || !records) { free(data);free(records);(void)fclose(file);(void)Error("Configuration allocation failed");return CONFIG_IO_ERROR; }
    size_t size=fread(data,1,MAX_FILE+1,file);bool io=ferror(file)!=0;if(fclose(file)!=0) io=true;
    config_result result=CONFIG_INVALID;unsigned count=0;
    if(io) { (void)Error("Configuration read failed");result=CONFIG_IO_ERROR; }
    else if(size>MAX_FILE || memchr(data,0,size) || !UTF8((const unsigned char *)data,size)) (void)Error("Configuration exceeds limits or contains invalid UTF-8/NUL");
    else {
        data[size]=0;
        if(Parse(data,records,&count)) {
            const record *version=Find(records,count,"","schema_version");
            if(version && (version->type!=INTEGER || version->number!=1)) { (void)Error("Unsupported configuration schema version");result=CONFIG_UNSUPPORTED; }
            else { Apply(records,count);result=CONFIG_LOADED; }
        }
    }
    free(records);free(data);return result;
}
static bool Append(char *out,size_t *length,const char *format,...)
{
    va_list args;va_start(args,format);int n=vsnprintf(out+*length,8192-*length,format,args);va_end(args);
    if(n<0 || (size_t)n>=8192-*length) return false;
    *length+=(size_t)n;return true;
}
bool Config_Save(const char *dir)
{
    error[0]=0;char path[4096],out[8192];size_t n=0;if(!Path(path,dir)) return false;
    display_settings d;if(!VL_DisplayPreferences(&d)) return Error("Cannot snapshot display preferences");
    if(!Append(out,&n,"# Wolf3D Revive preferences; separate from original data and saves.\nschema_version = 1\n\n[controls]\npreset = \"%s\"\n",CT_Preset()==CT_MODERN?"modern":"classic")) return Error("Configuration output overflow");
    for(unsigned i=0;i<2;++i) {
        ct_profile p;CT_GetProfile((ct_preset)i,&p);
        if(!Append(out,&n,"\n[%s_controls]\nmouse_enabled = %s\ninvert_mouse_x = %s\nmouse_sensitivity = %u\n",i==0?"classic":"modern",p.mouse?"true":"false",p.invert?"true":"false",p.sensitivity)) return Error("Configuration output overflow");
        for(unsigned a=0;a<CT_ACTIONS;++a) for(unsigned slot=0;slot<2;++slot)
            if(!Append(out,&n,"bind_%u_%u = %u\n",a,slot,p.binding[a][slot])) return Error("Configuration output overflow");
    }
    unsigned music,effects;bool music_on,effects_on;SD_GetSettings(&music,&effects,&music_on,&effects_on);
    if(!Append(out,&n,"\n[audio]\nmusic_volume = %u\neffects_volume = %u\nmusic_enabled = %s\neffects_enabled = %s\n\n[display]\nfullscreen = %s\ninteger_scaling = %s\nwindow_width = %d\nwindow_height = %d\nrenderer = \"%s\"\nrender_width = %u\nrender_height = %u\n",music,effects,music_on?"true":"false",effects_on?"true":"false",d.fullscreen?"true":"false",d.integer?"true":"false",d.width,d.height,d.high_resolution?"high_resolution":"original",d.render_width,d.render_height)) return Error("Configuration output overflow");
    static const char *const ratios[]={"auto","4:3","16:9","16:10","21:9","32:9"};
    if(!Append(out,&n,"render_auto = %s\nvsync = %s\nfilter = \"%s\"\naspect_ratio = \"%s\"\nsmooth_rendering = %s\npresentation_fps = %u\n\n[classic]\nview_size = %u\n",d.render_auto?"true":"false",d.vsync?"true":"false",d.linear?"linear":"nearest",ratios[d.aspect<6?d.aspect:0],d.smooth?"true":"false",d.presentation_fps,ViewSize())) return Error("Configuration output overflow");
    return SaveFile_WriteAtomic(path,out,n,true) || Error("Cannot atomically replace configuration; previous file retained");
}
