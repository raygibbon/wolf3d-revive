/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_VH.h"
#include "engine/WL_DEF.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%d: %s (%s)\n",__LINE__,#x,CA_Error());exit(1); } } while(0)
static unsigned char planes[64000];
static void Compare(FILE *f,unsigned index)
{
    CHECK(fread(planes,1,sizeof(planes),f)==sizeof(planes));
    unsigned differences=0;
    for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
        if(framebuffer[y*320+x]!=planes[(x%4)*16000+y*80+x/4]) ++differences;
    if(differences) { fprintf(stderr,"DOS UI frame %u: %u differing pixels\n",index,differences);exit(1); }
}
int main(int argc,char **argv)
{
    CHECK(argc==3);MM_Startup();CHECK(CA_Startup(argv[1]));
    FILE *f=fopen(argv[2],"rb");CHECK(f);
    for(unsigned i=0;i<18;++i) {
        unsigned n=i%9;ClearMScreen();
        CHECK(MessageAtHeight(MenuQuitMessage(n,n==8?1:0),i<9?200U:160U));Compare(f,i);
    }
    for(unsigned i=0;i<2;++i) { ClearMScreen();CHECK(DrawSaveActivity(i!=0,false));Compare(f,18+i); }
    ClearMScreen();CHECK(VWB_DrawPic(48,56,GETPSYCHEDPIC));Compare(f,20);
    for(unsigned i=0;i<=5;++i) { PreloadUpdate(i,5);Compare(f,21+i); }
    ClearMScreen();CHECK(VWB_DrawPic(128,64,PAUSEDPIC));Compare(f,27);
    CHECK(MessageAtHeight("You are currently in\na game. Continuing will\nerase old game. Ok?",200));Compare(f,28);
    CHECK(DrawHighScores());Compare(f,29);
    CHECK(fgetc(f)==EOF && fclose(f)==0);CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("30 DOS frames match: quit/disk dialogs, Get Psyched artwork and six progress stages, Pause artwork, new-game message and default scores.");return 0;
}
