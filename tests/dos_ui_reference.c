/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/WL_MENU.h"
#include "engine/ID_CA.h"
#include "engine/ID_VH.h"
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
    CHECK(fgetc(f)==EOF && fclose(f)==0);CA_Shutdown();MM_Shutdown();CHECK(!MM_TotalBytes());
    puts("All nine quit sayings at both DOS window heights and both disk dialogs match the original DOS executable pixel-for-pixel.");return 0;
}
