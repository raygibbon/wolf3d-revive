/* SPDX-License-Identifier: GPL-2.0-only */
/* Versioned loose-level representation for the first playable Workshop slice.
   This is deliberately independent of ECWolf PK3 containers and of the
   original WL6 map loader; the live tilemap remains the runtime authority. */
#ifndef WL_LEVEL_H
#define WL_LEVEL_H
#include "ID_MAP.h"
#include <stdbool.h>
#include <stddef.h>

#define WL_LEVEL_SIZE 64U
#define WL_LEVEL_FILE "workshop.w3l"
#define WL_LEVEL_VERSION 1U

typedef struct {
    unsigned width,height;
    unsigned spawn_x,spawn_y,spawn_angle;
    w3r_byte tiles[WL_LEVEL_SIZE][WL_LEVEL_SIZE];
} wl_level;

bool WL_LevelMaterialValid(unsigned material);
bool WL_LevelBlank(wl_level *level,unsigned material);
bool WL_LevelFromWorld(wl_level *level,const w3r_byte tiles[WL_LEVEL_SIZE][WL_LEVEL_SIZE],
                       unsigned spawn_x,unsigned spawn_y,int spawn_angle);
bool WL_LevelValidate(const wl_level *level,char *error,size_t error_size);
bool WL_LevelToMap(const wl_level *level,maptype *map);
bool WL_LevelPath(const char *directory,char *path,size_t path_size);
bool WL_LevelExists(const char *directory);
bool WL_LevelSave(const wl_level *level,const char *directory,char *error,size_t error_size);
bool WL_LevelLoad(wl_level *level,const char *directory,char *error,size_t error_size);

#endif
