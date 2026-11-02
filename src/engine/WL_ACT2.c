/* SPDX-License-Identifier: GPL-2.0-only */
/* Ported from id Software WOLFSRC/WL_ACT2.C and WL_STATE.C.
   Original source/licensing: WOLFSRC/ and docs/LICENSING.md. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include "ID_US_1.h"
#include <stdlib.h>
#include <math.h>
#define tics SimulationTics()
#define PROJECTILESIZE INT32_C(0xc000)
#define PROJSIZE INT32_C(0x2000)

statetype s_rocket = {true,SPR_ROCKET_1,3,T_Projectile,A_Smoke,&s_rocket};
statetype s_smoke1 = {false,SPR_SMOKE_1,3,NULL,NULL,&s_smoke2};
statetype s_smoke2 = {false,SPR_SMOKE_2,3,NULL,NULL,&s_smoke3};
statetype s_smoke3 = {false,SPR_SMOKE_3,3,NULL,NULL,&s_smoke4};
statetype s_smoke4 = {false,SPR_SMOKE_4,3,NULL,NULL,NULL};
statetype s_boom1 = {false,SPR_BOOM_1,6,NULL,NULL,&s_boom2};
statetype s_boom2 = {false,SPR_BOOM_2,6,NULL,NULL,&s_boom3};
statetype s_boom3 = {false,SPR_BOOM_3,6,NULL,NULL,NULL};
statetype s_schabbstand = {false,SPR_SCHABB_W1,0,T_Stand,NULL,&s_schabbstand};
statetype s_schabbchase1 = {false,SPR_SCHABB_W1,10,T_Schabb,NULL,&s_schabbchase1s};
statetype s_schabbchase1s = {false,SPR_SCHABB_W1,3,NULL,NULL,&s_schabbchase2};
statetype s_schabbchase2 = {false,SPR_SCHABB_W2,8,T_Schabb,NULL,&s_schabbchase3};
statetype s_schabbchase3 = {false,SPR_SCHABB_W3,10,T_Schabb,NULL,&s_schabbchase3s};
statetype s_schabbchase3s = {false,SPR_SCHABB_W3,3,NULL,NULL,&s_schabbchase4};
statetype s_schabbchase4 = {false,SPR_SCHABB_W4,8,T_Schabb,NULL,&s_schabbchase1};
statetype s_schabbdeathcam = {false,SPR_SCHABB_W1,1,NULL,NULL,&s_schabbdie1};
statetype s_schabbdie1 = {false,SPR_SCHABB_W1,10,NULL,A_DeathScream,&s_schabbdie2};
statetype s_schabbdie2 = {false,SPR_SCHABB_W1,10,NULL,NULL,&s_schabbdie3};
statetype s_schabbdie3 = {false,SPR_SCHABB_DIE1,10,NULL,NULL,&s_schabbdie4};
statetype s_schabbdie4 = {false,SPR_SCHABB_DIE2,10,NULL,NULL,&s_schabbdie5};
statetype s_schabbdie5 = {false,SPR_SCHABB_DIE3,10,NULL,NULL,&s_schabbdie6};
statetype s_schabbdie6 = {false,SPR_SCHABB_DEAD,20,NULL,A_StartDeathCam,&s_schabbdie6};
statetype s_schabbshoot1 = {false,SPR_SCHABB_SHOOT1,30,NULL,NULL,&s_schabbshoot2};
statetype s_schabbshoot2 = {false,SPR_SCHABB_SHOOT2,10,NULL,T_SchabbThrow,&s_schabbchase1};
statetype s_needle1 = {false,SPR_HYPO1,6,T_Projectile,NULL,&s_needle2};
statetype s_needle2 = {false,SPR_HYPO2,6,T_Projectile,NULL,&s_needle3};
statetype s_needle3 = {false,SPR_HYPO3,6,T_Projectile,NULL,&s_needle4};
statetype s_needle4 = {false,SPR_HYPO4,6,T_Projectile,NULL,&s_needle1};
statetype s_giftstand = {false,SPR_GIFT_W1,0,T_Stand,NULL,&s_giftstand};
statetype s_giftchase1 = {false,SPR_GIFT_W1,10,T_Gift,NULL,&s_giftchase1s};
statetype s_giftchase1s = {false,SPR_GIFT_W1,3,NULL,NULL,&s_giftchase2};
statetype s_giftchase2 = {false,SPR_GIFT_W2,8,T_Gift,NULL,&s_giftchase3};
statetype s_giftchase3 = {false,SPR_GIFT_W3,10,T_Gift,NULL,&s_giftchase3s};
statetype s_giftchase3s = {false,SPR_GIFT_W3,3,NULL,NULL,&s_giftchase4};
statetype s_giftchase4 = {false,SPR_GIFT_W4,8,T_Gift,NULL,&s_giftchase1};
statetype s_giftdeathcam = {false,SPR_GIFT_W1,1,NULL,NULL,&s_giftdie1};
statetype s_giftdie1 = {false,SPR_GIFT_W1,1,NULL,A_DeathScream,&s_giftdie2};
statetype s_giftdie2 = {false,SPR_GIFT_W1,10,NULL,NULL,&s_giftdie3};
statetype s_giftdie3 = {false,SPR_GIFT_DIE1,10,NULL,NULL,&s_giftdie4};
statetype s_giftdie4 = {false,SPR_GIFT_DIE2,10,NULL,NULL,&s_giftdie5};
statetype s_giftdie5 = {false,SPR_GIFT_DIE3,10,NULL,NULL,&s_giftdie6};
statetype s_giftdie6 = {false,SPR_GIFT_DEAD,20,NULL,A_StartDeathCam,&s_giftdie6};
statetype s_giftshoot1 = {false,SPR_GIFT_SHOOT1,30,NULL,NULL,&s_giftshoot2};
statetype s_giftshoot2 = {false,SPR_GIFT_SHOOT2,10,NULL,T_GiftThrow,&s_giftchase1};
statetype s_fatstand = {false,SPR_FAT_W1,0,T_Stand,NULL,&s_fatstand};
statetype s_fatchase1 = {false,SPR_FAT_W1,10,T_Fat,NULL,&s_fatchase1s};
statetype s_fatchase1s = {false,SPR_FAT_W1,3,NULL,NULL,&s_fatchase2};
statetype s_fatchase2 = {false,SPR_FAT_W2,8,T_Fat,NULL,&s_fatchase3};
statetype s_fatchase3 = {false,SPR_FAT_W3,10,T_Fat,NULL,&s_fatchase3s};
statetype s_fatchase3s = {false,SPR_FAT_W3,3,NULL,NULL,&s_fatchase4};
statetype s_fatchase4 = {false,SPR_FAT_W4,8,T_Fat,NULL,&s_fatchase1};
statetype s_fatdeathcam = {false,SPR_FAT_W1,1,NULL,NULL,&s_fatdie1};
statetype s_fatdie1 = {false,SPR_FAT_W1,1,NULL,A_DeathScream,&s_fatdie2};
statetype s_fatdie2 = {false,SPR_FAT_W1,10,NULL,NULL,&s_fatdie3};
statetype s_fatdie3 = {false,SPR_FAT_DIE1,10,NULL,NULL,&s_fatdie4};
statetype s_fatdie4 = {false,SPR_FAT_DIE2,10,NULL,NULL,&s_fatdie5};
statetype s_fatdie5 = {false,SPR_FAT_DIE3,10,NULL,NULL,&s_fatdie6};
statetype s_fatdie6 = {false,SPR_FAT_DEAD,20,NULL,A_StartDeathCam,&s_fatdie6};
statetype s_fatshoot1 = {false,SPR_FAT_SHOOT1,30,NULL,NULL,&s_fatshoot2};
statetype s_fatshoot2 = {false,SPR_FAT_SHOOT2,10,NULL,T_GiftThrow,&s_fatshoot3};
statetype s_fatshoot3 = {false,SPR_FAT_SHOOT3,10,NULL,T_Shoot,&s_fatshoot4};
statetype s_fatshoot4 = {false,SPR_FAT_SHOOT4,10,NULL,T_Shoot,&s_fatshoot5};
statetype s_fatshoot5 = {false,SPR_FAT_SHOOT3,10,NULL,T_Shoot,&s_fatshoot6};
statetype s_fatshoot6 = {false,SPR_FAT_SHOOT4,10,NULL,T_Shoot,&s_fatchase1};
statetype s_fakestand = {false,SPR_FAKE_W1,0,T_Stand,NULL,&s_fakestand};
statetype s_fakechase1 = {false,SPR_FAKE_W1,10,T_Fake,NULL,&s_fakechase1s};
statetype s_fakechase1s = {false,SPR_FAKE_W1,3,NULL,NULL,&s_fakechase2};
statetype s_fakechase2 = {false,SPR_FAKE_W2,8,T_Fake,NULL,&s_fakechase3};
statetype s_fakechase3 = {false,SPR_FAKE_W3,10,T_Fake,NULL,&s_fakechase3s};
statetype s_fakechase3s = {false,SPR_FAKE_W3,3,NULL,NULL,&s_fakechase4};
statetype s_fakechase4 = {false,SPR_FAKE_W4,8,T_Fake,NULL,&s_fakechase1};
statetype s_fakedie1 = {false,SPR_FAKE_DIE1,10,NULL,A_DeathScream,&s_fakedie2};
statetype s_fakedie2 = {false,SPR_FAKE_DIE2,10,NULL,NULL,&s_fakedie3};
statetype s_fakedie3 = {false,SPR_FAKE_DIE3,10,NULL,NULL,&s_fakedie4};
statetype s_fakedie4 = {false,SPR_FAKE_DIE4,10,NULL,NULL,&s_fakedie5};
statetype s_fakedie5 = {false,SPR_FAKE_DIE5,10,NULL,NULL,&s_fakedie6};
statetype s_fakedie6 = {false,SPR_FAKE_DEAD,0,NULL,NULL,&s_fakedie6};
statetype s_fakeshoot1 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot2};
statetype s_fakeshoot2 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot3};
statetype s_fakeshoot3 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot4};
statetype s_fakeshoot4 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot5};
statetype s_fakeshoot5 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot6};
statetype s_fakeshoot6 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot7};
statetype s_fakeshoot7 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot8};
statetype s_fakeshoot8 = {false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,&s_fakeshoot9};
statetype s_fakeshoot9 = {false,SPR_FAKE_SHOOT,8,NULL,NULL,&s_fakechase1};
statetype s_fire1 = {false,SPR_FIRE1,6,NULL,T_Projectile,&s_fire2};
statetype s_fire2 = {false,SPR_FIRE2,6,NULL,T_Projectile,&s_fire1};
statetype s_mechastand = {false,SPR_MECHA_W1,0,T_Stand,NULL,&s_mechastand};
statetype s_mechachase1 = {false,SPR_MECHA_W1,10,T_Chase,A_MechaSound,&s_mechachase1s};
statetype s_mechachase1s = {false,SPR_MECHA_W1,6,NULL,NULL,&s_mechachase2};
statetype s_mechachase2 = {false,SPR_MECHA_W2,8,T_Chase,NULL,&s_mechachase3};
statetype s_mechachase3 = {false,SPR_MECHA_W3,10,T_Chase,A_MechaSound,&s_mechachase3s};
statetype s_mechachase3s = {false,SPR_MECHA_W3,6,NULL,NULL,&s_mechachase4};
statetype s_mechachase4 = {false,SPR_MECHA_W4,8,T_Chase,NULL,&s_mechachase1};
statetype s_mechadie1 = {false,SPR_MECHA_DIE1,10,NULL,A_DeathScream,&s_mechadie2};
statetype s_mechadie2 = {false,SPR_MECHA_DIE2,10,NULL,NULL,&s_mechadie3};
statetype s_mechadie3 = {false,SPR_MECHA_DIE3,10,NULL,A_HitlerMorph,&s_mechadie4};
statetype s_mechadie4 = {false,SPR_MECHA_DEAD,0,NULL,NULL,&s_mechadie4};
statetype s_mechashoot1 = {false,SPR_MECHA_SHOOT1,30,NULL,NULL,&s_mechashoot2};
statetype s_mechashoot2 = {false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,&s_mechashoot3};
statetype s_mechashoot3 = {false,SPR_MECHA_SHOOT3,10,NULL,T_Shoot,&s_mechashoot4};
statetype s_mechashoot4 = {false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,&s_mechashoot5};
statetype s_mechashoot5 = {false,SPR_MECHA_SHOOT3,10,NULL,T_Shoot,&s_mechashoot6};
statetype s_mechashoot6 = {false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,&s_mechachase1};
statetype s_hitlerchase1 = {false,SPR_HITLER_W1,6,T_Chase,NULL,&s_hitlerchase1s};
statetype s_hitlerchase1s = {false,SPR_HITLER_W1,4,NULL,NULL,&s_hitlerchase2};
statetype s_hitlerchase2 = {false,SPR_HITLER_W2,2,T_Chase,NULL,&s_hitlerchase3};
statetype s_hitlerchase3 = {false,SPR_HITLER_W3,6,T_Chase,NULL,&s_hitlerchase3s};
statetype s_hitlerchase3s = {false,SPR_HITLER_W3,4,NULL,NULL,&s_hitlerchase4};
statetype s_hitlerchase4 = {false,SPR_HITLER_W4,2,T_Chase,NULL,&s_hitlerchase1};
statetype s_hitlerdeathcam = {false,SPR_HITLER_W1,10,NULL,NULL,&s_hitlerdie1};
statetype s_hitlerdie1 = {false,SPR_HITLER_W1,1,NULL,A_DeathScream,&s_hitlerdie2};
statetype s_hitlerdie2 = {false,SPR_HITLER_W1,10,NULL,NULL,&s_hitlerdie3};
statetype s_hitlerdie3 = {false,SPR_HITLER_DIE1,10,NULL,A_Slurpie,&s_hitlerdie4};
statetype s_hitlerdie4 = {false,SPR_HITLER_DIE2,10,NULL,NULL,&s_hitlerdie5};
statetype s_hitlerdie5 = {false,SPR_HITLER_DIE3,10,NULL,NULL,&s_hitlerdie6};
statetype s_hitlerdie6 = {false,SPR_HITLER_DIE4,10,NULL,NULL,&s_hitlerdie7};
statetype s_hitlerdie7 = {false,SPR_HITLER_DIE5,10,NULL,NULL,&s_hitlerdie8};
statetype s_hitlerdie8 = {false,SPR_HITLER_DIE6,10,NULL,NULL,&s_hitlerdie9};
statetype s_hitlerdie9 = {false,SPR_HITLER_DIE7,10,NULL,NULL,&s_hitlerdie10};
statetype s_hitlerdie10 = {false,SPR_HITLER_DEAD,20,NULL,A_StartDeathCam,&s_hitlerdie10};
statetype s_hitlershoot1 = {false,SPR_HITLER_SHOOT1,30,NULL,NULL,&s_hitlershoot2};
statetype s_hitlershoot2 = {false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,&s_hitlershoot3};
statetype s_hitlershoot3 = {false,SPR_HITLER_SHOOT3,10,NULL,T_Shoot,&s_hitlershoot4};
statetype s_hitlershoot4 = {false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,&s_hitlershoot5};
statetype s_hitlershoot5 = {false,SPR_HITLER_SHOOT3,10,NULL,T_Shoot,&s_hitlershoot6};
statetype s_hitlershoot6 = {false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,&s_hitlerchase1};

void SelectRunDir (objtype *ob)
{
	int deltax,deltay;
	dirtype d[3];
	dirtype tdir;

	deltax=player.tilex - ob->tilex;
	deltay=player.tiley - ob->tiley;

	if (deltax<0)
		d[1]= east;
	else
		d[1]= west;
	if (deltay<0)
		d[2]=south;
	else
		d[2]=north;

	if (abs(deltay)>abs(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	ob->dir=d[1];
	if (TryWalk(ob))
		return;     /*either moved forward or attacked*/

	ob->dir=d[2];
	if (TryWalk(ob))
		return;

