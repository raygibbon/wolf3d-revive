/* SPDX-License-Identifier: GPL-2.0-only */
#include "game_identity.h"
#include <string.h>

static const game_identity_t default_identity = {
    .game_id = "wolf3d-revive",
    .display_title = "Wolf3D Revive",
    .prefs_id = "wolf3d-revive",
};

const game_identity_t *GameIdentity_Default(void)
{
    return &default_identity;
}

int GameIdentity_Valid(const game_identity_t *identity)
{
    if (!identity) return 0;
    if (!identity->game_id || identity->game_id[0] == '\0') return 0;
    if (!identity->display_title || identity->display_title[0] == '\0') return 0;
    if (!identity->prefs_id || identity->prefs_id[0] == '\0') return 0;
    return 1;
}