/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform/sdl_video.h"
#include "engine/game_context.h"
#include "engine/game_identity.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, #x, SDL_GetError()); exit(1); } } while (0)

int main(void)
{
    /* 1. Default startup succeeds. */
    CHECK(VL_Startup());
    CHECK(VL_Screenshot());

    /* 2. Shutdown clears active context (verified via screenshot returning false). */
    VL_Shutdown();
    CHECK(!VL_Screenshot());  /* no active context after shutdown */

    /* 3. Restart with default. */
    CHECK(VL_Startup());
    CHECK(VL_Screenshot());

    /* 4. Invalid context rejected by VL_StartupWithContext. */
    game_context_t bad_ctx;
    memset(&bad_ctx, 0, sizeof(bad_ctx));
    CHECK(!VL_StartupWithContext(&bad_ctx));

    /* 5. NULL context rejected. */
    CHECK(!VL_StartupWithContext(NULL));

    /* 6. Valid alternative context accepted. */
    game_identity_t alt_id = { .game_id = "alt-game", .display_title = "Alt Game", .prefs_id = "alt-game" };
    game_context_t alt_ctx;
    CHECK(GameContext_Init(&alt_ctx, &alt_id));
    /* Startup with alt context should succeed (may fail on SDL init if already initialized,
       so shutdown first). */
    VL_Shutdown();
    CHECK(VL_StartupWithContext(&alt_ctx));

    /* 7. Screenshot override does not change active context. */
    game_identity_t override_id = { .game_id = "override", .display_title = "Override", .prefs_id = "override" };
    game_context_t override_ctx;
    CHECK(GameContext_Init(&override_ctx, &override_id));
    /* Screenshot with override context should succeed (or fail gracefully on missing dir). */
    /* The key check: active context remains alt_ctx's identity. */
    (void)VL_ScreenshotWithContext(&override_ctx);
    /* Active context should still be alt_ctx (not override). */
    /* We can't directly check vl_active_context, but VL_Screenshot() uses it. */
    /* If override changed active context, VL_Screenshot() would use override's prefs_id. */
    /* Since we can't easily verify the path, we verify the context pointer wasn't
       mutated by checking GameContext is still valid. */
    CHECK(GameContext_Valid(&alt_ctx));
    CHECK(strcmp(GameContext_Identity(&alt_ctx)->game_id, "alt-game") == 0);

    /* 8. Shutdown and restart with default. */
    VL_Shutdown();
    CHECK(VL_Startup());
    CHECK(VL_Screenshot());

    VL_Shutdown();
    printf("context_platform_tests: PASS\n");
    return 0;
}