/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_CONTROLS_H
#define WL_CONTROLS_H
#include <stdbool.h>
/* USB physical-key identifiers, matching SDL3 scancodes without an SDL
   dependency in the simulation. Zero is unbound. Menu keys stay in ID_IN. */
#define CT_KEYS 512U
typedef enum { CT_CLASSIC,CT_MODERN } ct_preset;
typedef enum { CT_FORWARD,CT_BACK,CT_LEFT,CT_RIGHT,CT_STRAFE_LEFT,CT_STRAFE_RIGHT,
    CT_RUN,CT_ALT,CT_FIRE,CT_USE,CT_WEAPON1,CT_WEAPON2,CT_WEAPON3,CT_WEAPON4,CT_MENU,CT_ACTIONS } ct_action;
typedef struct {
    unsigned binding[CT_ACTIONS][2];
    bool mouse,invert;
    unsigned sensitivity;
} ct_profile;
void CT_DefaultProfile(ct_preset preset,ct_profile *profile);
void CT_GetProfile(ct_preset preset,ct_profile *profile);
bool CT_SetProfile(ct_preset preset,const ct_profile *profile);
bool CT_ValidProfile(const ct_profile *profile);
void CT_SelectPreset(ct_preset preset); /* Retains each preset's custom profile. */
void CT_Reset(ct_preset preset); /* Explicit reset to defaults for this preset. */
void CT_SetGameplay(bool active);
bool CT_Gameplay(void);
ct_preset CT_Preset(void);
void CT_Key(unsigned key,bool down);
void CT_LegacyKey(unsigned scan,bool down);
void CT_MouseButton(unsigned button,bool down);
void CT_MouseMotion(double x);
void CT_Clear(void);
void CT_EndTic(void);
bool CT_Down(ct_action action);
bool CT_Pressed(ct_action action);
int CT_MouseTurn(void);
unsigned CT_Binding(ct_action action,unsigned alternative);
/* Reject duplicate assignments atomically. action/slot/zero are validated. */
bool CT_Bind(ct_action action,unsigned alternative,unsigned key);
const char *CT_ActionName(ct_action action);
bool CT_MouseEnabled(void);
void CT_SetMouse(bool enabled);
unsigned CT_Sensitivity(void);
void CT_SetSensitivity(unsigned sensitivity);
bool CT_Invert(void);
void CT_SetInvert(bool invert);
/* Binding capture consumes physical keys without changing menu navigation. */
unsigned CT_NextKey(void);
#endif
