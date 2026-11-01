/* SPDX-License-Identifier: GPL-2.0-only */
/* WOLFSRC/ID_CA.C CAL_CarmackExpand, CA_RLEWexpand, CA_CacheMap.
   Three disk planes are retained, including the unused third plane. */
#include "ID_MAP.h"
#include "ID_CA.h"
#include "ID_FILE.h"
#include <string.h>
static memptr mapmem;
const maptype *CA_Map(void) { return mapmem; }
void CA_UncacheMap(void) { if (mapmem) (void)MM_FreePtr(&mapmem); }
bool CAL_CarmackExpand(const w3r_byte *src,size_t bytes,w3r_word *dst,size_t words)
{
    size_t in=0,out=0;
    if ((!src && bytes) || (!dst && words)) return CA_Fail("Invalid Carmack buffers");
    while (out<words) {
        if (bytes-in<2) return CA_Fail("Truncated Carmack stream");
        unsigned ch=CA_LE16(src+in); in+=2;
        unsigned tag=ch>>8,count=ch&255;
        if (tag!=0xa7 && tag!=0xa8) { dst[out++]=(w3r_word)ch; continue; }
        if (!count) {
            if (in>=bytes) return CA_Fail("Truncated escaped Carmack tag");
            dst[out++]=(w3r_word)((ch&0xff00)|src[in++]); continue;
        }
        size_t offset;
        if (tag==0xa7) {
            if (in>=bytes) return CA_Fail("Truncated Carmack near offset");
            unsigned back=src[in++];
            if (!back || back>out) return CA_Fail("Invalid Carmack near reference");
            offset=out-back;
        } else {
            if (bytes-in<2) return CA_Fail("Truncated Carmack far offset");
            offset=CA_LE16(src+in); in+=2;
            if (offset>=out) return CA_Fail("Invalid Carmack far reference");
        }
        if (count>words-out) return CA_Fail("Carmack output overrun");
        /* Overlap is intentional: original forward LZ word copies. */
        while (count--) dst[out++]=dst[offset++];
    }
    return true;
}
bool CA_RLEWexpand(const w3r_word *src,size_t words,w3r_word *dst,size_t output,w3r_word tag)
{
    size_t in=0,out=0;
    if ((!src && words) || (!dst && output)) return CA_Fail("Invalid RLEW buffers");
    while (out<output) {
        if (in>=words) return CA_Fail("Truncated RLEW stream");
        w3r_word value=src[in++];
        if (value!=tag) { dst[out++]=value; continue; }
        if (words-in<2) return CA_Fail("Truncated RLEW run");
        size_t count=src[in++]; value=src[in++];
        if (!count || count>output-out) return CA_Fail("Invalid RLEW run length");
        while (count--) dst[out++]=value;
    }
    return true;
}
bool CA_CacheMap(unsigned number)
{
    memptr head=NULL,file=NULL,pending=NULL,scratch=NULL;
    size_t heads=0,bytes=0; bool ok=false;
    if (number>=60) return CA_Fail("Unsupported WL6 map number");
    if (!CA_ReadDataFile("MAPHEAD.WL6",&head,&heads) || !CA_ReadDataFile("GAMEMAPS.WL6",&file,&bytes)) goto done;
    if (heads<242 || heads>402 || (heads-2)%4) { (void)CA_Fail("Invalid MAPHEAD table"); goto done; }
    const w3r_byte *h=head,*raw=file;
    uint32_t at=CA_LE32(h+2+number*4);
    if (!at || at==UINT32_MAX || at>bytes || bytes-at<38) { (void)CA_Fail("Invalid GAMEMAPS map header offset"); goto done; }
    const w3r_byte *mh=raw+at;
    if (CA_LE16(mh+18)!=64 || CA_LE16(mh+20)!=64) { (void)CA_Fail("Map not 64x64"); goto done; }
    if (!MM_GetPtr(&pending,sizeof(maptype))) { (void)CA_Fail("Map allocation failed"); goto done; }
    maptype *map=pending; map->number=number; memcpy(map->name,mh+22,16); map->name[16]=0;
    for (unsigned plane=0;plane<3;++plane) {
        uint32_t start=CA_LE32(mh+plane*4); size_t length=CA_LE16(mh+12+plane*2);
        if (length<4 || start>bytes || length>bytes-start ||
            (start<(size_t)at+38 && (size_t)start+length>at)) { (void)CA_Fail("Invalid map plane %u bounds",plane); goto done; }
        for (unsigned prior=0;prior<plane;++prior) {
            uint32_t p=CA_LE32(mh+prior*4); size_t n=CA_LE16(mh+12+prior*2);
            if (start<(size_t)p+n && p<(size_t)start+length) { (void)CA_Fail("Overlapping map planes"); goto done; }
        }
        size_t expanded=CA_LE16(raw+start);
        if (expanded<2 || expanded%2 || !MM_GetPtr(&scratch,expanded)) { (void)CA_Fail("Invalid Carmack expanded size"); goto done; }
        if (!CAL_CarmackExpand(raw+start+2,length-2,scratch,expanded/2)) goto done;
        const w3r_word *rlew=scratch;
        if (rlew[0]!=MAPAREA*2) { (void)CA_Fail("Invalid RLEW plane size"); goto done; }
        if (!CA_RLEWexpand(rlew+1,expanded/2-1,map->planes[plane],MAPAREA,CA_LE16(h))) goto done;
        (void)MM_FreePtr(&scratch);
    }
    /* Commit through the stable owner; a failed decode preserves the old map. */
    if (!mapmem && !MM_GetPtr(&mapmem,sizeof(maptype))) { (void)CA_Fail("Map allocation failed"); goto done; }
    memcpy(mapmem,pending,sizeof(maptype)); ok=true;
done:
    if (scratch) (void)MM_FreePtr(&scratch);
    if (pending) (void)MM_FreePtr(&pending);
    if (head) (void)MM_FreePtr(&head);
    if (file) (void)MM_FreePtr(&file);
    return ok;
}
