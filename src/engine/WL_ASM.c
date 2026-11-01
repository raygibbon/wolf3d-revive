/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_DRAW.C FixedByFrac asm: the second operand is SIGNED MAGNITUDE,
   and only its low 16-bit fraction is multiplied. Explicit 32-bit wrap. */
#include "WL_DEF.h"
#include <math.h>
#include <limits.h>
uint32_t sintable[450];
int32_t finetangent[900];
int pixelangle[VIEWWIDTH];
static int32_t FromBits(uint32_t bits)
{
    return bits<=INT32_MAX ? (int32_t)bits : -(int32_t)(UINT32_MAX-bits)-1;
}
int32_t FixedByFrac(int32_t a,uint32_t b)
{
    uint64_t magnitude=a<0 ? (uint64_t)(-(int64_t)a) : (uint64_t)a;
    uint32_t product=(uint32_t)((magnitude*(b&UINT32_C(65535)))>>16);
    if ((a<0) != ((b&UINT32_C(0x80000000))!=0)) product=0U-product;
    return FromBits(product);
}
int32_t WL_FloorShift(int64_t value,unsigned bits)
{
    if (bits>=63) return value<0 ? -1 : 0;
    int64_t divisor=INT64_C(1)<<bits;
    int64_t result=value/divisor;
    if (value%divisor<0) --result;
    if (result<INT32_MIN) return INT32_MIN;
    if (result>INT32_MAX) return INT32_MAX;
    return (int32_t)result;
}
void BuildTables(void)
{
    /* Same float stores and half-angle tangent sampling as WL_MAIN.C. */
    const double pi=3.141592657;
    const float radtoint=(float)(3600/2/pi);
    for (int i=0;i<450;++i) {
        double tangent=tan((i+0.5)/radtoint);
        finetangent[i]=(int32_t)(tangent*65536);
        finetangent[899-i]=(int32_t)(65536/tangent);
    }
    float angle=0,step=(float)(pi/2/90);
    for (int i=0;i<=90;++i) {
        uint32_t value=(uint32_t)(65536*sin(angle));
        sintable[i]=sintable[180-i]=value;
        if (i+360<450) sintable[i+360]=value;
        if (360-i<450) sintable[360-i]=value|UINT32_C(0x80000000);
        sintable[180+i]=value|UINT32_C(0x80000000);
        angle+=step;
    }
    CalcProjection();
}
void CalcProjection(void)
{
    const float radtoint=(float)(3600/2/3.141592657);
    double facedist=FOCALLENGTH+MINDIST;
    for (int i=0;i<VIEWWIDTH/2;++i) {
        double tangent=(double)(i*65536/VIEWWIDTH)/facedist;
        float angle=(float)atan(tangent);
        int value=(int)(angle*radtoint);
        pixelangle[VIEWWIDTH/2-1-i]=value; pixelangle[VIEWWIDTH/2+i]=-value;
    }
}
