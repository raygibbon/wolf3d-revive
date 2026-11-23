/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/game_identity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

int main(void)
{
    const game_identity_t *id = GameIdentity_Default();
    CHECK(id != NULL);
    CHECK(id->game_id != NULL);
    CHECK(id->display_title != NULL);
    CHECK(id->prefs_id != NULL);
    CHECK(strlen(id->game_id) > 0);
    CHECK(strlen(id->display_title) > 0);
    CHECK(strlen(id->prefs_id) > 0);
    CHECK(strcmp(id->game_id, "wolf3d-revive") == 0);
    CHECK(strcmp(id->display_title, "Wolf3D Revive") == 0);
    CHECK(strcmp(id->prefs_id, "wolf3d-revive") == 0);

    /* Stable lifetime: repeated calls return the same pointer. */
    const game_identity_t *id2 = GameIdentity_Default();
    CHECK(id == id2);

    /* Non-null metadata strings. */
    CHECK(id->game_id[0] != '\0');
    CHECK(id->display_title[0] != '\0');
    CHECK(id->prefs_id[0] != '\0');

    printf("game_identity_tests: PASS\n");
    return 0;
}