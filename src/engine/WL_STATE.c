/* SPDX-License-Identifier: GPL-2.0-only */
/* Original WL_ACT2.C normal-enemy states; WL_STATE.C movement/sight and
   WL_PLAY.C DoActor. One tic per call. DOS actorat integer sentinels are
   separated into tilemap/staticblock and genuine actor pointers. */
#include "WL_DEF.h"
#include "ID_CA.h"
#include "ID_PM.h"
#include "ID_US_1.h"
#include <stdlib.h>
#include <string.h>
#define tics 1
#define MAPSIZE 64
#define MAPSPOT(x,y,p) (CA_Map()->planes[(p)][(y)*64+(x)])
extern statetype s_grdstand;
extern statetype s_grdpath1;
extern statetype s_grdpath1s;
extern statetype s_grdpath2;
extern statetype s_grdpath3;
extern statetype s_grdpath3s;
extern statetype s_grdpath4;
extern statetype s_grdpain;
extern statetype s_grdpain1;
extern statetype s_grdshoot1;
extern statetype s_grdshoot2;
extern statetype s_grdshoot3;
extern statetype s_grdchase1;
extern statetype s_grdchase1s;
extern statetype s_grdchase2;
extern statetype s_grdchase3;
extern statetype s_grdchase3s;
extern statetype s_grdchase4;
extern statetype s_grddie1;
extern statetype s_grddie2;
extern statetype s_grddie3;
extern statetype s_grddie4;
extern statetype s_dogpath1;
extern statetype s_dogpath1s;
extern statetype s_dogpath2;
extern statetype s_dogpath3;
extern statetype s_dogpath3s;
extern statetype s_dogpath4;
extern statetype s_dogjump1;
extern statetype s_dogjump2;
extern statetype s_dogjump3;
extern statetype s_dogjump4;
extern statetype s_dogjump5;
extern statetype s_dogchase1;
extern statetype s_dogchase1s;
extern statetype s_dogchase2;
extern statetype s_dogchase3;
extern statetype s_dogchase3s;
extern statetype s_dogchase4;
extern statetype s_dogdie1;
extern statetype s_dogdie2;
extern statetype s_dogdie3;
extern statetype s_dogdead;
extern statetype s_ofcstand;
extern statetype s_ofcpath1;
extern statetype s_ofcpath1s;
extern statetype s_ofcpath2;
extern statetype s_ofcpath3;
extern statetype s_ofcpath3s;
extern statetype s_ofcpath4;
extern statetype s_ofcpain;
extern statetype s_ofcpain1;
extern statetype s_ofcshoot1;
extern statetype s_ofcshoot2;
extern statetype s_ofcshoot3;
extern statetype s_ofcchase1;
extern statetype s_ofcchase1s;
extern statetype s_ofcchase2;
extern statetype s_ofcchase3;
extern statetype s_ofcchase3s;
extern statetype s_ofcchase4;
extern statetype s_ofcdie1;
extern statetype s_ofcdie2;
extern statetype s_ofcdie3;
extern statetype s_ofcdie4;
extern statetype s_ofcdie5;
extern statetype s_mutstand;
extern statetype s_mutpath1;
extern statetype s_mutpath1s;
extern statetype s_mutpath2;
extern statetype s_mutpath3;
extern statetype s_mutpath3s;
extern statetype s_mutpath4;
extern statetype s_mutpain;
extern statetype s_mutpain1;
extern statetype s_mutshoot1;
extern statetype s_mutshoot2;
extern statetype s_mutshoot3;
extern statetype s_mutshoot4;
extern statetype s_mutchase1;
extern statetype s_mutchase1s;
extern statetype s_mutchase2;
extern statetype s_mutchase3;
extern statetype s_mutchase3s;
extern statetype s_mutchase4;
extern statetype s_mutdie1;
extern statetype s_mutdie2;
extern statetype s_mutdie3;
extern statetype s_mutdie4;
extern statetype s_mutdie5;
extern statetype s_ssstand;
extern statetype s_sspath1;
extern statetype s_sspath1s;
extern statetype s_sspath2;
extern statetype s_sspath3;
extern statetype s_sspath3s;
extern statetype s_sspath4;
extern statetype s_sspain;
extern statetype s_sspain1;
extern statetype s_ssshoot1;
extern statetype s_ssshoot2;
extern statetype s_ssshoot3;
extern statetype s_ssshoot4;
extern statetype s_ssshoot5;
extern statetype s_ssshoot6;
extern statetype s_ssshoot7;
extern statetype s_ssshoot8;
extern statetype s_ssshoot9;
extern statetype s_sschase1;
extern statetype s_sschase1s;
extern statetype s_sschase2;
extern statetype s_sschase3;
extern statetype s_sschase3s;
extern statetype s_sschase4;
extern statetype s_ssdie1;
extern statetype s_ssdie2;
extern statetype s_ssdie3;
extern statetype s_ssdie4;
statetype s_grdstand = {true,SPR_GRD_S_1,0,T_Stand,NULL,&s_grdstand};
statetype s_grdpath1 = {true,SPR_GRD_W1_1,20,T_Path,NULL,&s_grdpath1s};
statetype s_grdpath1s = {true,SPR_GRD_W1_1,5,NULL,NULL,&s_grdpath2};
statetype s_grdpath2 = {true,SPR_GRD_W2_1,15,T_Path,NULL,&s_grdpath3};
statetype s_grdpath3 = {true,SPR_GRD_W3_1,20,T_Path,NULL,&s_grdpath3s};
statetype s_grdpath3s = {true,SPR_GRD_W3_1,5,NULL,NULL,&s_grdpath4};
statetype s_grdpath4 = {true,SPR_GRD_W4_1,15,T_Path,NULL,&s_grdpath1};
statetype s_grdpain = {2,SPR_GRD_PAIN_1,10,NULL,NULL,&s_grdchase1};
statetype s_grdpain1 = {2,SPR_GRD_PAIN_2,10,NULL,NULL,&s_grdchase1};
statetype s_grdshoot1 = {false,SPR_GRD_SHOOT1,20,NULL,NULL,&s_grdshoot2};
statetype s_grdshoot2 = {false,SPR_GRD_SHOOT2,20,NULL,T_Shoot,&s_grdshoot3};
statetype s_grdshoot3 = {false,SPR_GRD_SHOOT3,20,NULL,NULL,&s_grdchase1};
statetype s_grdchase1 = {true,SPR_GRD_W1_1,10,T_Chase,NULL,&s_grdchase1s};
statetype s_grdchase1s = {true,SPR_GRD_W1_1,3,NULL,NULL,&s_grdchase2};
statetype s_grdchase2 = {true,SPR_GRD_W2_1,8,T_Chase,NULL,&s_grdchase3};
statetype s_grdchase3 = {true,SPR_GRD_W3_1,10,T_Chase,NULL,&s_grdchase3s};
statetype s_grdchase3s = {true,SPR_GRD_W3_1,3,NULL,NULL,&s_grdchase4};
statetype s_grdchase4 = {true,SPR_GRD_W4_1,8,T_Chase,NULL,&s_grdchase1};
statetype s_grddie1 = {false,SPR_GRD_DIE_1,15,NULL,A_DeathScream,&s_grddie2};
statetype s_grddie2 = {false,SPR_GRD_DIE_2,15,NULL,NULL,&s_grddie3};
statetype s_grddie3 = {false,SPR_GRD_DIE_3,15,NULL,NULL,&s_grddie4};
statetype s_grddie4 = {false,SPR_GRD_DEAD,0,NULL,NULL,&s_grddie4};
statetype s_dogpath1 = {true,SPR_DOG_W1_1,20,T_Path,NULL,&s_dogpath1s};
statetype s_dogpath1s = {true,SPR_DOG_W1_1,5,NULL,NULL,&s_dogpath2};
statetype s_dogpath2 = {true,SPR_DOG_W2_1,15,T_Path,NULL,&s_dogpath3};
statetype s_dogpath3 = {true,SPR_DOG_W3_1,20,T_Path,NULL,&s_dogpath3s};
statetype s_dogpath3s = {true,SPR_DOG_W3_1,5,NULL,NULL,&s_dogpath4};
statetype s_dogpath4 = {true,SPR_DOG_W4_1,15,T_Path,NULL,&s_dogpath1};
statetype s_dogjump1 = {false,SPR_DOG_JUMP1,10,NULL,NULL,&s_dogjump2};
statetype s_dogjump2 = {false,SPR_DOG_JUMP2,10,NULL,T_Bite,&s_dogjump3};
statetype s_dogjump3 = {false,SPR_DOG_JUMP3,10,NULL,NULL,&s_dogjump4};
statetype s_dogjump4 = {false,SPR_DOG_JUMP1,10,NULL,NULL,&s_dogjump5};
statetype s_dogjump5 = {false,SPR_DOG_W1_1,10,NULL,NULL,&s_dogchase1};
statetype s_dogchase1 = {true,SPR_DOG_W1_1,10,T_DogChase,NULL,&s_dogchase1s};
statetype s_dogchase1s = {true,SPR_DOG_W1_1,3,NULL,NULL,&s_dogchase2};
statetype s_dogchase2 = {true,SPR_DOG_W2_1,8,T_DogChase,NULL,&s_dogchase3};
statetype s_dogchase3 = {true,SPR_DOG_W3_1,10,T_DogChase,NULL,&s_dogchase3s};
statetype s_dogchase3s = {true,SPR_DOG_W3_1,3,NULL,NULL,&s_dogchase4};
statetype s_dogchase4 = {true,SPR_DOG_W4_1,8,T_DogChase,NULL,&s_dogchase1};
statetype s_dogdie1 = {false,SPR_DOG_DIE_1,15,NULL,A_DeathScream,&s_dogdie2};
statetype s_dogdie2 = {false,SPR_DOG_DIE_2,15,NULL,NULL,&s_dogdie3};
statetype s_dogdie3 = {false,SPR_DOG_DIE_3,15,NULL,NULL,&s_dogdead};
statetype s_dogdead = {false,SPR_DOG_DEAD,15,NULL,NULL,&s_dogdead};
statetype s_ofcstand = {true,SPR_OFC_S_1,0,T_Stand,NULL,&s_ofcstand};
statetype s_ofcpath1 = {true,SPR_OFC_W1_1,20,T_Path,NULL,&s_ofcpath1s};
statetype s_ofcpath1s = {true,SPR_OFC_W1_1,5,NULL,NULL,&s_ofcpath2};
statetype s_ofcpath2 = {true,SPR_OFC_W2_1,15,T_Path,NULL,&s_ofcpath3};
statetype s_ofcpath3 = {true,SPR_OFC_W3_1,20,T_Path,NULL,&s_ofcpath3s};
statetype s_ofcpath3s = {true,SPR_OFC_W3_1,5,NULL,NULL,&s_ofcpath4};
statetype s_ofcpath4 = {true,SPR_OFC_W4_1,15,T_Path,NULL,&s_ofcpath1};
statetype s_ofcpain = {2,SPR_OFC_PAIN_1,10,NULL,NULL,&s_ofcchase1};
statetype s_ofcpain1 = {2,SPR_OFC_PAIN_2,10,NULL,NULL,&s_ofcchase1};
statetype s_ofcshoot1 = {false,SPR_OFC_SHOOT1,6,NULL,NULL,&s_ofcshoot2};
statetype s_ofcshoot2 = {false,SPR_OFC_SHOOT2,20,NULL,T_Shoot,&s_ofcshoot3};
statetype s_ofcshoot3 = {false,SPR_OFC_SHOOT3,10,NULL,NULL,&s_ofcchase1};
statetype s_ofcchase1 = {true,SPR_OFC_W1_1,10,T_Chase,NULL,&s_ofcchase1s};
statetype s_ofcchase1s = {true,SPR_OFC_W1_1,3,NULL,NULL,&s_ofcchase2};
statetype s_ofcchase2 = {true,SPR_OFC_W2_1,8,T_Chase,NULL,&s_ofcchase3};
statetype s_ofcchase3 = {true,SPR_OFC_W3_1,10,T_Chase,NULL,&s_ofcchase3s};
statetype s_ofcchase3s = {true,SPR_OFC_W3_1,3,NULL,NULL,&s_ofcchase4};
statetype s_ofcchase4 = {true,SPR_OFC_W4_1,8,T_Chase,NULL,&s_ofcchase1};
statetype s_ofcdie1 = {false,SPR_OFC_DIE_1,11,NULL,A_DeathScream,&s_ofcdie2};
statetype s_ofcdie2 = {false,SPR_OFC_DIE_2,11,NULL,NULL,&s_ofcdie3};
statetype s_ofcdie3 = {false,SPR_OFC_DIE_3,11,NULL,NULL,&s_ofcdie4};
statetype s_ofcdie4 = {false,SPR_OFC_DIE_4,11,NULL,NULL,&s_ofcdie5};
statetype s_ofcdie5 = {false,SPR_OFC_DEAD,0,NULL,NULL,&s_ofcdie5};
statetype s_mutstand = {true,SPR_MUT_S_1,0,T_Stand,NULL,&s_mutstand};
statetype s_mutpath1 = {true,SPR_MUT_W1_1,20,T_Path,NULL,&s_mutpath1s};
statetype s_mutpath1s = {true,SPR_MUT_W1_1,5,NULL,NULL,&s_mutpath2};
statetype s_mutpath2 = {true,SPR_MUT_W2_1,15,T_Path,NULL,&s_mutpath3};
statetype s_mutpath3 = {true,SPR_MUT_W3_1,20,T_Path,NULL,&s_mutpath3s};
statetype s_mutpath3s = {true,SPR_MUT_W3_1,5,NULL,NULL,&s_mutpath4};
statetype s_mutpath4 = {true,SPR_MUT_W4_1,15,T_Path,NULL,&s_mutpath1};
statetype s_mutpain = {2,SPR_MUT_PAIN_1,10,NULL,NULL,&s_mutchase1};
statetype s_mutpain1 = {2,SPR_MUT_PAIN_2,10,NULL,NULL,&s_mutchase1};
statetype s_mutshoot1 = {false,SPR_MUT_SHOOT1,6,NULL,T_Shoot,&s_mutshoot2};
statetype s_mutshoot2 = {false,SPR_MUT_SHOOT2,20,NULL,NULL,&s_mutshoot3};
statetype s_mutshoot3 = {false,SPR_MUT_SHOOT3,10,NULL,T_Shoot,&s_mutshoot4};
statetype s_mutshoot4 = {false,SPR_MUT_SHOOT4,20,NULL,NULL,&s_mutchase1};
statetype s_mutchase1 = {true,SPR_MUT_W1_1,10,T_Chase,NULL,&s_mutchase1s};
statetype s_mutchase1s = {true,SPR_MUT_W1_1,3,NULL,NULL,&s_mutchase2};
statetype s_mutchase2 = {true,SPR_MUT_W2_1,8,T_Chase,NULL,&s_mutchase3};
statetype s_mutchase3 = {true,SPR_MUT_W3_1,10,T_Chase,NULL,&s_mutchase3s};
statetype s_mutchase3s = {true,SPR_MUT_W3_1,3,NULL,NULL,&s_mutchase4};
statetype s_mutchase4 = {true,SPR_MUT_W4_1,8,T_Chase,NULL,&s_mutchase1};
statetype s_mutdie1 = {false,SPR_MUT_DIE_1,7,NULL,A_DeathScream,&s_mutdie2};
statetype s_mutdie2 = {false,SPR_MUT_DIE_2,7,NULL,NULL,&s_mutdie3};
statetype s_mutdie3 = {false,SPR_MUT_DIE_3,7,NULL,NULL,&s_mutdie4};
statetype s_mutdie4 = {false,SPR_MUT_DIE_4,7,NULL,NULL,&s_mutdie5};
statetype s_mutdie5 = {false,SPR_MUT_DEAD,0,NULL,NULL,&s_mutdie5};
statetype s_ssstand = {true,SPR_SS_S_1,0,T_Stand,NULL,&s_ssstand};
statetype s_sspath1 = {true,SPR_SS_W1_1,20,T_Path,NULL,&s_sspath1s};
statetype s_sspath1s = {true,SPR_SS_W1_1,5,NULL,NULL,&s_sspath2};
statetype s_sspath2 = {true,SPR_SS_W2_1,15,T_Path,NULL,&s_sspath3};
statetype s_sspath3 = {true,SPR_SS_W3_1,20,T_Path,NULL,&s_sspath3s};
statetype s_sspath3s = {true,SPR_SS_W3_1,5,NULL,NULL,&s_sspath4};
statetype s_sspath4 = {true,SPR_SS_W4_1,15,T_Path,NULL,&s_sspath1};
statetype s_sspain = {2,SPR_SS_PAIN_1,10,NULL,NULL,&s_sschase1};
statetype s_sspain1 = {2,SPR_SS_PAIN_2,10,NULL,NULL,&s_sschase1};
statetype s_ssshoot1 = {false,SPR_SS_SHOOT1,20,NULL,NULL,&s_ssshoot2};
statetype s_ssshoot2 = {false,SPR_SS_SHOOT2,20,NULL,T_Shoot,&s_ssshoot3};
statetype s_ssshoot3 = {false,SPR_SS_SHOOT3,10,NULL,NULL,&s_ssshoot4};
statetype s_ssshoot4 = {false,SPR_SS_SHOOT2,10,NULL,T_Shoot,&s_ssshoot5};
statetype s_ssshoot5 = {false,SPR_SS_SHOOT3,10,NULL,NULL,&s_ssshoot6};
statetype s_ssshoot6 = {false,SPR_SS_SHOOT2,10,NULL,T_Shoot,&s_ssshoot7};
statetype s_ssshoot7 = {false,SPR_SS_SHOOT3,10,NULL,NULL,&s_ssshoot8};
statetype s_ssshoot8 = {false,SPR_SS_SHOOT2,10,NULL,T_Shoot,&s_ssshoot9};
statetype s_ssshoot9 = {false,SPR_SS_SHOOT3,10,NULL,NULL,&s_sschase1};
statetype s_sschase1 = {true,SPR_SS_W1_1,10,T_Chase,NULL,&s_sschase1s};
statetype s_sschase1s = {true,SPR_SS_W1_1,3,NULL,NULL,&s_sschase2};
statetype s_sschase2 = {true,SPR_SS_W2_1,8,T_Chase,NULL,&s_sschase3};
statetype s_sschase3 = {true,SPR_SS_W3_1,10,T_Chase,NULL,&s_sschase3s};
statetype s_sschase3s = {true,SPR_SS_W3_1,3,NULL,NULL,&s_sschase4};
statetype s_sschase4 = {true,SPR_SS_W4_1,8,T_Chase,NULL,&s_sschase1};
statetype s_ssdie1 = {false,SPR_SS_DIE_1,15,NULL,A_DeathScream,&s_ssdie2};
statetype s_ssdie2 = {false,SPR_SS_DIE_2,15,NULL,NULL,&s_ssdie3};
statetype s_ssdie3 = {false,SPR_SS_DIE_3,15,NULL,NULL,&s_ssdie4};
statetype s_ssdie4 = {false,SPR_SS_DEAD,0,NULL,NULL,&s_ssdie4};
dirtype opposite[9] =
	{west,southwest,south,southeast,east,northeast,north,northwest,nodir};
