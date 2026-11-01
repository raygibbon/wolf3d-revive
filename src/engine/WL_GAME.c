/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_GAME.C SetupGameLevel/ScanInfoPlane, and WL_ACT1.C SpawnDoor marking.
   Actor spawning/combat are not part of this wall/movement slice. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include <string.h>
objtype player;
w3r_byte tilemap[64][64];
doorobj_t doorobjlist[64];
unsigned doornum;
void ShutdownGameLevel(void) { CA_UncacheMap(); PM_Shutdown(); doornum=0; }
bool SetupGameLevel(void)
{
    if (!CA_CacheMap(0) || !PM_Startup()) { ShutdownGameLevel(); return false; }
    const maptype *map=CA_Map(); unsigned starts=0;
    memset(tilemap,0,sizeof(tilemap)); doornum=0;
    for (unsigned y=0;y<64;++y) for (unsigned x=0;x<64;++x) {
        unsigned tile=map->planes[0][y*64+x];
        if (tile>143) { (void)CA_Fail("Invalid map area number"); goto bad; }
        if (tile==106 || tile>=107) tile=0; /* AMBUSHTILE becomes area floor. */
        if (tile>63 && (tile<90 || tile>101)) { (void)CA_Fail("Unsupported/invalid wall tile %u",tile); goto bad; }
        if ((!x || !y || x==63 || y==63) && (!tile || tile>=90)) { (void)CA_Fail("Map perimeter must be solid walls"); goto bad; }
        tilemap[x][y]=(w3r_byte)tile;
        unsigned spawn=map->planes[1][y*64+x];
        if (spawn>=19 && spawn<=22) {
            ++starts; player.x=(int32_t)(x*65536+32768); player.y=(int32_t)(y*65536+32768);
            player.tilex=x; player.tiley=y;
            player.angle=(1-(int)(spawn-19))*90;
            if (player.angle<0) player.angle+=360;
        }
    }
    for (unsigned y=1;y<63;++y) for (unsigned x=1;x<63;++x) {
        unsigned tile=map->planes[0][y*64+x];
        if (tile>=90 && tile<=101) {
            if (doornum>=64) { (void)CA_Fail("Too many doors"); goto bad; }
            size_t cell=y*64+x;
            unsigned a=map->planes[0][cell-((tile%2)==0 ? 64U : 1U)];
            unsigned b=map->planes[0][cell+((tile%2)==0 ? 64U : 1U)];
            if (!a || a>63 || !b || b>63) { (void)CA_Fail("Invalid door jamb walls"); goto bad; }
            doorobj_t *door=&doorobjlist[doornum];
            door->action=dr_closed;door->position=0;door->ticcount=0;
            door->x=x; door->y=y; door->vertical=(tile%2)==0; door->lock=(tile-90)/2;
            tilemap[x][y]=(w3r_byte)(128+doornum++);
            if (door->vertical) { tilemap[x][y-1]|=64; tilemap[x][y+1]|=64; }
            else { tilemap[x-1][y]|=64; tilemap[x+1][y]|=64; }
        }
    }
    if (starts!=1 || !TryMove(&player)) { (void)CA_Fail("Invalid/obstructed player spawn"); goto bad; }
    /* Validate and cache every wall page referenced by this level before play. */
    for (unsigned y=0;y<64;++y) for (unsigned x=0;x<64;++x) {
        unsigned tile=tilemap[x][y];
        if (!tile || (tile&128)) continue;
        tile&=63;
        if (tile && (tile*2-1>=PMSpriteStart)) { (void)CA_Fail("Wall texture outside VSWAP wall pages"); goto bad; }
        if (tile && (!PM_GetPage((tile-1)*2,NULL) || !PM_GetPage((tile-1)*2+1,NULL))) goto bad;
    }
    for (unsigned i=0;i<8;++i) if (!PM_GetPage(PMSpriteStart-8+i,NULL)) goto bad;
    ResetMovement(); BuildTables(); return true;
bad:
    ShutdownGameLevel(); return false;
}
