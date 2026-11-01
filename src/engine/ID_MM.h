/* SPDX-License-Identifier: GPL-2.0-only */
/* Portable slice of WOLFSRC/ID_MM.H: owner handles replace DOS segments. */
#ifndef ID_MM_H
#define ID_MM_H
#include <stdbool.h>
#include <stddef.h>

typedef void *memptr;
#define MM_MAX_BYTES ((size_t)64 * 1024 * 1024)

void MM_Startup(void);
void MM_Shutdown(void);
/* Handles must remain at a stable address until freed/shutdown. */
bool MM_GetPtr(memptr *baseptr, size_t size);
bool MM_FreePtr(memptr *baseptr);
bool MM_SetPurge(memptr *baseptr, unsigned purge);
bool MM_SetLock(memptr *baseptr, bool locked);
void MM_SortMem(void);
size_t MM_TotalBytes(void);
#endif