dirtype diagonal[9][9] =
{
	{nodir,nodir,northeast,nodir,nodir,nodir,southeast,nodir,nodir},
			{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
 {northeast,nodir,nodir,nodir,northwest,nodir,nodir,nodir,nodir},
			{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
  {nodir,nodir,northwest,nodir,nodir,nodir,southwest,nodir,nodir},
			{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
 {southeast,nodir,nodir,nodir,southwest,nodir,nodir,nodir,nodir},
			{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir},
			{nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir,nodir}
};

/* WL_STATE.C SelectDodgeDir */
void SelectDodgeDir (objtype *ob)
{
 int deltax,deltay,i;
 int absdx,absdy;
 dirtype dirtry[5];
 dirtype turnaround,tdir;
 if (ob->flags & FL_FIRSTATTACK)
 {
  turnaround = nodir;
  ob->flags &= ~FL_FIRSTATTACK;
 }
 else
  turnaround=opposite[ob->dir];
 deltax = player.tilex - ob->tilex;
 deltay = player.tiley - ob->tiley;
 if (deltax>0)
 {
  dirtry[1]= east;
  dirtry[3]= west;
 }
 else
 {
  dirtry[1]= west;
  dirtry[3]= east;
 }
 if (deltay>0)
 {
  dirtry[2]= south;
  dirtry[4]= north;
 }
 else
 {
  dirtry[2]= north;
  dirtry[4]= south;
 }
 absdx = abs(deltax);
 absdy = abs(deltay);
 if (absdx > absdy)
 {
  tdir = dirtry[1];
  dirtry[1] = dirtry[2];
  dirtry[2] = tdir;
  tdir = dirtry[3];
  dirtry[3] = dirtry[4];
  dirtry[4] = tdir;
 }
 if (US_RndT() < 128)
 {
  tdir = dirtry[1];
  dirtry[1] = dirtry[2];
  dirtry[2] = tdir;
  tdir = dirtry[3];
  dirtry[3] = dirtry[4];
  dirtry[4] = tdir;
 }
 dirtry[0] = diagonal [ dirtry[1] ] [ dirtry[2] ];
 for (i=0;i<5;i++)
 {
  if ( dirtry[i] == nodir || dirtry[i] == turnaround)
   continue;
  ob->dir = dirtry[i];
  if (TryWalk(ob))
   return;
 }
 if (turnaround != nodir)
 {
  ob->dir = turnaround;
  if (TryWalk(ob))
   return;
 }
 ob->dir = nodir;
}

/* WL_STATE.C SelectChaseDir */
void SelectChaseDir (objtype *ob)
{
 int deltax,deltay;
 dirtype d[3];
 dirtype tdir, olddir, turnaround;
 olddir=ob->dir;
 turnaround=opposite[olddir];
 deltax=player.tilex - ob->tilex;
 deltay=player.tiley - ob->tiley;
 d[1]=nodir;
 d[2]=nodir;
 if (deltax>0)
  d[1]= east;
 else if (deltax<0)
  d[1]= west;
 if (deltay>0)
  d[2]=south;
 else if (deltay<0)
  d[2]=north;
 if (abs(deltay)>abs(deltax))
 {
  tdir=d[1];
  d[1]=d[2];
  d[2]=tdir;
 }
 if (d[1]==turnaround)
  d[1]=nodir;
 if (d[2]==turnaround)
  d[2]=nodir;
 if (d[1]!=nodir)
 {
  ob->dir=d[1];
  if (TryWalk(ob))
   return;
 }
 if (d[2]!=nodir)
 {
  ob->dir=d[2];
  if (TryWalk(ob))
   return;
 }
 if (olddir!=nodir)
 {
  ob->dir=olddir;
  if (TryWalk(ob))
   return;
 }
 if (US_RndT()>128)
 {
  for (tdir=north;tdir<=west;tdir++)
  {
   if (tdir!=turnaround)
   {
    ob->dir=tdir;
    if ( TryWalk(ob) )
     return;
   }
  }
 }
 else
 {
  for (tdir=west;tdir>=north;tdir--)
  {
   if (tdir!=turnaround)
   {
     ob->dir=tdir;
     if ( TryWalk(ob) )
    return;
   }
  }
 }
 if (turnaround != nodir)
 {
  ob->dir=turnaround;
  if (ob->dir != nodir)
  {
   if ( TryWalk(ob) )
    return;
  }
 }
 ob->dir = nodir;
}

/* WL_STATE.C MoveObj */
void MoveObj (objtype *ob, int32_t move)
{
 int32_t deltax,deltay;
 switch (ob->dir)
 {
 case north:
  ob->y -= move;
  break;
 case northeast:
  ob->x += move;
  ob->y -= move;
  break;
 case east:
  ob->x += move;
  break;
 case southeast:
  ob->x += move;
  ob->y += move;
  break;
 case south:
  ob->y += move;
  break;
 case southwest:
  ob->x -= move;
  ob->y += move;
  break;
 case west:
  ob->x -= move;
  break;
 case northwest:
  ob->x -= move;
  ob->y -= move;
  break;
 case nodir:
  return;
 default:
  return;
 }
 if (areabyplayer[ob->areanumber])
 {
  deltax = ob->x - player.x;
  if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
   goto moveok;
  deltay = ob->y - player.y;
  if (deltay < -MINACTORDIST || deltay > MINACTORDIST)
   goto moveok;
  
  switch (ob->dir)
  {
  case north:
   ob->y += move;
   break;
  case northeast:
   ob->x -= move;
   ob->y += move;
   break;
  case east:
   ob->x -= move;
   break;
  case southeast:
   ob->x -= move;
   ob->y -= move;
   break;
  case south:
   ob->y -= move;
   break;
  case southwest:
   ob->x += move;
   ob->y -= move;
   break;
  case west:
   ob->x += move;
   break;
  case northwest:
   ob->x += move;
   ob->y += move;
   break;
  case nodir:
   return;
  }
  return;
 }
moveok:
 ob->distance -=move;
}

/* WL_STATE.C CheckLine */
bool CheckLine (objtype *ob)
{
 int x1,y1,xt1,yt1,x2,y2,xt2,yt2;
 int x,y;
 int xdist,ydist,xstep,ystep;
 
 int partial,delta;
 int32_t ltemp;
 int xfrac,yfrac,deltafrac;
 unsigned value,intercept;
 x1 = ob->x >> UNSIGNEDSHIFT;
 y1 = ob->y >> UNSIGNEDSHIFT;
 xt1 = x1 >> 8;
 yt1 = y1 >> 8;
 x2 = player.x / 256;
 y2 = player.y / 256;
 xt2 = player.tilex;
 yt2 = player.tiley;
 xdist = abs(xt2-xt1);
 if (xdist > 0)
 {
  if (xt2 > xt1)
  {
   partial = 256-(x1&0xff);
   xstep = 1;
  }
  else
  {
   partial = x1&0xff;
   xstep = -1;
  }
  deltafrac = abs(x2-x1);
  delta = y2-y1;
  ltemp = ((int32_t)delta*256)/deltafrac;
  if (ltemp > 0x7fffl)
   ystep = 0x7fff;
  else if (ltemp < -0x7fffl)
   ystep = -0x7fff;
  else
   ystep = ltemp;
  yfrac = y1 + WL_FloorShift((int64_t)ystep*partial,8);
  x = xt1+xstep;
  xt2 += xstep;
  do
  {
   y = yfrac>>8;
   yfrac += ystep;
   if (x<0 || y<0 || x>=64 || y>=64) return false;
            value = tilemap[x][y];
   x += xstep;
   if (!value)
    continue;
   if (value<128 || value>191)
    return false;
   value &= 63;
   intercept = (uint16_t)(yfrac-ystep/2);
   if (intercept>doorobjlist[value].position)
    return false;
  } while (x != xt2);
 }
 ydist = abs(yt2-yt1);
 if (ydist > 0)
 {
  if (yt2 > yt1)
  {
   partial = 256-(y1&0xff);
   ystep = 1;
  }
  else
  {
   partial = y1&0xff;
   ystep = -1;
  }
  deltafrac = abs(y2-y1);
  delta = x2-x1;
  ltemp = ((int32_t)delta*256)/deltafrac;
  if (ltemp > 0x7fffl)
   xstep = 0x7fff;
  else if (ltemp < -0x7fffl)
   xstep = -0x7fff;
  else
   xstep = ltemp;
  xfrac = x1 + WL_FloorShift((int64_t)xstep*partial,8);
  y = yt1 + ystep;
  yt2 += ystep;
  do
  {
   x = xfrac>>8;
   xfrac += xstep;
   if (x<0 || y<0 || x>=64 || y>=64) return false;
            value = tilemap[x][y];
   y += ystep;
   if (!value)
    continue;
   if (value<128 || value>191)
    return false;
   value &= 63;
   intercept = (uint16_t)(xfrac-xstep/2);
   if (intercept>doorobjlist[value].position)
    return false;
  } while (y != yt2);
 }
 return true;
}

/* WL_STATE.C CheckSight */
bool CheckSight (objtype *ob)
{
 int32_t deltax,deltay;
 if (!areabyplayer[ob->areanumber])
  return false;
 deltax = player.x - ob->x;
 deltay = player.y - ob->y;
 if (deltax > -MINSIGHT && deltax < MINSIGHT
 && deltay > -MINSIGHT && deltay < MINSIGHT)
  return true;
 switch (ob->dir)
 {
 case north:
  if (deltay > 0)
   return false;
  break;
 case east:
  if (deltax < 0)
   return false;
  break;
 case south:
  if (deltay < 0)
   return false;
  break;
 case west:
  if (deltax > 0)
   return false;
  break;
 }
 return CheckLine (ob);
}

/* WL_STATE.C FirstSighting */
void FirstSighting (objtype *ob)
{
 switch (ob->obclass)
 {
 case guardobj:

  NewState (ob,&s_grdchase1);
  ob->speed *= 3;
  break;
 case officerobj:

  NewState (ob,&s_ofcchase1);
  ob->speed *= 5;
  break;
 case mutantobj:
  NewState (ob,&s_mutchase1);
  ob->speed *= 3;
  break;
 case ssobj:

  NewState (ob,&s_sschase1);
  ob->speed *= 4;
  break;
 case dogobj:

  NewState (ob,&s_dogchase1);
  ob->speed *= 2;
  break;
 }
 if (ob->distance < 0)
  ob->distance = 0;
 ob->flags |= FL_ATTACKMODE|FL_FIRSTATTACK;
}

/* WL_STATE.C SightPlayer */
bool SightPlayer (objtype *ob)
{
 if (ob->flags & FL_ATTACKMODE) return false;
 if (ob->temp2)
 {
  ob->temp2 -= tics;
  if (ob->temp2 > 0)
   return false;
  ob->temp2 = 0;
 }
 else
 {
  if (!areabyplayer[ob->areanumber])
   return false;
  if (ob->flags & FL_AMBUSH)
  {
   if (!CheckSight (ob))
    return false;
   ob->flags &= ~FL_AMBUSH;
  }
  else
  {
   if (!madenoise && !CheckSight (ob))
    return false;
  }
  switch (ob->obclass)
  {
  case guardobj:
   ob->temp2 = 1+US_RndT()/4;
   break;
  case officerobj:
   ob->temp2 = 2;
   break;
  case mutantobj:
   ob->temp2 = 1+US_RndT()/6;
   break;
  case ssobj:
   ob->temp2 = 1+US_RndT()/6;
   break;
  case dogobj:
   ob->temp2 = 1+US_RndT()/8;
   break;
  case bossobj:
  case schabbobj:
  case fakeobj:
  case mechahitlerobj:
  case realhitlerobj:
  case gretelobj:
  case giftobj:
  case fatobj:
  case spectreobj:
  case angelobj:
  case transobj:
  case uberobj:
  case willobj:
  case deathobj:
   ob->temp2 = 1;
   break;
  }
  return false;
 }
 FirstSighting (ob);
 return true;
}

/* WL_STATE.C DamageActor */
void DamageActor (objtype *ob, int damage)
{
 if (!ob || !(ob->flags&FL_SHOOTABLE) || damage<0) return;
 /* Damage is bounded by the original weapon rules. Saturate API outliers
    before the original surprise multiplier, rather than overflowing C. */
 if (damage>32767) damage=32767;
 madenoise = true;
 if ( !(ob->flags & FL_ATTACKMODE) )
  damage <<= 1;
 ob->hitpoints -= damage;
 if (ob->hitpoints<=0)
  KillActor (ob);
 else
 {
  if (! (ob->flags & FL_ATTACKMODE) )
   FirstSighting (ob);
  switch (ob->obclass)
  {
  case guardobj:
   if (ob->hitpoints&1)
    NewState (ob,&s_grdpain);
   else
    NewState (ob,&s_grdpain1);
   break;
  case officerobj:
   if (ob->hitpoints&1)
    NewState (ob,&s_ofcpain);
   else
    NewState (ob,&s_ofcpain1);
   break;
  case mutantobj:
   if (ob->hitpoints&1)
    NewState (ob,&s_mutpain);
   else
    NewState (ob,&s_mutpain1);
   break;
  case ssobj:
   if (ob->hitpoints&1)
    NewState (ob,&s_sspain);
   else
    NewState (ob,&s_sspain1);
   break;
  }
 }
}

/* WL_STATE.C KillActor */
void KillActor (objtype *ob)
{
 int tilex,tiley;
 tilex = ob->tilex = ob->x >> TILESHIFT;
 tiley = ob->tiley = ob->y >> TILESHIFT;
 switch (ob->obclass)
 {
 case guardobj:
  GivePoints (100);
  NewState (ob,&s_grddie1);
  PlaceItemType (bo_clip2,tilex,tiley);
  break;
 case officerobj:
  GivePoints (400);
  NewState (ob,&s_ofcdie1);
  PlaceItemType (bo_clip2,tilex,tiley);
  break;
 case mutantobj:
  GivePoints (700);
  NewState (ob,&s_mutdie1);
  PlaceItemType (bo_clip2,tilex,tiley);
  break;
 case ssobj:
  GivePoints (500);
  NewState (ob,&s_ssdie1);
  if (gamestate.bestweapon < wp_machinegun)
   PlaceItemType (bo_machinegun,tilex,tiley);
  else
   PlaceItemType (bo_clip2,tilex,tiley);
  break;
 case dogobj:
  GivePoints (200);
  NewState (ob,&s_dogdie1);
  break;
 }
 gamestate.killcount++;
 ob->flags &= ~FL_SHOOTABLE;
 for (int x=0;x<64;++x) for (int y=0;y<64;++y)
        if (actorat[x][y]==ob) actorat[x][y]=NULL;
 ob->flags |= FL_NONMARK;
}

/* WL_ACT2.C T_Stand */
void T_Stand (objtype *ob)
{
 SightPlayer (ob);
}

/* WL_ACT2.C T_Chase */
void T_Chase (objtype *ob)
{
 int32_t move;
 int dx,dy,dist,chance;
 bool dodge;
 if (gamestate.victoryflag)
  return;
 dodge = false;
 if (CheckLine(ob))
 {
  dx = abs(ob->tilex - player.tilex);
  dy = abs(ob->tiley - player.tiley);
  dist = dx>dy ? dx : dy;
  if (!dist || (dist==1 && ob->distance<0x4000) )
   chance = 300;
  else
   chance = (tics<<4)/dist;
  if ( US_RndT()<chance)
  {
   switch (ob->obclass)
   {
   case guardobj:
    NewState (ob,&s_grdshoot1);
    break;
   case officerobj:
    NewState (ob,&s_ofcshoot1);
    break;
   case mutantobj:
    NewState (ob,&s_mutshoot1);
    break;
   case ssobj:
    NewState (ob,&s_ssshoot1);
    break;
   }
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
   return;
 }
 move = ob->speed*tics;
 while (move)
 {
  if (ob->distance < 0)
  {
   OpenDoor ((unsigned)(-ob->distance-1));
   if (doorobjlist[-ob->distance-1].action != dr_open)
    return;
   ob->distance = TILEGLOBAL;
  }
  if (move < ob->distance)
  {
   MoveObj (ob,move);
   break;
  }
  ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
  ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;
  move -= ob->distance;
  if (dodge)
   SelectDodgeDir (ob);
  else
   SelectChaseDir (ob);
  if (ob->dir == nodir)
   return;
 }
}

/* WL_ACT2.C T_DogChase */
void T_DogChase (objtype *ob)
{
 int32_t move;
 
 int32_t dx,dy;
 if (ob->dir == nodir)
 {
  SelectDodgeDir (ob);
  if (ob->dir == nodir)
   return;
 }
 move = ob->speed*tics;
 while (move)
 {
  dx = player.x - ob->x;
  if (dx<0)
   dx = -dx;
  dx -= move;
  if (dx <= MINACTORDIST)
  {
   dy = player.y - ob->y;
   if (dy<0)
    dy = -dy;
   dy -= move;
   if (dy <= MINACTORDIST)
   {
    NewState (ob,&s_dogjump1);
    return;
   }
  }
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
   return;
 }
}

/* WL_ACT2.C SelectPathDir */
void SelectPathDir (objtype *ob)
{
 int spot;
 spot = MAPSPOT(ob->tilex,ob->tiley,1)-ICONARROWS;
 if (spot>=0 && spot<8)
 {
  ob->dir = spot;
 }
 ob->distance = TILEGLOBAL;
 if (!TryWalk (ob))
  ob->dir = nodir;
}

/* WL_ACT2.C T_Path */
void T_Path (objtype *ob)
{
 int32_t move;
 
 if (SightPlayer (ob))
  return;
 if (ob->dir == nodir)
 {
  SelectPathDir (ob);
  if (ob->dir == nodir)
   return;
 }
 move = ob->speed*tics;
 while (move)
 {
  if (ob->distance < 0)
  {
   OpenDoor ((unsigned)(-ob->distance-1));
   if (doorobjlist[-ob->distance-1].action != dr_open)
    return;
   ob->distance = TILEGLOBAL;
  }
  if (move < ob->distance)
  {
   MoveObj (ob,move);
   break;
  }
  ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
  ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;
  move -= ob->distance;
  SelectPathDir (ob);
  if (ob->dir == nodir)
   return;
 }
}

/* WL_ACT2.C T_Shoot */
void T_Shoot (objtype *ob)
{
 int dx,dy,dist,hitchance,damage;
 
 hitchance = 128;
 if (!areabyplayer[ob->areanumber])
  return;
 if (!CheckLine (ob))
   return;
 dx = abs(ob->tilex - player.tilex);
 dy = abs(ob->tiley - player.tiley);
 dist = dx>dy ? dx:dy;
 if (ob->obclass == ssobj || ob->obclass == bossobj)
  dist = dist*2/3;
 if (thrustspeed >= RUNSPEED)
 {
  if (ob->flags&FL_VISABLE)
   hitchance = 160-dist*16;
  else
   hitchance = 160-dist*8;
 }
 else
 {
  if (ob->flags&FL_VISABLE)
   hitchance = 256-dist*16;
  else
   hitchance = 256-dist*8;
 }
 if (US_RndT()<hitchance)
 {
  if (dist<2)
   damage = US_RndT()>>2;
  else if (dist<4)
   damage = US_RndT()>>3;
  else
   damage = US_RndT()>>4;
  TakeDamage (damage,ob);
 }
}

/* WL_ACT2.C T_Bite */
void T_Bite (objtype *ob)
{
 int32_t dx,dy;
 

 dx = player.x - ob->x;
 if (dx<0)
  dx = -dx;
 dx -= TILEGLOBAL;
 if (dx <= MINACTORDIST)
 {
  dy = player.y - ob->y;
  if (dy<0)
   dy = -dy;
  dy -= TILEGLOBAL;
  if (dy <= MINACTORDIST)
  {
     if (US_RndT()<180)
     {
      TakeDamage (US_RndT()>>4,ob);
      return;
     }
  }
 }
 return;
}

/* WL_PLAY.C DoActor */
void DoActor (objtype *ob)
{
 void (*think)(objtype *);
 if (!ob->active && !areabyplayer[ob->areanumber])
  return;
 if (!(ob->flags&(FL_NONMARK|FL_NEVERMARK)) )
  actorat[ob->tilex][ob->tiley] = NULL;
 if (!ob->ticcount)
 {
  think = ob->state->think;
  if (think)
  {
   think (ob);
   if (!ob->state)
   {
    ob->state=NULL;
    return;
   }
  }
  if (ob->flags&FL_NEVERMARK)
   return;
  if ( (ob->flags&FL_NONMARK) && actorat[ob->tilex][ob->tiley])
   return;
  actorat[ob->tilex][ob->tiley] = ob;
  return;
 }
 ob->ticcount-=tics;
 while ( ob->ticcount <= 0)
 {
  think = ob->state->action;
  if (think)
  {
   think (ob);
   if (!ob->state)
   {
    ob->state=NULL;
    return;
   }
  }
  ob->state = ob->state->next;
  if (!ob->state)
  {
   ob->state=NULL;
   return;
  }
  if (!ob->state->tictime)
  {
   ob->ticcount = 0;
   goto think;
  }
  ob->ticcount += ob->state->tictime;
 }
think:
 think = ob->state->think;
 if (think)
 {
  think (ob);
  if (!ob->state)
  {
   ob->state=NULL;
   return;
  }
 }
 if (ob->flags&FL_NEVERMARK)
  return;
 if ( (ob->flags&FL_NONMARK) && actorat[ob->tilex][ob->tiley])
  return;
 actorat[ob->tilex][ob->tiley] = ob;
}

/* Validate every reachable normal-enemy animation before entering play. */
bool ValidateActorSprites(void)
{
    static const statetype *const states[]={&s_grdstand,&s_grdpath1,&s_grdpath1s,&s_grdpath2,&s_grdpath3,&s_grdpath3s,&s_grdpath4,&s_grdpain,&s_grdpain1,&s_grdshoot1,&s_grdshoot2,&s_grdshoot3,&s_grdchase1,&s_grdchase1s,&s_grdchase2,&s_grdchase3,&s_grdchase3s,&s_grdchase4,&s_grddie1,&s_grddie2,&s_grddie3,&s_grddie4,&s_dogpath1,&s_dogpath1s,&s_dogpath2,&s_dogpath3,&s_dogpath3s,&s_dogpath4,&s_dogjump1,&s_dogjump2,&s_dogjump3,&s_dogjump4,&s_dogjump5,&s_dogchase1,&s_dogchase1s,&s_dogchase2,&s_dogchase3,&s_dogchase3s,&s_dogchase4,&s_dogdie1,&s_dogdie2,&s_dogdie3,&s_dogdead,&s_ofcstand,&s_ofcpath1,&s_ofcpath1s,&s_ofcpath2,&s_ofcpath3,&s_ofcpath3s,&s_ofcpath4,&s_ofcpain,&s_ofcpain1,&s_ofcshoot1,&s_ofcshoot2,&s_ofcshoot3,&s_ofcchase1,&s_ofcchase1s,&s_ofcchase2,&s_ofcchase3,&s_ofcchase3s,&s_ofcchase4,&s_ofcdie1,&s_ofcdie2,&s_ofcdie3,&s_ofcdie4,&s_ofcdie5,&s_mutstand,&s_mutpath1,&s_mutpath1s,&s_mutpath2,&s_mutpath3,&s_mutpath3s,&s_mutpath4,&s_mutpain,&s_mutpain1,&s_mutshoot1,&s_mutshoot2,&s_mutshoot3,&s_mutshoot4,&s_mutchase1,&s_mutchase1s,&s_mutchase2,&s_mutchase3,&s_mutchase3s,&s_mutchase4,&s_mutdie1,&s_mutdie2,&s_mutdie3,&s_mutdie4,&s_mutdie5,&s_ssstand,&s_sspath1,&s_sspath1s,&s_sspath2,&s_sspath3,&s_sspath3s,&s_sspath4,&s_sspain,&s_sspain1,&s_ssshoot1,&s_ssshoot2,&s_ssshoot3,&s_ssshoot4,&s_ssshoot5,&s_ssshoot6,&s_ssshoot7,&s_ssshoot8,&s_ssshoot9,&s_sschase1,&s_sschase1s,&s_sschase2,&s_sschase3,&s_sschase3s,&s_sschase4,&s_ssdie1,&s_ssdie2,&s_ssdie3,&s_ssdie4};
    w3r_byte pixels[4096],mask[4096];
    for (unsigned i=0;i<sizeof(states)/sizeof(states[0]);++i) {
        const statetype *state=states[i];
        unsigned rotations=state->rotate==1 ? 8U : state->rotate==2 ? 2U : 1U;
        for (unsigned j=0;j<rotations;++j) {
            unsigned shape=(unsigned)state->shapenum+(state->rotate==2 ? j*4 : j);
            if (!PM_DecodeSprite(shape,pixels,mask)) return false;
        }
    }
    return true;
}
