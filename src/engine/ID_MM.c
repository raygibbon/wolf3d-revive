/* SPDX-License-Identifier: GPL-2.0-only */
/* Port of WOLFSRC/ID_MM.C ownership/purge semantics. No segment relocation. */
#include "ID_MM.h"
#include <stdlib.h>

typedef struct mmblocktype {
    memptr *useptr;
    void *allocation;
    size_t length;
    unsigned purge;
    bool locked;
    struct mmblocktype *next;
} mmblocktype;

static mmblocktype *mmhead;
static size_t used;

static mmblocktype *MM_Find(memptr *baseptr)
{
    for (mmblocktype *block = mmhead; block; block = block->next)
        if (block->useptr == baseptr) return block;
    return NULL;
}

void MM_Shutdown(void)
{
    while (mmhead) (void)MM_FreePtr(mmhead->useptr);
}

void MM_Startup(void)
{
    MM_Shutdown();
}

bool MM_GetPtr(memptr *baseptr, size_t size)
{
    mmblocktype *block;
    if (!baseptr || *baseptr || !size || size > MM_MAX_BYTES) return false;
    if (size > MM_MAX_BYTES - used) MM_SortMem();
    if (size > MM_MAX_BYTES - used) return false;
    block = calloc(1, sizeof(*block));
    if (!block) return false;
    block->allocation = calloc(1, size);
    if (!block->allocation) { free(block); return false; }
    block->useptr = baseptr;
    block->length = size;
    block->next = mmhead;
    mmhead = block;
    *baseptr = block->allocation;
    used += size;
    return true;
}

bool MM_FreePtr(memptr *baseptr)
{
    mmblocktype **link = &mmhead;
    while (*link) {
        mmblocktype *block = *link;
        if (block->useptr == baseptr) {
            *link = block->next;
            used -= block->length;
            free(block->allocation);
            *baseptr = NULL;
            free(block);
            return true;
        }
        link = &block->next;
    }
    return false;
}

bool MM_SetPurge(memptr *baseptr, unsigned purge)
{
    mmblocktype *block = MM_Find(baseptr);
    if (!block || purge > 3) return false;
    block->purge = purge;
    return true;
}

bool MM_SetLock(memptr *baseptr, bool locked)
{
    mmblocktype *block = MM_Find(baseptr);
    if (!block) return false;
    block->locked = locked;
    return true;
}

void MM_SortMem(void)
{
    mmblocktype *block = mmhead;
    while (block) {
        mmblocktype *next = block->next;
        if (!block->locked && block->purge) (void)MM_FreePtr(block->useptr);
        block = next;
    }
}

size_t MM_TotalBytes(void) { return used; }
