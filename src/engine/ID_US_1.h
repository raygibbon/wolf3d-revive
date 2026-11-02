/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef ID_US_1_H
#define ID_US_1_H
#include <stdbool.h>
extern int PrintX, PrintY, WindowX;
bool US_Print(const char *text);
void US_InitRndT(unsigned seed);
int US_RndT(void);
unsigned US_RndIndex(void);
#endif