/* there is no direct path to the player, so pick another direction */

	if (US_RndT()>128) 	/*randomly determine direction of search*/
	{
		for (tdir=north;tdir<=west;tdir++)
		{
			ob->dir=tdir;
			if ( TryWalk(ob) )
				return;
		}
	}
	else
	{
		for (tdir=west;tdir>=north;tdir--)
		{
			ob->dir=tdir;
			if ( TryWalk(ob) )
			  return;
		}
	}

	ob->dir = nodir;		// can't move
}

void T_Schabb (objtype *ob)
{
	int32_t move;
	int	dx,dy,dist;
	bool	dodge;

	dodge = false;
	dx = abs(ob->tilex - player.tilex);
	dy = abs(ob->tiley - player.tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{

			NewState (ob,&s_schabbshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{

			OpenDoor ((unsigned)(-ob->distance-1));
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

void T_Gift (objtype *ob)
{
	int32_t move;
	int	dx,dy,dist;
	bool	dodge;

	dodge = false;
	dx = abs(ob->tilex - player.tilex);
	dy = abs(ob->tiley - player.tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{

			NewState (ob,&s_giftshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{

			OpenDoor ((unsigned)(-ob->distance-1));
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

void T_Fat (objtype *ob)
{
	int32_t move;
	int	dx,dy,dist;
	bool	dodge;

	dodge = false;
	dx = abs(ob->tilex - player.tilex);
	dy = abs(ob->tiley - player.tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{

			NewState (ob,&s_fatshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{

			OpenDoor ((unsigned)(-ob->distance-1));
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

void T_Fake (objtype *ob)
{
	int32_t move;

	if (CheckLine(ob))			// got a shot at player?
	{
		if ( US_RndT() < (tics<<1) )
		{

			NewState (ob,&s_fakeshoot1);
			return;
		}
	}

	if (ob->dir == nodir)
	{
		SelectDodgeDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectDodgeDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

/* Original wall/static sentinel collision, separated from real actors. */
bool ProjectileTryMove(objtype *ob)
{
    int xl=WL_FloorShift((int64_t)ob->x-PROJSIZE,16),xh=WL_FloorShift((int64_t)ob->x+PROJSIZE,16);
    int yl=WL_FloorShift((int64_t)ob->y-PROJSIZE,16),yh=WL_FloorShift((int64_t)ob->y+PROJSIZE,16);
    if (xl<0 || yl<0 || xh>=64 || yh>=64) return false;
    for (int y=yl;y<=yh;++y) for (int x=xl;x<=xh;++x) {
        unsigned tile=tilemap[x][y];
        if (staticblock[x][y] || (tile && tile!=64 &&
            (!(tile&128) || (tile&64) || DoorSolid(tile&63)))) return false;
    }
    return true;
}
void T_Projectile(objtype *ob)
{
    int32_t speed=ob->speed*SimulationTics();
    int32_t dx=FixedByFrac(speed,sintable[ob->angle+90]);
    int32_t dy=-FixedByFrac(speed,sintable[ob->angle]);
    if (dx>0x10000) dx=0x10000;
    if (dy>0x10000) dy=0x10000;
    ob->x+=dx;ob->y+=dy;
    if (!ProjectileTryMove(ob)) {
        if (ob->obclass==rocketobj) { GameSound(MISSILEHITSND,ob);ob->state=&s_boom1; }
        else ob->state=NULL;
        return;
    }
    if (abs(ob->x-player.x)<PROJECTILESIZE && abs(ob->y-player.y)<PROJECTILESIZE) {
        int damage=US_RndT()>>3;
        if (ob->obclass==needleobj) damage+=20;
        else if (ob->obclass==rocketobj) damage+=30;
        TakeDamage(damage,ob);ob->state=NULL;return;
    }
    ob->tilex=ob->x>>16;ob->tiley=ob->y>>16;
}
void T_SchabbThrow (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int		iangle;

	deltax = player.x - ob->x;
	deltay = ob->y - player.y;
	angle = (float)atan2 ((double)deltay,(double)deltax);
	if (angle<0)
		angle = (float)(3.14159265358979323846*2+angle);
	iangle = (int)(angle/(3.14159265358979323846*2)*ANGLES);

	objtype *spawn=GetNewActor();if (!spawn) return;
	spawn->state = &s_needle1;
	spawn->ticcount = 1;

	spawn->tilex = ob->tilex;
	spawn->tiley = ob->tiley;
	spawn->x = ob->x;
	spawn->y = ob->y;
	spawn->obclass = needleobj;
	spawn->dir = nodir;
	spawn->angle = iangle%ANGLES;
	spawn->speed = 0x2000l;

	spawn->flags = FL_NONMARK;
	spawn->active = true;

	GameSound (SCHABBSTHROWSND,spawn);
}

void T_GiftThrow (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int		iangle;

	deltax = player.x - ob->x;
	deltay = ob->y - player.y;
	angle = (float)atan2 ((double)deltay,(double)deltax);
	if (angle<0)
		angle = (float)(3.14159265358979323846*2+angle);
	iangle = (int)(angle/(3.14159265358979323846*2)*ANGLES);

	objtype *spawn=GetNewActor();if (!spawn) return;
	spawn->state = &s_rocket;
	spawn->ticcount = 1;

	spawn->tilex = ob->tilex;
	spawn->tiley = ob->tiley;
	spawn->x = ob->x;
	spawn->y = ob->y;
	spawn->obclass = rocketobj;
	spawn->dir = nodir;
	spawn->angle = iangle%ANGLES;
	spawn->speed = 0x2000l;
	spawn->flags = FL_NONMARK;
	spawn->active = true;

	GameSound (MISSILEFIRESND,spawn);
}

void T_FakeFire (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int		iangle;

	deltax = player.x - ob->x;
	deltay = ob->y - player.y;
	angle = (float)atan2 ((double)deltay,(double)deltax);
	if (angle<0)
		angle = (float)(3.14159265358979323846*2+angle);
	iangle = (int)(angle/(3.14159265358979323846*2)*ANGLES);

	objtype *spawn=GetNewActor();if (!spawn) return;
	spawn->state = &s_fire1;
	spawn->ticcount = 1;

	spawn->tilex = ob->tilex;
	spawn->tiley = ob->tiley;
	spawn->x = ob->x;
	spawn->y = ob->y;
	spawn->dir = nodir;
	spawn->angle = iangle%ANGLES;
	spawn->obclass = fireobj;
	spawn->speed = 0x1200l;
	spawn->flags = FL_NEVERMARK;
	spawn->active = true;

	GameSound (FLAMETHROWERSND,spawn);
}

void A_Smoke(objtype *ob)
{
    objtype *spawn=GetNewActor();if (!spawn) return;
    spawn->state=&s_smoke1;spawn->ticcount=6;
    spawn->tilex=ob->tilex;spawn->tiley=ob->tiley;
    spawn->x=ob->x;spawn->y=ob->y;spawn->obclass=inertobj;
    spawn->active=true;spawn->flags=FL_NEVERMARK;
}
/* Keep the mecha corpse and inherit the original position, reserved move,
   direction and flags; SpawnActor consumes the original randomized start tic. */
void A_HitlerMorph(objtype *ob)
{
    static const int hp[4]={500,700,800,900};
    objtype *spawn=SpawnActor(ob->tilex,ob->tiley,&s_hitlerchase1);
    if (!spawn) return;
    spawn->speed=SPDPATROL*5;spawn->x=ob->x;spawn->y=ob->y;
    spawn->distance=ob->distance;spawn->dir=ob->dir;
    spawn->flags=ob->flags|FL_SHOOTABLE;
    spawn->obclass=realhitlerobj;spawn->hitpoints=hp[gamestate.difficulty];
}
void A_MechaSound(objtype *ob)
{
    if (areabyplayer[ob->areanumber]) GameSound(MECHSTEPSND,ob);
}
void A_Slurpie(objtype *ob) { (void)ob;GameSound(SLURPIESND,NULL); }
extern statetype s_bjrun1,s_bjrun1s,s_bjrun2,s_bjrun3,s_bjrun3s,s_bjrun4;
extern statetype s_bjjump1,s_bjjump2,s_bjjump3,s_bjjump4;
bool ValidateBossSprites(void)
{
    static const statetype *const states[]={
        &s_bjrun1,&s_bjrun1s,&s_bjrun2,&s_bjrun3,&s_bjrun3s,&s_bjrun4,
        &s_bjjump1,&s_bjjump2,&s_bjjump3,&s_bjjump4,
        &s_rocket,
        &s_smoke1,
        &s_smoke2,
        &s_smoke3,
        &s_smoke4,
        &s_boom1,
        &s_boom2,
        &s_boom3,
        &s_schabbstand,
        &s_schabbchase1,
        &s_schabbchase1s,
        &s_schabbchase2,
        &s_schabbchase3,
        &s_schabbchase3s,
        &s_schabbchase4,
        &s_schabbdeathcam,
        &s_schabbdie1,
        &s_schabbdie2,
        &s_schabbdie3,
        &s_schabbdie4,
        &s_schabbdie5,
        &s_schabbdie6,
        &s_schabbshoot1,
        &s_schabbshoot2,
        &s_needle1,
        &s_needle2,
        &s_needle3,
        &s_needle4,
        &s_giftstand,
        &s_giftchase1,
        &s_giftchase1s,
        &s_giftchase2,
        &s_giftchase3,
        &s_giftchase3s,
        &s_giftchase4,
        &s_giftdeathcam,
        &s_giftdie1,
        &s_giftdie2,
        &s_giftdie3,
        &s_giftdie4,
        &s_giftdie5,
        &s_giftdie6,
        &s_giftshoot1,
        &s_giftshoot2,
        &s_fatstand,
        &s_fatchase1,
        &s_fatchase1s,
        &s_fatchase2,
        &s_fatchase3,
        &s_fatchase3s,
        &s_fatchase4,
        &s_fatdeathcam,
        &s_fatdie1,
        &s_fatdie2,
        &s_fatdie3,
        &s_fatdie4,
        &s_fatdie5,
        &s_fatdie6,
        &s_fatshoot1,
        &s_fatshoot2,
        &s_fatshoot3,
        &s_fatshoot4,
        &s_fatshoot5,
        &s_fatshoot6,
        &s_fakestand,
        &s_fakechase1,
        &s_fakechase1s,
        &s_fakechase2,
        &s_fakechase3,
        &s_fakechase3s,
        &s_fakechase4,
        &s_fakedie1,
        &s_fakedie2,
        &s_fakedie3,
        &s_fakedie4,
        &s_fakedie5,
        &s_fakedie6,
        &s_fakeshoot1,
        &s_fakeshoot2,
        &s_fakeshoot3,
        &s_fakeshoot4,
        &s_fakeshoot5,
        &s_fakeshoot6,
        &s_fakeshoot7,
        &s_fakeshoot8,
        &s_fakeshoot9,
        &s_fire1,
        &s_fire2,
        &s_mechastand,
        &s_mechachase1,
        &s_mechachase1s,
        &s_mechachase2,
        &s_mechachase3,
        &s_mechachase3s,
        &s_mechachase4,
        &s_mechadie1,
        &s_mechadie2,
        &s_mechadie3,
        &s_mechadie4,
        &s_mechashoot1,
        &s_mechashoot2,
        &s_mechashoot3,
        &s_mechashoot4,
        &s_mechashoot5,
        &s_mechashoot6,
        &s_hitlerchase1,
        &s_hitlerchase1s,
        &s_hitlerchase2,
        &s_hitlerchase3,
        &s_hitlerchase3s,
        &s_hitlerchase4,
        &s_hitlerdeathcam,
        &s_hitlerdie1,
        &s_hitlerdie2,
        &s_hitlerdie3,
        &s_hitlerdie4,
        &s_hitlerdie5,
        &s_hitlerdie6,
        &s_hitlerdie7,
        &s_hitlerdie8,
        &s_hitlerdie9,
        &s_hitlerdie10,
        &s_hitlershoot1,
        &s_hitlershoot2,
        &s_hitlershoot3,
        &s_hitlershoot4,
        &s_hitlershoot5,
        &s_hitlershoot6,

    };
    w3r_byte pixels[4096],mask[4096];
    if (!PM_DecodeSprite(SPR_DEATHCAM,pixels,mask)) return false;
    for (unsigned i=0;i<sizeof(states)/sizeof(states[0]);++i) {
        const statetype *state=states[i];
        for (unsigned j=0;j<(state->rotate ? 8U : 1U);++j)
            if (!PM_DecodeSprite((unsigned)state->shapenum+j,pixels,mask)) return false;
    }
    return true;
}
