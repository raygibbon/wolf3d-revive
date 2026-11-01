/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded WL_TEXT.C EndText/ShowArticle/PageLayout for the original WL6
   external end articles. Original source/licensing: docs/LICENSING.md. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#define TEXTROWS 15
static const w3r_byte *article;
static size_t pages[64],articleend;
static unsigned pagecount,currentpage;
void ResetEndText(void) { article=NULL;pagecount=currentpage=0; }
static bool LineNumbers(size_t *cursor,size_t end,int *numbers,unsigned count)
{
    unsigned found=0;size_t at=*cursor;
    while (at<end && article[at]!='\n') {
        if (isdigit(article[at])) {
            if (found>=count) return CA_Fail("Too many article command arguments");
            unsigned value=0;
            do {
                value=value*10+(unsigned)(article[at++]-'0');
                if (value>9999) return CA_Fail("Article command number overflow");
            } while (at<end && isdigit(article[at]));
            numbers[found++]=(int)value;
        } else ++at;
    }
    if (found!=count || at==end) return CA_Fail("Truncated article command");
    *cursor=at+1;return true;
}
static bool DrawArticlePage(void)
{
    size_t at=pages[currentpage],end=currentpage+1<pagecount ? pages[currentpage+1] : articleend;
    VWB_Bar(0,0,320,200,0x11);
    if (!VWB_DrawPic(0,0,H_TOPWINDOWPIC) || !VWB_DrawPic(0,8,H_LEFTWINDOWPIC) ||
        !VWB_DrawPic(312,8,H_RIGHTWINDOWPIC) || !VWB_DrawPic(8,176,H_BOTTOMINFOPIC)) return false;
    while (at<end && article[at]!='\n') ++at;
    if (at==end) return CA_Fail("Truncated article page header");
    ++at;int left[TEXTROWS],right[TEXTROWS];
    for (unsigned i=0;i<TEXTROWS;++i) { left[i]=16;right[i]=304; }
    unsigned row=0;fontnumber=0;fontcolor=0;px=16;py=16;
    while (at<end && row<TEXTROWS) {
        unsigned ch=article[at++];
        if (ch=='^') {
            if (at==end) return CA_Fail("Truncated article escape");
            unsigned command=(unsigned)toupper(article[at++]);int args[4];
            if (command=='E' || command=='P') break;
            if (command=='C') {
                if (end-at<2 || !isxdigit(article[at]) || !isxdigit(article[at+1]))
                    return CA_Fail("Invalid article color");
                unsigned color=0;
                for (unsigned i=0;i<2;++i) {
                    unsigned digit=(unsigned)toupper(article[at++]);
                    color=color*16+(digit>='A' ? digit-'A'+10 : digit-'0');
                }
                fontcolor=(w3r_byte)color;
            } else if (command==';') {
                while (at<end && article[at++]!='\n') {}
            } else if (command=='>') px=160;
            else if (command=='G') {
                if (!LineNumbers(&at,end,args,3) || !VWB_DrawPic(args[1],args[0],args[2])) return false;
                int width=pictable[args[2]-STARTPICS].width,height=pictable[args[2]-STARTPICS].height;
                int top=(args[0]-16)/10,bottom=(args[0]+height-16)/10;
                if (top<0) top=0;
                if (bottom>=TEXTROWS) bottom=TEXTROWS-1;
                bool rhs=args[1]+width/2>160;
                for (int i=top;i<=bottom;++i) {
                    if (rhs) right[i]=args[1]-8;else left[i]=args[1]+width+8;
                }
                if (px<left[row]) px=left[row];
            } else if (command=='L') {
                if (!LineNumbers(&at,end,args,2) || args[0]<16 || args[0]>=166 || args[1]>304)
                    return CA_Fail("Invalid article text location");
                row=(unsigned)(args[0]-16)/10;py=16+(int)row*10;px=args[1];
            } else if (command=='B') {
                if (!LineNumbers(&at,end,args,4) || args[1]>320-args[2] || args[0]>200-args[3])
                    return CA_Fail("Invalid article rectangle");
                VWB_Bar(args[1],args[0],args[2],args[3],0x11);
            } else return CA_Fail("Unsupported article command %c",(int)command);
        } else if (ch=='\n') {
            if (++row==TEXTROWS) break;
            py+=10;px=left[row];
        } else if (ch=='\t') px=(px+8)&0xf8;
        else if (ch>32) {
            char word[80];unsigned length=0;word[length++]=(char)ch;
            while (at<end && article[at]>32) {
                if (length==sizeof(word)-1) return CA_Fail("Article word exceeds original limit");
                word[length++]=(char)article[at++];
            }
            word[length]=0;int width,height;
            if (!VW_MeasurePropString(word,&width,&height)) return false;
            while (px+width>right[row]) {
                if (++row==TEXTROWS) break;
                py+=10;px=left[row];
            }
            if (row==TEXTROWS) break;
            int next=px+width;
            if (!VW_DrawPropString(word)) return false;
            px=next;
            while (at<end && article[at]==' ') { px+=7;++at; }
        }
    }
    char text[32];(void)snprintf(text,sizeof(text),"pg %u of %u",currentpage+1,pagecount);
    px=213;py=183;fontcolor=0x4f;return VW_DrawPropString(text);
}
bool StartEndText(void)
{
    ResetEndText();int chunk=T_ENDART1+gamestate.episode;
    if (!CA_CacheGrChunk(chunk)) return false;
    article=grsegs[chunk];size_t size=grsizes[chunk];
    if (size>30000) return CA_Fail("Article exceeds original scan limit");
    size_t first=0;while (first<size && article[first]<=32) ++first;
    if (size-first<2 || article[first]!='^' || toupper(article[first+1])!='P')
        return CA_Fail("Article is not headed with a page command");
    bool ended=false;
    for (size_t at=first;at+1<size;++at) if (article[at]=='^') {
        int command=toupper(article[at+1]);
        if (command=='P') {
            if (pagecount==64) return CA_Fail("Too many article pages");
            pages[pagecount++]=at;
        } else if (command=='E') { articleend=at;ended=true;break; }
    }
    if (!ended || !pagecount) return CA_Fail("Article lacks an end command");
    return DrawArticlePage();
}
bool StepEndText(bool *done)
{
    *done=false;
    if (IN_ConsumePress(sc_Escape)) { *done=true;return true; }
    unsigned previous=currentpage;
    if (IN_ConsumePress(sc_UpArrow) || IN_ConsumePress(sc_LeftArrow) || IN_ConsumePress(0x49)) {
        if (currentpage) --currentpage;
    } else if (IN_ConsumePress(sc_Enter) || IN_ConsumePress(sc_DownArrow) ||
               IN_ConsumePress(sc_RightArrow) || IN_ConsumePress(0x51)) {
        if (currentpage+1<pagecount) ++currentpage;
    }
    IN_ClearPressed();return previous==currentpage || DrawArticlePage();
}
