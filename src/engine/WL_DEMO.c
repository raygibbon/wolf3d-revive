/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_GAME.C PlayDemo / WL_PLAY.C PollControls. Original embedded WL6 chunks,
   four 70-Hz tics per signed x/y/button command; no host input or fake RNG. */
#include "WL_DEMO.h"
#include "WL_DEF.h"
#include "WL_SAVE.h"
#include "ID_CA.h"
#include "ID_FILE.h"
static const w3r_byte *stream;
static size_t end,cursor;
static unsigned buttons,source_map;
bool Demo_Progress(demo_progress *progress)
{
    if(!progress || !stream || end<4 || cursor<4 || cursor>end) return false;
    progress->map=source_map;
    progress->commands_total=(end-4)/3;
    progress->commands_consumed=(cursor-4)/3;
    progress->buttons=buttons;
    return true;
}
int SimulationTics(void) { return Demo_Active()?4:1; }
static int Signed(unsigned value) { return value<128?(int)value:(int)value-256; }
bool Demo_Validate(const w3r_byte *data,size_t size,unsigned *map,size_t *length)
{
    if(!data || size<7 || !map || !length) return CA_Fail("Truncated demo header/commands");
    size_t n=CA_LE16(data+1);
    if(data[0]>=60 || n<7 || n>size || (n-4)%3) return CA_Fail("Invalid demo map/length");
    for(size_t i=4;i<n;i+=3) {
        int x=Signed(data[i+1]),y=Signed(data[i+2]);
        if(x< -100 || x>100 || y< -100 || y>100) return CA_Fail("Demo control outside original limits");
    }
    *map=data[0];*length=n;return true;
}
void Demo_Stop(void) { stream=NULL;end=cursor=0;buttons=source_map=0; }
bool Demo_Active(void) { return stream!=NULL; }
unsigned Demo_Buttons(void) { return buttons; }
bool Demo_Finished(void) { return !stream || cursor>=end || gamestate.died || playstate!=ex_playing; }
bool Demo_ReadTic(demo_command *command)
{
    if(Demo_Finished() || !command) return false;
    unsigned previous=buttons;buttons=stream[cursor];
    command->buttons=buttons;command->x=Signed(stream[cursor+1]);command->y=Signed(stream[cursor+2]);
    command->use=(buttons&8U)!=0 && !(previous&8U);
    cursor+=3;
    return true;
}
bool Demo_Start(unsigned number,uint64_t ticks)
{
    GameStop();if(number>=4) return CA_Fail("Invalid demo number");
    int chunk=T_DEMO0+(int)number;
    if(!CA_CacheGrChunk(chunk)) return false;
    unsigned map;size_t length;
    if(!Demo_Validate(grsegs[chunk],grsizes[chunk],&map,&length) || !GameStartMap(ticks,map,3,0)) return false;
    stream=grsegs[chunk];end=length;cursor=4;buttons=0;source_map=map;
    if(GameDrawPaused()) return true;
    GameStop();return false;
}
