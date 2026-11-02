/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_ACTOR_H
#define WL_ACTOR_H
#include "WL_SPRITES.h"
#include "WL_SOUND.h"
#define MAXACTORS 150
#define MAXSTATS 400
#define FL_SHOOTABLE 1U
#define FL_VISABLE 8U
#define FL_ATTACKMODE 16U
#define FL_FIRSTATTACK 32U
#define FL_AMBUSH 64U
#define FL_NONMARK 128U
#define FL_NEVERMARK 4U
#define MINACTORDIST INT32_C(65536)
#define TILESHIFT 16
#define UNSIGNEDSHIFT 8
#define MINSIGHT INT32_C(0x18000)
#define ICONARROWS 90
#define RUNSPEED 6000
#define SPDPATROL 512
#define SPDDOG 1500
#define ACTORSIZE INT32_C(0x4000)
typedef enum { east,northeast,north,northwest,west,southwest,south,southeast,nodir } dirtype;
typedef enum { nothing,playerobj,inertobj,guardobj,officerobj,ssobj,dogobj,bossobj,schabbobj,fakeobj,mechahitlerobj,mutantobj,needleobj,fireobj,bjobj,ghostobj,realhitlerobj,gretelobj,giftobj,fatobj,rocketobj,spectreobj,angelobj,transobj,uberobj,willobj,deathobj,hrocketobj,sparkobj } classtype;
typedef enum { dressing,block,bo_gibs,bo_alpo,bo_firstaid,bo_key1,bo_key2,bo_key3,bo_key4,bo_cross,bo_chalice,bo_bible,bo_crown,bo_clip,bo_clip2,bo_machinegun,bo_chaingun,bo_food,bo_fullheal,bo_25clip,bo_spear } stat_t;
typedef enum { wp_knife,wp_pistol,wp_machinegun,wp_chaingun } weapontype;
typedef struct statestruct {
    int rotate,shapenum,tictime;
    void (*think)(objtype *),(*action)(objtype *);
    struct statestruct *next;
} statetype;
typedef struct { int tilex,tiley,shapenum;stat_t itemnumber;bool bonus; } statobj_t;
typedef struct {
    int health,ammo,lives,keys,score,nextextra,weapon,bestweapon,chosenweapon;
    int attackframe,weaponframe,attackcount,faceframe,killcount,killtotal,treasurecount,treasuretotal;
    int episode,mapon,difficulty,oldscore,secretcount,secrettotal;
    int32_t killx,killy;
    uint32_t TimeCount;
    bool died,victoryflag;
} gametype;
extern gametype gamestate;
extern objtype objlist[MAXACTORS],*actorat[64][64];
extern unsigned objcount,statcount;
extern objtype *killerobj;
bool GameFinished(void);
extern statobj_t statobjlist[MAXSTATS];
extern bool staticblock[64][64],spotvis[64][64],areabyplayer[37],madenoise;
extern unsigned wallheight[VIEWWIDTH];
extern int areaMap[64][64],thrustspeed;
void InitObjects(void);
objtype *GetNewActor(void);
objtype *SpawnActor(int x,int y,statetype *state);
void RemoveActor(objtype *ob);
bool ActorAllocationFailed(void);
bool ValidateActorSprites(void);
bool ScanInfoPlane(void);
void ConnectAreas(void);
void DoActor(objtype *ob);
void NewState(objtype *ob,statetype *state);
bool TryWalk(objtype *ob);
void MoveObj(objtype *ob,int32_t move);
void SelectDodgeDir(objtype *ob);
void SelectChaseDir(objtype *ob);
void SelectPathDir(objtype *ob);
bool CheckLine(objtype *ob);
bool CheckSight(objtype *ob);
bool SightPlayer(objtype *ob);
void FirstSighting(objtype *ob);
void DamageActor(objtype *ob,int damage);
void KillActor(objtype *ob);
void T_Stand(objtype *ob);
void T_Path(objtype *ob);
void T_Chase(objtype *ob);
void T_DogChase(objtype *ob);
void T_Ghosts(objtype *ob);
void T_Shoot(objtype *ob);
void T_Bite(objtype *ob);
void A_DeathScream(objtype *ob);
/* Optional synchronous sound sink. NULL actor means a non-positional sound.
   The sink must not mutate simulation or retain the actor pointer. */
void SetGameSoundHook(void (*hook)(soundnames,const objtype *));
void UpdateSoundLoc(void);
void GameSoundPosition(int32_t gx,int32_t gy,unsigned *left,unsigned *right);
void GameSound(soundnames sound,const objtype *actor);
void PlaceItemType(stat_t item,int x,int y);
void GetBonus(statobj_t *obj);
void GivePoints(int points);
void GiveAmmo(int ammo);
void GiveWeapon(int weapon);
void HealSelf(int points);
void TakeDamage(int points,objtype *attacker);
void CheckWeaponChange(void);
void T_Attack(bool attack);
void UpdateFace(void);
bool DrawStatus(void);
void ResetStatus(void);
void InvalidateStatus(void);
void ClearPaletteShifts(void);
void StartBonusFlash(void);
void StartDamageFlash(int damage);
void FinishPaletteShifts(void);
bool UpdatePaletteShifts(void);
void TransformActor(objtype *ob);
bool TransformTile(int tx,int ty,int *x,int *height);
int CalcRotate(const objtype *ob);
bool DrawScaleds(void);
bool ScaleShape(int xcenter,unsigned shapenum,unsigned height);
bool SimpleScaleShape(int xcenter,unsigned shapenum,unsigned height);
#include "WL_ACT2.h"
#endif
