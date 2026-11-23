/* SPDX-License-Identifier: GPL-2.0-only */
#include "game_identity.h"

static const game_identity_t default_identity = {
    .game_id = "wolf3d-revive",
    .display_title = "Wolf3D Revive",
    .prefs_id = "wolf3d-revive",
};

const game_identity_t *GameIdentity_Default(void)
{
    return &default_identity;
}