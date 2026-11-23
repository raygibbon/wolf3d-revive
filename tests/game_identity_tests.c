/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/game_identity.h"
#include "engine/game_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

int main(void)
{
    /* 1. Default identity. */
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
    CHECK(GameIdentity_Valid(id));

    /* 2. Alternative identity. */
    game_identity_t alt = {
        .game_id = "test-game",
        .display_title = "Test Game",
        .prefs_id = "test-game",
    };
    CHECK(GameIdentity_Valid(&alt));
    CHECK(strcmp(alt.game_id, "test-game") == 0);
    CHECK(strcmp(alt.display_title, "Test Game") == 0);
    CHECK(strcmp(alt.prefs_id, "test-game") == 0);

    /* 3. Missing or empty required fields. */
    game_identity_t empty_id = { .game_id = "", .display_title = "X", .prefs_id = "X" };
    CHECK(!GameIdentity_Valid(&empty_id));

    game_identity_t null_id = { .game_id = NULL, .display_title = "X", .prefs_id = "X" };
    CHECK(!GameIdentity_Valid(&null_id));

    game_identity_t empty_title = { .game_id = "X", .display_title = "", .prefs_id = "X" };
    CHECK(!GameIdentity_Valid(&empty_title));

    game_identity_t empty_prefs = { .game_id = "X", .display_title = "X", .prefs_id = "" };
    CHECK(!GameIdentity_Valid(&empty_prefs));

    /* Unsafe filesystem paths. */
    game_identity_t unsafe_slash = { .game_id = "X", .display_title = "X", .prefs_id = "a/b" };
    CHECK(!GameIdentity_Valid(&unsafe_slash));

    game_identity_t unsafe_backslash = { .game_id = "X", .display_title = "X", .prefs_id = "a\\b" };
    CHECK(!GameIdentity_Valid(&unsafe_backslash));

    game_identity_t unsafe_traversal = { .game_id = "X", .display_title = "X", .prefs_id = ".." };
    CHECK(!GameIdentity_Valid(&unsafe_traversal));

    game_identity_t unsafe_game_id = { .game_id = "a/b", .display_title = "X", .prefs_id = "X" };
    CHECK(!GameIdentity_Valid(&unsafe_game_id));

    CHECK(!GameIdentity_Valid(NULL));

    /* 4. Stable ownership and lifetime. */
    const game_identity_t *id2 = GameIdentity_Default();
    CHECK(id == id2);

    /* 5. No change to default values. */
    CHECK(strcmp(GameIdentity_Default()->game_id, "wolf3d-revive") == 0);
    CHECK(strcmp(GameIdentity_Default()->display_title, "Wolf3D Revive") == 0);
    CHECK(strcmp(GameIdentity_Default()->prefs_id, "wolf3d-revive") == 0);

    /* 6. Context tests. */
    game_context_t ctx;
    CHECK(GameContext_InitDefault(&ctx));
    CHECK(GameContext_Valid(&ctx));
    const game_identity_t *ctx_id = GameContext_Identity(&ctx);
    CHECK(ctx_id != NULL);
    CHECK(strcmp(ctx_id->game_id, "wolf3d-revive") == 0);

    /* Alternative identity via context. */
    game_identity_t alt2 = { .game_id = "alt-game", .display_title = "Alt Game", .prefs_id = "alt-game" };
    game_context_t alt_ctx;
    CHECK(GameContext_Init(&alt_ctx, &alt2));
    CHECK(GameContext_Valid(&alt_ctx));
    const game_identity_t *alt_ctx_id = GameContext_Identity(&alt_ctx);
    CHECK(alt_ctx_id != NULL);
    CHECK(strcmp(alt_ctx_id->game_id, "alt-game") == 0);

    /* Invalid identity rejected by context. */
    game_identity_t bad = { .game_id = "", .display_title = "X", .prefs_id = "X" };
    game_context_t bad_ctx;
    CHECK(!GameContext_Init(&bad_ctx, &bad));

    /* NULL context rejected. */
    CHECK(!GameContext_Init(NULL, &alt2));
    CHECK(!GameContext_Init(&alt_ctx, NULL));
    CHECK(!GameContext_Valid(NULL));
    CHECK(GameContext_Identity(NULL) == NULL);

    /* Default preferences paths unchanged. */
    CHECK(strcmp(GameContext_Identity(&ctx)->prefs_id, "wolf3d-revive") == 0);

    printf("game_identity_tests: PASS\n");
    return 0;
}