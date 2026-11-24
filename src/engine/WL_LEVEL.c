/* SPDX-License-Identifier: GPL-2.0-only */
/* Minimal versioned loose-level file used by the Workshop blank-level builder.
   The format stores only authored map semantics: dimensions, spawn and the
   wall/tile grid. Textures remain external asset-library references (wall IDs).
   It is intentionally not a PK3 archive or a replacement for GAMEMAPS. */
#include "WL_LEVEL.h"
#include "platform/save_file.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WL_LEVEL_HEADER 48U
#define WL_LEVEL_PAYLOAD (WL_LEVEL_SIZE*WL_LEVEL_SIZE)
#define WL_LEVEL_FILE_SIZE (WL_LEVEL_HEADER+WL_LEVEL_PAYLOAD)
#define WL_LEVEL_MAGIC "W3RLEVEL"

static void SetError(char *error,size_t size,const char *text)
{
    if(!error || !size) return;
    (void)snprintf(error,size,"%s",text?text:"invalid level");
}

static void Put32(uint8_t *p,uint32_t value)
{
    for(unsigned i=0;i<4;++i) p[i]=(uint8_t)(value>>(i*8U));
}

static uint32_t Get32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

static uint32_t Crc32(uint32_t crc,const uint8_t *data,size_t size)
{
    while(size--) {
        crc^=*data++;
        for(unsigned i=0;i<8;++i) crc=(crc>>1)^((crc&1U)?UINT32_C(0xedb88320):0U);
    }
    return crc;
}

bool WL_LevelMaterialValid(unsigned material)
{
    /* 21/22 are the elevator-switch wall textures in the original asset set.
       They are valid engine wall IDs but not safe authored materials. */
    return material>=1 && material<=63 && material!=21 && material!=22;
}

static bool BoundaryCell(unsigned x,unsigned y)
{
    return x==0 || y==0 || x==WL_LEVEL_SIZE-1U || y==WL_LEVEL_SIZE-1U;
}

bool WL_LevelValidate(const wl_level *level,char *error,size_t error_size)
{
    if(!level) { SetError(error,error_size,"missing level");return false; }
    if(level->width!=WL_LEVEL_SIZE || level->height!=WL_LEVEL_SIZE) {
        SetError(error,error_size,"level must be 64x64");return false;
    }
    if(level->spawn_x<1 || level->spawn_x>=WL_LEVEL_SIZE-1U ||
       level->spawn_y<1 || level->spawn_y>=WL_LEVEL_SIZE-1U) {
        SetError(error,error_size,"spawn is outside the interior");return false;
    }
    if(level->spawn_angle>=360) { SetError(error,error_size,"invalid spawn angle");return false; }
    for(unsigned x=0;x<WL_LEVEL_SIZE;++x) for(unsigned y=0;y<WL_LEVEL_SIZE;++y) {
        unsigned tile=level->tiles[x][y];
        if(tile>63 || tile==21 || tile==22) {
            SetError(error,error_size,"unsupported wall texture id");return false;
        }
        if(BoundaryCell(x,y) && !tile) {
            SetError(error,error_size,"map boundary is not enclosed");return false;
        }
    }
    if(level->tiles[level->spawn_x][level->spawn_y]) {
        SetError(error,error_size,"spawn is inside a wall");return false;
    }
    bool seen[WL_LEVEL_SIZE][WL_LEVEL_SIZE]={{false}};
    unsigned queue[WL_LEVEL_SIZE*WL_LEVEL_SIZE],head=0,tail=0;
    queue[tail++]=(unsigned)(level->spawn_y*WL_LEVEL_SIZE+level->spawn_x);
    seen[level->spawn_x][level->spawn_y]=true;
    static const int dx[]={1,0,-1,0},dy[]={0,1,0,-1};
    while(head<tail) {
        unsigned cell=queue[head++];
        unsigned x=cell%WL_LEVEL_SIZE,y=cell/WL_LEVEL_SIZE;
        for(unsigned i=0;i<4;++i) {
            int nx=(int)x+dx[i],ny=(int)y+dy[i];
            if(nx<0 || ny<0 || nx>=(int)WL_LEVEL_SIZE || ny>=(int)WL_LEVEL_SIZE || seen[nx][ny]) continue;
            if(level->tiles[nx][ny]) continue;
            seen[nx][ny]=true;queue[tail++]=(unsigned)((unsigned)ny*WL_LEVEL_SIZE+(unsigned)nx);
        }
    }
    for(unsigned x=0;x<WL_LEVEL_SIZE;++x) for(unsigned y=0;y<WL_LEVEL_SIZE;++y) {
        if(!level->tiles[x][y] && !seen[x][y]) {
            SetError(error,error_size,"level contains an unreachable floor area");return false;
        }
    }
    return true;
}

