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

/* Validates that all required fields of the given identity are non-null and
   non-empty, and that game_id and prefs_id are filesystem-safe (no path
   separators or traversal sequences). Returns true if valid, false otherwise.
   Does not modify the input. The caller owns the identity memory and all
   strings; the engine does not copy or free them. Strings must remain valid
   for the lifetime of any context that references this identity. */
int GameIdentity_Valid(const game_identity_t *identity);

#endif