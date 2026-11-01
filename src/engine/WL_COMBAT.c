/* SPDX-License-Identifier: GPL-2.0-only */
/* WL_AGENT.C inventory, attackinfo, T_Attack and status-bar drawing. Audio
   calls are omitted until the sound manager is ported. No gameplay assets. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_IN.h"
#include "ID_VH.h"
#include "ID_US_1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
objtype *killerobj;
static int facecount;
static bool attackheld,gotgatgun,lastgatgun;
static gametype laststatus;
static bool statusvalid;
void ResetStatus(void) { ClearPaletteShifts();facecount=0;killerobj=NULL;attackheld=gotgatgun=false;statusvalid=false; }
void HealSelf(int points) { gotgatgun=false;gamestate.health+=points;if (gamestate.health>100) gamestate.health=100; }
void GiveAmmo(int ammo)
{
    if (!gamestate.ammo && !gamestate.attackframe) gamestate.weapon=gamestate.chosenweapon;
    gamestate.ammo+=ammo;if (gamestate.ammo>99) gamestate.ammo=99;
}
static void GiveExtraMan(void) { if (gamestate.lives<9) ++gamestate.lives; }
void GivePoints(int points)
{
    gamestate.score+=points;
    while (gamestate.score>=gamestate.nextextra) { gamestate.nextextra+=40000;GiveExtraMan(); }
}
void GiveWeapon(int weapon)
{
    GiveAmmo(6);
    if (gamestate.bestweapon<weapon) gamestate.bestweapon=gamestate.weapon=gamestate.chosenweapon=weapon;
}
void TakeDamage(int points,objtype *attacker)
{
    if (gamestate.died || gamestate.victoryflag || points<0) return;
    if (gamestate.difficulty==0) points/=4;
    gotgatgun=false;
    StartDamageFlash(points);
    gamestate.health-=points;
    if (gamestate.health<=0) { gamestate.health=0;gamestate.died=true;killerobj=attacker; }
}
void GetBonus(statobj_t *ob)
{
    switch(ob->itemnumber) {
    case bo_firstaid: if (gamestate.health==100) return;HealSelf(25);break;
    case bo_key1: case bo_key2: case bo_key3: case bo_key4: gamestate.keys|=1<<(ob->itemnumber-bo_key1);break;
    case bo_cross: GivePoints(100);++gamestate.treasurecount;break;
    case bo_chalice: GivePoints(500);++gamestate.treasurecount;break;
    case bo_bible: GivePoints(1000);++gamestate.treasurecount;break;
    case bo_crown: GivePoints(5000);++gamestate.treasurecount;break;
    case bo_clip: case bo_clip2: if (gamestate.ammo==99) return;GiveAmmo(ob->itemnumber==bo_clip?8:4);break;
    case bo_machinegun: GiveWeapon(wp_machinegun);break;
    case bo_chaingun: GiveWeapon(wp_chaingun);gotgatgun=true;facecount=0;break;
    case bo_fullheal: HealSelf(99);GiveAmmo(25);GiveExtraMan();++gamestate.treasurecount;break;
    case bo_food: if (gamestate.health==100) return;HealSelf(10);break;
    case bo_alpo: if (gamestate.health==100) return;HealSelf(4);break;
    case bo_gibs: if (gamestate.health>10) return;HealSelf(1);break;
    default: return;
    }
    StartBonusFlash();
    ob->shapenum=-1;
}
void UpdateFace(void)
{
    ++facecount;
    if (facecount>US_RndT()) {
        gamestate.faceframe=US_RndT()>>6;
        if (gamestate.faceframe==3) gamestate.faceframe=1;
        facecount=0;gotgatgun=false;
    }
}
static bool StatusDrawPic(int x,int y,int pic) { return VWB_DrawPic(x*8,160+y,pic); }
static bool LatchNumber(int x,int y,unsigned width,int number)
{
    char text[24];int n=snprintf(text,sizeof(text),"%d",number);
    if (n<0 || (size_t)n>=sizeof(text)) return CA_Fail("Status number overflow");
    unsigned length=(unsigned)n;
    while (length<width) { if (!StatusDrawPic(x++,y,N_BLANKPIC)) return false;--width; }
    unsigned at=length<=width ? 0 : length-width;
    while (at<length) if (!StatusDrawPic(x++,y,N_0PIC+text[at++]-'0')) return false;
    return true;
}
bool DrawStatus(void)
{
    if (statusvalid && gotgatgun==lastgatgun && memcmp(&laststatus,&gamestate,sizeof(gamestate))==0) return true;
    if (!LatchNumber(2,16,2,gamestate.mapon+1) || !LatchNumber(14,16,1,gamestate.lives) ||
        !LatchNumber(6,16,6,gamestate.score) || !LatchNumber(21,16,3,gamestate.health) ||
        !LatchNumber(27,16,2,gamestate.ammo) || !StatusDrawPic(32,8,KNIFEPIC+gamestate.weapon) ||
        !StatusDrawPic(30,4,(gamestate.keys&1)?GOLDKEYPIC:NOKEYPIC) ||
        !StatusDrawPic(30,20,(gamestate.keys&2)?SILVERKEYPIC:NOKEYPIC) ||
        !StatusDrawPic(17,4,gotgatgun ? GOTGATLINGPIC : gamestate.health ? FACE1APIC+3*((100-gamestate.health)/16)+gamestate.faceframe : FACE8APIC)) return false;
    laststatus=gamestate;lastgatgun=gotgatgun;statusvalid=true;return true;
}
void	KnifeAttack (objtype *ob)
{
	objtype *check,*closest;
	int32_t	dist;

	dist = 0x7fffffff;
	closest = NULL;
	for (check=ob->next ; check ; check=check->next)
		if ( (check->flags & FL_SHOOTABLE)
		&& (check->flags & FL_VISABLE)
		&& abs (check->viewx-(VIEWWIDTH/2)) < (VIEWWIDTH/10)
		)
		{
			if (check->transx < dist)
			{
				dist = check->transx;
				closest = check;
			}
		}
	if (!closest || dist> 0x18000l)
	{
		return;
	}
	DamageActor (closest,US_RndT() >> 4);
}
void	GunAttack (objtype *ob)
{
	objtype *check,*closest,*oldclosest;
	int		damage;
	int		dx,dy,dist;
	int32_t	viewdist;
	madenoise = true;
	viewdist = 0x7fffffffl;
	closest = NULL;
	while (1)
	{
		oldclosest = closest;
		for (check=ob->next ; check ; check=check->next)
			if ( (check->flags & FL_SHOOTABLE)
			&& (check->flags & FL_VISABLE)
			&& abs (check->viewx-(VIEWWIDTH/2)) < (VIEWWIDTH/10)
			)
			{
				if (check->transx < viewdist)
				{
					viewdist = check->transx;
					closest = check;
				}
			}
		if (closest == oldclosest)
			return;						
		if (CheckLine(closest))
			break;
	}
	dx = abs(closest->tilex - player.tilex);
	dy = abs(closest->tiley - player.tiley);
	dist = dx>dy ? dx:dy;
	if (dist<2)
		damage = US_RndT() / 4;
	else if (dist<4)
		damage = US_RndT() / 6;
	else
	{
		if ( (US_RndT() / 12) < dist)		
			return;
		damage = US_RndT() / 6;
	}
	DamageActor (closest,damage);
}
struct atkinf { int tics,attack,frame; };
static const struct atkinf attackinfo[4][4]={
    {{6,0,1},{6,2,2},{6,0,3},{6,-1,4}},
    {{6,0,1},{6,1,2},{6,0,3},{6,-1,4}},
    {{6,0,1},{6,1,2},{6,3,3},{6,-1,4}},
    {{6,0,1},{6,1,2},{6,4,3},{6,-1,4}}
};
/* WL_AGENT.C CheckWeaponChange: first ready key wins; empty ammunition
   preserves the chosen gun so GiveAmmo can restore it from the knife. */
