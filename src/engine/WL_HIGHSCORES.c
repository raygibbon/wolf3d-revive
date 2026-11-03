/* SPDX-License-Identifier: GPL-2.0-only */
/* ID_US_1.C defaults/US_LineInput and WL_INTER.C CheckHighScore.
   Persistence is a portable, versioned file rather than a DOS struct dump. */
#include "WL_HIGHSCORES.h"
#include "WL_MENU.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include "platform/save_file.h"
#include <stdio.h>
#include <string.h>
static high_score table[HIGH_SCORE_COUNT];
static bool initialized,writable;
static char path[4096],edit[HIGH_NAME_MAX+1];
static int entry=-1;
static size_t cursor;
static uint64_t blink_at;
static bool visible;
void HighScores_Reset(void)
{
    static const char *const names[]={"id software-'92","Adrian Carmack","John Carmack","Kevin Cloud","Tom Hall","John Romero","Jay Wilbur"};
    memset(table,0,sizeof(table));
    for(unsigned i=0;i<HIGH_SCORE_COUNT;++i) {
        (void)snprintf(table[i].name,sizeof(table[i].name),"%s",names[i]);
        table[i].score=10000;table[i].completed=1;
    }
    initialized=true;entry=-1;
}
const high_score *HighScores_Table(void) { if(!initialized) HighScores_Reset();return table; }
static uint32_t Get32(const unsigned char *p)
{ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void Put32(unsigned char *p,uint32_t v)
{ for(unsigned i=0;i<4;++i) p[i]=(unsigned char)(v>>(i*8)); }
static uint32_t Hash(const unsigned char *p,size_t n)
{ uint32_t h=2166136261U;while(n--) h=(h^*p++)*16777619U;return h; }
bool HighScores_Load(const char *directory)
{
    HighScores_Reset();writable=false;path[0]=0;
    if(!directory || !*directory) return CA_Fail("High-score storage unavailable");
    int n=snprintf(path,sizeof(path),"%s/highscores.w3h",directory);
    if(n<0 || (size_t)n>=sizeof(path)) { path[0]=0;return CA_Fail("High-score path too long"); }
    FILE *f=SaveFile_Open(path);
    if(!f) { writable=SaveFile_Missing(path);return writable?true:CA_Fail("Cannot read high scores"); }
    unsigned char bytes[460];size_t count=fread(bytes,1,sizeof(bytes),f);int extra=fgetc(f),error=ferror(f);
    if(fclose(f)!=0) error=1;
    if(error || count!=sizeof(bytes) || extra!=EOF || memcmp(bytes,"W3HS\1\7\0\0",8) || Get32(bytes+456)!=Hash(bytes,456))
        return CA_Fail("Invalid or unsupported high-score file; preserved");
    high_score loaded[HIGH_SCORE_COUNT]={0};
    for(unsigned i=0;i<HIGH_SCORE_COUNT;++i) {
        const unsigned char *p=bytes+8+i*64;
        if(!memchr(p,0,HIGH_NAME_MAX+1) || p[58]<1 || p[58]>10 || p[59]>5)
            return CA_Fail("Invalid high-score entry; file preserved");
        size_t len=strlen((const char *)p);
        for(size_t j=0;j<len;++j) if(p[j]<32 || p[j]>126) return CA_Fail("Invalid high-score name; file preserved");
        memcpy(loaded[i].name,p,len+1);loaded[i].completed=p[58];loaded[i].episode=p[59];loaded[i].score=Get32(p+60);
        if(i && (loaded[i].score>loaded[i-1].score || (loaded[i].score==loaded[i-1].score && loaded[i].completed>loaded[i-1].completed)))
            return CA_Fail("Unsorted high-score file; preserved");
    }
    memcpy(table,loaded,sizeof(table));writable=true;return true;
}
bool HighScores_Save(void)
{
    if(!writable || !*path) return CA_Fail("High-score storage unavailable or protected");
    unsigned char bytes[460]={0};memcpy(bytes,"W3HS\1\7\0\0",8);
    const high_score *scores=HighScores_Table();
    for(unsigned i=0;i<HIGH_SCORE_COUNT;++i) {
        unsigned char *p=bytes+8+i*64;memcpy(p,scores[i].name,strlen(scores[i].name)+1);
        p[58]=(unsigned char)scores[i].completed;p[59]=(unsigned char)scores[i].episode;Put32(p+60,scores[i].score);
    }
    Put32(bytes+456,Hash(bytes,456));
    return SaveFile_WriteAtomic(path,bytes,sizeof(bytes),true)?true:CA_Fail("Cannot save high scores");
}
int HighScores_Insert(uint32_t score,unsigned completed,unsigned episode)
{
    (void)HighScores_Table();if(completed<1 || completed>10 || episode>5) return -1;
    for(unsigned i=0;i<HIGH_SCORE_COUNT;++i) if(score>table[i].score || (score==table[i].score && completed>table[i].completed)) {
        for(unsigned j=HIGH_SCORE_COUNT-1;j>i;--j) table[j]=table[j-1];
        memset(&table[i],0,sizeof(table[i]));table[i].score=score;table[i].completed=completed;table[i].episode=episode;return (int)i;
    }
    return -1;
}
static bool DrawEntry(void)
{
    if(!DrawHighScores()) return false;
    fontnumber=0;fontcolor=15;PrintX=32;PrintY=76+16*entry;
    if(!US_Print(edit)) return false;
    if(visible) {
        char prefix[HIGH_NAME_MAX+1];memcpy(prefix,edit,cursor);prefix[cursor]=0;
        int w,h;if(!VW_MeasurePropString(prefix,&w,&h)) return false;
        /* ID_US_1.C USL_XORICursor uses the original I-bar glyph 0x80. */
        px=32+w-1;py=PrintY;return VW_DrawPropString("\x80");
    }
    return true;
}
bool BeginHighScoreEntry(int rank,uint64_t ticks)
{
    if(rank<0 || rank>=HIGH_SCORE_COUNT) return false;
    entry=rank;edit[0]=0;cursor=0;visible=true;blink_at=ticks+35;IN_ClearKeysDown();return DrawEntry();
}
int HandleHighScoreEntry(uint64_t ticks)
{
    if(entry<0) return 1;
    bool accept=IN_ConsumePress(sc_Enter),cancel=IN_ConsumePress(sc_Escape);
    if(accept || cancel) {
        /* DOS Escape cancels only the typed name; the qualifying score stays. */
        if(accept) (void)snprintf(table[entry].name,sizeof(table[entry].name),"%s",edit);
        entry=-1;IN_ClearKeysDown();return 1;
    }
    size_t n=strlen(edit);
    if(IN_ConsumePress(sc_LeftArrow) && cursor) --cursor;
    if(IN_ConsumePress(sc_RightArrow) && cursor<n) ++cursor;
    if(IN_ConsumePress(0x47)) cursor=0;
    if(IN_ConsumePress(0x4f)) cursor=n;
    if(IN_ConsumePress(0x0e) && cursor) { memmove(edit+cursor-1,edit+cursor,n-cursor+1);--cursor;--n; }
    if(IN_ConsumePress(0x53) && cursor<n) { memmove(edit+cursor,edit+cursor+1,n-cursor);--n; }
    char ch;
    while((ch=IN_NextASCII())!=0) {
        int w,h;fontnumber=0;if(!VW_MeasurePropString(edit,&w,&h)) return -1;
        /* Original US_LineInput tests the current width before insertion. */
        if(n>=HIGH_NAME_MAX || w>=100) continue;
        memmove(edit+cursor+1,edit+cursor,n-cursor+1);edit[cursor++]=ch;++n;
    }
    if(ticks>=blink_at) { uint64_t count=(ticks-blink_at)/35+1;blink_at+=count*35;if(count%2) visible=!visible; }
    return DrawEntry()?0:-1;
}