bool WL_LevelBlank(wl_level *level,unsigned material)
{
    if(!level || !WL_LevelMaterialValid(material)) return false;
    memset(level,0,sizeof(*level));
    level->width=level->height=WL_LEVEL_SIZE;
    level->spawn_x=level->spawn_y=WL_LEVEL_SIZE/2U;
    level->spawn_angle=0;
    for(unsigned i=0;i<WL_LEVEL_SIZE;++i) {
        level->tiles[0][i]=level->tiles[WL_LEVEL_SIZE-1U][i]=(w3r_byte)material;
        level->tiles[i][0]=level->tiles[i][WL_LEVEL_SIZE-1U]=(w3r_byte)material;
    }
    return WL_LevelValidate(level,NULL,0);
}

bool WL_LevelFromWorld(wl_level *level,const w3r_byte tiles[WL_LEVEL_SIZE][WL_LEVEL_SIZE],
                       unsigned spawn_x,unsigned spawn_y,int spawn_angle)
{
    if(!level || !tiles || spawn_x>=WL_LEVEL_SIZE || spawn_y>=WL_LEVEL_SIZE) return false;
    memset(level,0,sizeof(*level));
    level->width=level->height=WL_LEVEL_SIZE;
    level->spawn_x=spawn_x;level->spawn_y=spawn_y;
    level->spawn_angle=(unsigned)((spawn_angle%360+360)%360);
    memcpy(level->tiles,tiles,sizeof(level->tiles));
    return WL_LevelValidate(level,NULL,0);
}

bool WL_LevelToMap(const wl_level *level,maptype *map)
{
    if(!level || !map || !WL_LevelValidate(level,NULL,0)) return false;
    memset(map,0,sizeof(*map));
    (void)memcpy(map->name,"WORKSHOP",sizeof("WORKSHOP"));
    map->number=0;
    for(unsigned x=0;x<WL_LEVEL_SIZE;++x) for(unsigned y=0;y<WL_LEVEL_SIZE;++y)
        map->planes[0][y*WL_LEVEL_SIZE+x]=level->tiles[x][y];
    /* Plane 1 carries the original cardinal spawn code. The exact authored
       facing angle is restored by the game after setup. */
    static const w3r_word spawn_codes[4]={20,19,22,21};
    unsigned quadrant=((level->spawn_angle+45U)/90U)%4U;
    map->planes[1][level->spawn_y*WL_LEVEL_SIZE+level->spawn_x]=spawn_codes[quadrant];
    return true;
}

bool WL_LevelPath(const char *directory,char *path,size_t path_size)
{
    if(!directory || !*directory || !path || path_size<=sizeof(WL_LEVEL_FILE)+1U) return false;
    size_t length=strlen(directory);
    if(length+sizeof(WL_LEVEL_FILE)+1U>path_size) return false;
    int written=snprintf(path,path_size,"%s/%s",directory,WL_LEVEL_FILE);
    return written>0 && (size_t)written<path_size;
}

bool WL_LevelExists(const char *directory)
{
    char path[4096];
    if(!WL_LevelPath(directory,path,sizeof(path))) return false;
    FILE *file=SaveFile_Open(path);
    if(!file) return false;
    (void)fclose(file);
    return true;
}

