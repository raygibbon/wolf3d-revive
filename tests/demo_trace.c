/* SPDX-License-Identifier: GPL-2.0-only */
/* Developer-only deterministic trace harness; never linked into gameplay. */
#include "engine/WL_DEF.h"
#include "engine/WL_DEMO.h"
#include "engine/WL_SAVE.h"
#include "engine/ID_US_1.h"
#include "engine/ID_CA.h"
#include "engine/ID_VH.h"
#include <stdio.h>
#include <stdlib.h>
#define SAVE_STATE(name) #name,
static const char *const names[]={"null",
#include "engine/WL_SAVE_STATES.inc"
};
#undef SAVE_STATE
static const char *State(const statetype *s) { unsigned id=Save_StateID(s);return id<Save_StateCount()?names[id]:"unknown"; }
static void Trace(unsigned demo)
{
    demo_progress p;demo_command command={0};
    if(!Demo_Progress(&p)) exit(2);
    /* Source bytes, not scaled movement or host input. */
    if(p.commands_consumed) {
        const w3r_byte *s=(const w3r_byte *)grsegs[T_DEMO0+demo]+4+3*(p.commands_consumed-1);
        command.buttons=s[0];command.x=s[1]<128?s[1]:(int)s[1]-256;command.y=s[2]<128?s[2]:(int)s[2]-256;
    }
    printf("F,%u,%zu,%u,%d,%d,%d,%d,%d,%u,%u,%d,%d,%d,%d,%d\n",demo,p.commands_consumed,gamestate.TimeCount,
           player.x,player.y,player.angle,gamestate.health,gamestate.ammo,US_RndIndex(),command.buttons,command.x,command.y,
           gamestate.weapon,gamestate.attackframe?gamestate.attackframe-1:0,gamestate.attackcount);
    printf("V,");
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) putchar(spotvis[x][y]?'1':'0');
    putchar('\n');
    printf("M,");
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) printf("%02x",tilemap[x][y]);
    putchar('\n');
    for(unsigned d=0;d<doornum;++d) {
        const doorobj_t *o=&doorobjlist[d];
        printf("D,%u,%u,%u,%u,%u,%d,%u,%u\n",d,o->x,o->y,o->vertical?1U:0U,o->lock,(int)o->action,o->ticcount,(unsigned)o->position);
    }
    printf("Q");
    for(unsigned x=0;x<64;++x) for(unsigned y=0;y<64;++y) {
        unsigned slot=0;
        for(unsigned j=0;actorat[x][y] && j<objcount;++j) if(actorat[x][y]==&objlist[j]) { slot=j+1;break; }
        printf(",%u",slot);
    }
    putchar('\n');
    unsigned i=0;
    for(const objtype *o=&player;o;o=o->next,++i)
        printf("A,%u,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d\n",i,State(o->state),o->x,o->y,o->angle,o->tilex,o->tiley,o->obclass,
            o->ticcount,o->hitpoints,o->dir,o->speed,o->distance,o->temp1,o->temp2,o->flags,o->active?1:0,o->viewx,o->viewheight);
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    MM_Startup();if(!CA_Startup(argv[1])) return 2;
    for(unsigned n=0;n<4;++n) {
        if(!NewViewSize(15) || !Demo_Start(n,0)) return 2;
        Trace(n);
        for(uint64_t t=4;!Demo_Finished();t+=4) {
            if(!GameStep(t)) return 2;
            Trace(n);
        }
        GameStop();
    }
    CA_Shutdown();MM_Shutdown();return MM_TotalBytes()?2:0;
}
