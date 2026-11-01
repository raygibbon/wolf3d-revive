/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_GAME.C SetupGameLevel/ScanInfoPlane, and WL_ACT1.C SpawnDoor marking.
   Original objects and enemies are spawned by ScanInfoPlane. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include <string.h>
objtype player;
w3r_byte tilemap[64][64];
doorobj_t doorobjlist[64];
unsigned doornum;
exit_t playstate;
bool NewGame(unsigned difficulty,unsigned episode)
{
    if (difficulty>3 || episode>5) return CA_Fail("Invalid episode/difficulty");
    memset(&gamestate,0,sizeof(gamestate));
    memset(LevelRatios,0,sizeof(LevelRatios));
    gamestate.episode=(int)episode;gamestate.difficulty=(int)difficulty;
    gamestate.health=100;gamestate.ammo=8;gamestate.lives=3;
    gamestate.nextextra=40000;
    gamestate.weapon=gamestate.bestweapon=gamestate.chosenweapon=wp_pistol;
    return true;
}
unsigned NextMap(unsigned episode,unsigned map,exit_t exit)
{
    static const unsigned back[]={1,1,7,3,5,3}; /* WL_GAME.C ElevatorBackTo */
    /* Floor nine ends an episode; it must never advance to the secret floor.
       Victory is handled separately by GameStep; return the invalid-map sentinel. */
    if (episode>5 || map>9 || map==8 ||
        (exit!=ex_completed && exit!=ex_secretlevel)) return 60;
    return episode*10+(map==9 ? back[episode] : exit==ex_secretlevel ? 9 : map+1);
}
void ShutdownGameLevel(void) { ResetVictory(); CA_UncacheMap(); PM_Shutdown(); doornum=0; InitObjects();ResetPushwalls(); }
/* Original maps can contain unused floor cells on the outer edge (E1M3).
   Only initially reachable space (including closed doors) must be enclosed. This
   keeps malformed maps from letting rays or movement leave the 64x64 grid. */
static bool ValidateEnclosure(void)
{
    bool seen[64][64]={{false}};
    unsigned queue[4096],head=0,tail=0;
    queue[tail++]=(unsigned)(player.tiley*64+player.tilex);seen[player.tilex][player.tiley]=true;
    static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
    while (head<tail) {
        unsigned cell=queue[head++];int x=(int)(cell%64),y=(int)(cell/64);
        if (!x || !y || x==63 || y==63) return CA_Fail("Map perimeter must enclose reachable floor");
        for (unsigned i=0;i<4;++i) {
            int nx=x+dx[i],ny=y+dy[i];
            if (seen[nx][ny]) continue;
            if (tilemap[nx][ny] && tilemap[nx][ny]!=64 && !(tilemap[nx][ny]&128)) continue;
            seen[nx][ny]=true;queue[tail++]=(unsigned)(ny*64+nx);
        }
    }
    return true;
}
bool SetupGameLevel(void)
{
    if (gamestate.episode<0 || gamestate.episode>5 || gamestate.mapon<0 || gamestate.mapon>9 ||
        gamestate.difficulty<0 || gamestate.difficulty>3) return CA_Fail("Invalid game selection");
    if (!CA_CacheMap((unsigned)(gamestate.episode*10+gamestate.mapon)) || !PM_Startup()) { ShutdownGameLevel(); return false; }
    InitObjects();ResetPushwalls();
    gamestate.killcount=gamestate.killtotal=gamestate.treasurecount=gamestate.treasuretotal=0;
    gamestate.secretcount=gamestate.secrettotal=0;gamestate.TimeCount=0;
    gamestate.attackframe=gamestate.weaponframe=gamestate.attackcount=gamestate.faceframe=0;
    gamestate.died=gamestate.victoryflag=false;playstate=ex_playing;
    ResetStatus();
    const maptype *map=CA_Map(); unsigned starts=0;
    memset(tilemap,0,sizeof(tilemap)); doornum=0;
    for (unsigned y=0;y<64;++y) for (unsigned x=0;x<64;++x) {
        unsigned tile=map->planes[0][y*64+x];
        if (tile>143) { (void)CA_Fail("Invalid map area number"); goto bad; }
        if (tile==106 || tile>=107) tile=0; /* AMBUSHTILE becomes area floor. */
        if (tile>63 && (tile<90 || tile>101)) { (void)CA_Fail("Unsupported/invalid wall tile %u",tile); goto bad; }
        tilemap[x][y]=(w3r_byte)tile;
        unsigned spawn=map->planes[1][y*64+x];
        if (spawn>=19 && spawn<=22) {
            ++starts; player.x=(int32_t)(x*65536+32768); player.y=(int32_t)(y*65536+32768);
            player.tilex=(int)x; player.tiley=(int)y;
            player.angle=(1-(int)(spawn-19))*90;
            if (player.angle<0) player.angle+=360;
        }
    }
    for (unsigned y=1;y<63;++y) for (unsigned x=1;x<63;++x) {
        unsigned tile=map->planes[0][y*64+x];
        if (tile>=90 && tile<=101) {
            if (doornum>=64) { (void)CA_Fail("Too many doors"); goto bad; }
            doorobj_t *door=&doorobjlist[doornum];
            door->action=dr_closed;door->position=0;door->ticcount=0;
            door->x=x; door->y=y; door->vertical=(tile%2)==0; door->lock=(tile-90)/2;
            tilemap[x][y]=(w3r_byte)(128+doornum++);
            if (door->vertical) { tilemap[x][y-1]|=64; tilemap[x][y+1]|=64; }
            else { tilemap[x-1][y]|=64; tilemap[x+1][y]|=64; }
        }
    }
    if (!ScanInfoPlane()) goto bad;
    if (starts!=1 || !TryMove(&player)) { (void)CA_Fail("Invalid/obstructed player spawn"); goto bad; }
    if (!ValidateEnclosure()) goto bad;
    /* Validate and cache every wall page referenced by this level before play. */
    for (unsigned y=0;y<64;++y) for (unsigned x=0;x<64;++x) {
        unsigned tile=tilemap[x][y];
        if (!tile || (tile&128)) continue;
        tile&=63;
        if (tile && (tile*2-1>=PMSpriteStart)) { (void)CA_Fail("Wall texture outside VSWAP wall pages"); goto bad; }
        if (tile && (!PM_GetPage((tile-1)*2,NULL) || !PM_GetPage((tile-1)*2+1,NULL))) goto bad;
    }
    for (unsigned i=0;i<8;++i) if (!PM_GetPage(PMSpriteStart-8+i,NULL)) goto bad;
    w3r_byte pixels[4096],mask[4096];
    for (unsigned sprite=0;sprite<PMSoundStart-PMSpriteStart;++sprite) {
        /* Sparse sprite entries are allowed by the original page directory. */
        if (PM_PagePresent(PMSpriteStart+sprite) && !PM_DecodeSprite(sprite,pixels,mask)) goto bad;
    }
    if (!ValidateActorSprites()) goto bad;
    for (unsigned sprite=SPR_KNIFEREADY;sprite<=SPR_CHAINATK4;++sprite)
        if (!PM_DecodeSprite(sprite,pixels,mask)) goto bad;
    for (unsigned i=0;i<statcount;++i)
        if (!PM_DecodeSprite((unsigned)statobjlist[i].shapenum,pixels,mask)) goto bad;
    ResetMovement(); BuildTables(); return true;
bad:
    ShutdownGameLevel(); return false;
}