bool WL_LevelSave(const wl_level *level,const char *directory,char *error,size_t error_size)
{
    if(!WL_LevelValidate(level,error,error_size)) return false;
    char path[4096];
    if(!WL_LevelPath(directory,path,sizeof(path))) {
        SetError(error,error_size,"invalid or too-long level directory");return false;
    }
    uint8_t data[WL_LEVEL_FILE_SIZE];
    memset(data,0,sizeof(data));
    memcpy(data,WL_LEVEL_MAGIC,8);
    Put32(data+8,WL_LEVEL_VERSION);
    Put32(data+12,WL_LEVEL_HEADER);
    Put32(data+16,WL_LEVEL_PAYLOAD);
    Put32(data+24,level->width);
    Put32(data+28,level->height);
    Put32(data+32,level->spawn_x);
    Put32(data+36,level->spawn_y);
    Put32(data+40,level->spawn_angle);
    Put32(data+44,0);
    for(unsigned x=0;x<WL_LEVEL_SIZE;++x)
        memcpy(data+WL_LEVEL_HEADER+x*WL_LEVEL_SIZE,level->tiles[x],WL_LEVEL_SIZE);
    Put32(data+20,Crc32(UINT32_MAX,data+WL_LEVEL_HEADER,WL_LEVEL_PAYLOAD));
    if(!SaveFile_WriteAtomic(path,data,sizeof(data),true)) {
        SetError(error,error_size,"cannot write level file; previous file retained");return false;
    }
    return true;
}

bool WL_LevelLoad(wl_level *level,const char *directory,char *error,size_t error_size)
{
    if(!level) { SetError(error,error_size,"missing level destination");return false; }
    char path[4096];
    if(!WL_LevelPath(directory,path,sizeof(path))) {
        SetError(error,error_size,"invalid or too-long level directory");return false;
    }
    FILE *file=SaveFile_Open(path);
    if(!file) { SetError(error,error_size,"cannot open level file");return false; }
    uint8_t data[WL_LEVEL_FILE_SIZE];
    bool ok=false;
    if(fseek(file,0,SEEK_END)!=0) goto done;
    long size=ftell(file);
    if(size!=(long)WL_LEVEL_FILE_SIZE) { SetError(error,error_size,"truncated or oversized level file");goto done; }
    if(fseek(file,0,SEEK_SET)!=0 || fread(data,1,sizeof(data),file)!=sizeof(data)) {
        SetError(error,error_size,"cannot read level file");goto done;
    }
    if(fgetc(file)!=EOF || ferror(file)) { SetError(error,error_size,"level file has trailing data");goto done; }
    if(memcmp(data,WL_LEVEL_MAGIC,8)) { SetError(error,error_size,"not a Wolf3D Revive level file");goto done; }
    if(Get32(data+12)!=WL_LEVEL_HEADER || Get32(data+16)!=WL_LEVEL_PAYLOAD) {
        SetError(error,error_size,"invalid level header size");goto done;
    }
    if(Get32(data+8)!=WL_LEVEL_VERSION) { SetError(error,error_size,"unsupported level format version");goto done; }
    if(Get32(data+24)!=WL_LEVEL_SIZE || Get32(data+28)!=WL_LEVEL_SIZE) {
        SetError(error,error_size,"invalid level dimensions");goto done;
    }
    if(Get32(data+44)!=0) { SetError(error,error_size,"unsupported level flags");goto done; }
    if(Get32(data+20)!=Crc32(UINT32_MAX,data+WL_LEVEL_HEADER,WL_LEVEL_PAYLOAD)) {
        SetError(error,error_size,"level file checksum mismatch");goto done;
    }
    wl_level staged;
    memset(&staged,0,sizeof(staged));
    staged.width=staged.height=WL_LEVEL_SIZE;
    staged.spawn_x=Get32(data+32);staged.spawn_y=Get32(data+36);staged.spawn_angle=Get32(data+40);
    for(unsigned x=0;x<WL_LEVEL_SIZE;++x)
        memcpy(staged.tiles[x],data+WL_LEVEL_HEADER+x*WL_LEVEL_SIZE,WL_LEVEL_SIZE);
    if(!WL_LevelValidate(&staged,error,error_size)) goto done;
    *level=staged;
    ok=true;
done:
    (void)fclose(file);
    return ok;
}
