/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef WL_DEMO_H
#define WL_DEMO_H
#include "w3r_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned buttons;int x,y;bool use; } demo_command;
bool Demo_Validate(const w3r_byte *data,size_t size,unsigned *map,size_t *length);
bool Demo_Start(unsigned number,uint64_t ticks);
void Demo_Stop(void);
bool Demo_Active(void);
bool Demo_Finished(void);
/* Read one original four-tic command frame. */
bool Demo_ReadTic(demo_command *command);
unsigned Demo_Buttons(void);
#endif
