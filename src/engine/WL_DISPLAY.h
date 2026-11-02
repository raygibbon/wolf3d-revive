/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_DISPLAY_H
#define WL_DISPLAY_H
#include "platform/display_settings.h"
/* The native application connects its two settings operations. Headless
   engine tests retain no dependency on SDL or a video device. */
void Display_Connect(bool (*get)(display_settings *),bool (*set)(const display_settings *));
bool Display_Available(void);
bool BeginDisplayMenu(void);
int HandleDisplayMenu(void);
#endif
