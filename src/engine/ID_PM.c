/* SPDX-License-Identifier: GPL-2.0-only */
/* ID_PM.C PML_OpenPageFile/PM_GetPage, portable heap cache instead of EMS/XMS. */
#include "ID_PM.h"
#include "ID_FILE.h"
#include "ID_CA.h"
#include "platform/data_file.h"
#include <stdio.h>
#include <string.h>
#define PM_MAX_PAGES 4096
unsigned ChunksInFile,PMSpriteStart,PMSoundStart;
static FILE *PageFile;
static uint32_t offsets[PM_MAX_PAGES];
static uint16_t lengths[PM_MAX_PAGES];
static memptr cache[PM_MAX_PAGES];
void PM_Shutdown(void)
{
    for (unsigned i=0;i<PM_MAX_PAGES;++i) if (cache[i]) (void)MM_FreePtr(&cache[i]);
    if (PageFile) { (void)fclose(PageFile); PageFile=NULL; }
    ChunksInFile=PMSpriteStart=PMSoundStart=0;
}
bool PM_Ready(void) { return PageFile!=NULL; }
bool PM_Startup(void)
{
    w3r_byte header[6],directory[PM_MAX_PAGES*6]; long size;
    PM_Shutdown();
    if (!*CA_DataDirectory()) return CA_Fail("Configure a data directory before reading VSWAP");
    PageFile=CA_OpenDataFile(CA_DataDirectory(),"VSWAP.WL6");
    if (!PageFile) return CA_Fail("Cannot open VSWAP.WL6");
    if (fseek(PageFile,0,SEEK_END) || (size=ftell(PageFile))<6 || size>32*1024*1024 || fseek(PageFile,0,SEEK_SET)) goto bad;
    if (fread(header,1,6,PageFile)!=6) goto bad;
    ChunksInFile=CA_LE16(header); PMSpriteStart=CA_LE16(header+2); PMSoundStart=CA_LE16(header+4);
    if (!ChunksInFile || ChunksInFile>PM_MAX_PAGES || PMSpriteStart<8 || PMSpriteStart>PMSoundStart || PMSoundStart>ChunksInFile) goto bad;
    size_t end=6+(size_t)ChunksInFile*6;
    if (end>(size_t)size || fread(directory,1,end-6,PageFile)!=end-6) goto bad;
    size_t previous=end;
    for (unsigned i=0;i<ChunksInFile;++i) {
        uint32_t off=CA_LE32(directory+i*4); unsigned len=CA_LE16(directory+ChunksInFile*4+i*2);
        offsets[i]=off; lengths[i]=(uint16_t)len;
        if (!off && !len) continue;
        if (!off || !len || off<previous || off>(size_t)size || len>(size_t)size-off || (i<PMSpriteStart && len!=4096)) goto bad;
        previous=(size_t)off+len;
    }
    return true;
bad:
    PM_Shutdown(); return CA_Fail("Malformed/truncated VSWAP directory or page bounds");
}
bool PM_PageCached(unsigned page) { return page<ChunksInFile && cache[page]!=NULL; }
bool PM_PagePresent(unsigned page) { return PageFile && page<ChunksInFile && offsets[page] && lengths[page]; }
const w3r_byte *PM_GetPage(unsigned page,size_t *length)
{
    if (!PageFile || page>=ChunksInFile || !offsets[page] || !lengths[page]) { (void)CA_Fail("Invalid/sparse VSWAP page %u",page); return NULL; }
    if (!cache[page]) {
        if (!MM_GetPtr(&cache[page],lengths[page])) { (void)CA_Fail("VSWAP cache allocation failed"); return NULL; }
        if (fseek(PageFile,(long)offsets[page],SEEK_SET) || fread(cache[page],1,lengths[page],PageFile)!=lengths[page]) {
            (void)MM_FreePtr(&cache[page]); (void)CA_Fail("Truncated VSWAP page"); return NULL;
        }
    }
    if (length) *length=lengths[page];
    return cache[page];
}
bool PM_DecodeSprite(unsigned sprite,w3r_byte pixels[4096],w3r_byte opaque[4096])
{
    size_t length;
    if (sprite>=PMSoundStart-PMSpriteStart || !pixels || !opaque) return CA_Fail("Invalid sprite number");
    const w3r_byte *page=PM_GetPage(PMSpriteStart+sprite,&length);
    if (!page || length<4) return CA_Fail("Truncated sprite header");
    unsigned left=CA_LE16(page),right=CA_LE16(page+2);
    if (left>right || right>=64 || length<4+(right-left+1)*2) return CA_Fail("Invalid sprite column table");
    memset(pixels,0,4096); memset(opaque,0,4096);
    for (unsigned x=left;x<=right;++x) {
        size_t command=CA_LE16(page+4+(x-left)*2); unsigned last=0;
        if (command<4+(right-left+1)*2) return CA_Fail("Sprite command inside column table");
        for (;;) {
            if (command>length || length-command<2) return CA_Fail("Truncated sprite posts");
            unsigned end=CA_LE16(page+command); command+=2;
            if (!end) break;
            if (length-command<4) return CA_Fail("Truncated sprite post");
            unsigned raw=CA_LE16(page+command),start=CA_LE16(page+command+2); command+=4;
            int data=(raw&0x8000) ? (int)raw-65536 : (int)raw;
            if ((start|end)&1U || start>=end || end>128 || start/2<last) return CA_Fail("Invalid sprite post rows");
            for (unsigned y=start/2;y<end/2;++y) {
                int at=data+(int)y;
                if (at<(int)(4+(right-left+1)*2) || (size_t)at>=length) return CA_Fail("Sprite pixel outside page");
                pixels[y*64+x]=page[at]; opaque[y*64+x]=1;
            }
            last=end/2;
        }
    }
    return true;
}