void CheckWeaponChange(void)
{
    if (!gamestate.ammo) return;
    for (int weapon=wp_knife;weapon<=gamestate.bestweapon;++weapon) {
        unsigned scan=(unsigned)(0x02+weapon);
        if (Keyboard[scan] || IN_PressPending(scan)) {
            gamestate.weapon=gamestate.chosenweapon=weapon;return;
        }
    }
}
void T_Attack(bool attack)
{
    if (!gamestate.attackframe) {
        CheckWeaponChange();
        if (attack && !attackheld) {
            gamestate.attackframe=1;gamestate.attackcount=6;gamestate.weaponframe=1;
        }
        attackheld=attack;return;
    }
    attackheld=attack;
    if (--gamestate.attackcount>0) return;
    /* attackframe is stored +1 so zero remains the original non-attack state. */
    const struct atkinf *cur=&attackinfo[gamestate.weapon][gamestate.attackframe-1];
    switch (cur->attack) {
    case -1:
        gamestate.weapon=gamestate.ammo ? gamestate.chosenweapon : wp_knife;
        gamestate.attackframe=gamestate.weaponframe=0;return;
    case 4:
        if (!gamestate.ammo) break;
        if (attack) gamestate.attackframe-=2;
        /* fall through */
    case 1:
        if (!gamestate.ammo) { ++gamestate.attackframe;break; }
        GunAttack(&player);--gamestate.ammo;break;
    case 2: KnifeAttack(&player);break;
    case 3: if (gamestate.ammo && attack) gamestate.attackframe-=2;break;
    default: break;
    }
    gamestate.attackcount+=cur->tics;++gamestate.attackframe;
    gamestate.weaponframe=attackinfo[gamestate.weapon][gamestate.attackframe-1].frame;
}
