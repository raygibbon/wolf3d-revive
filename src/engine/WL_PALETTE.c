/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_PLAY.C InitRedShifts/StartBonusFlash/StartDamageFlash/UpdatePaletteShifts.
   The original integer DAC interpolation and red-over-white priority remain. */
#include "WL_DEF.h"
#include "ID_VL.h"
static w3r_byte redshifts[6][768],whiteshifts[3][768];
static int damagecount,bonuscount;
static bool initialized,palshifted;
void ClearPaletteShifts(void)
{
    damagecount=bonuscount=0;palshifted=false;
    if (initialized) return;
    for (unsigned i=1;i<=6;++i) for (unsigned j=0;j<768;++j) {
        int base=gamepal[j],target=j%3==0 ? 64 : 0;
        redshifts[i-1][j]=(w3r_byte)(base+(target-base)*(int)i/8);
    }
    for (unsigned i=1;i<=3;++i) for (unsigned j=0;j<768;++j) {
        int base=gamepal[j],target=j%3==0 ? 64 : j%3==1 ? 62 : 0;
        whiteshifts[i-1][j]=(w3r_byte)(base+(target-base)*(int)i/20);
    }
    initialized=true;
}
void StartBonusFlash(void) { bonuscount=18; }
void StartDamageFlash(int damage)
{
    if (damage>0) damagecount=damage>1000-damagecount ? 1000 : damagecount+damage;
}
void FinishPaletteShifts(void)
{
    if (palshifted) { (void)VL_SetPalette(gamepal,768);palshifted=false; }
}
bool UpdatePaletteShifts(void)
{
    int white=bonuscount ? bonuscount/6+1 : 0,red=damagecount ? damagecount/10+1 : 0;
    if (white>3) white=3;
    if (red>6) red=6;
    if (bonuscount) { bonuscount-=SimulationTics();if(bonuscount<0) bonuscount=0; }
    if (damagecount) { damagecount-=SimulationTics();if(damagecount<0) damagecount=0; }
    if (red || white) {
        palshifted=true;
        return VL_SetPalette(red ? redshifts[red-1] : whiteshifts[white-1],768);
    }
    FinishPaletteShifts();return true;
}
