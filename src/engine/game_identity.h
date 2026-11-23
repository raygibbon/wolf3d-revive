/* SPDX-License-Identifier: GPL-2.0-only */
/* Engine-facing game identity interface.
   The engine uses this to obtain game-specific metadata without
   hardcoding those values into platform or application code. */
#ifndef GAME_IDENTITY_H
#define GAME_IDENTITY_H

typedef struct {
    const char *game_id;       /* unique game identifier, e.g. "wolf3d-revive" */
    const char *display_title; /* window title, e.g. "Wolf3D Revive" */
    const char *prefs_id;      /* preferences directory name, e.g. "wolf3d-revive" */
} game_identity_t;

/* Returns the default game identity. The returned pointer is valid for the
   lifetime of the process and must not be freed. */
const game_identity_t *GameIdentity_Default(void);

#endif