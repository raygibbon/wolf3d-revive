/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "platform/sdl_video.h"
#include "engine/game_context.h"
#include "engine/game_definition.h"
#include "engine/game_identity.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, #x, SDL_GetError()); exit(1); } } while (0)

static int dir_has_bmp(const char *dirpath)
{
    DIR *d = opendir(dirpath);
    if (!d) return 0;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strstr(ent->d_name, ".bmp")) count++;
    }
    closedir(d);
    return count;
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    SDL_SetMainReady();

    /* Use argv[1] as prefs dir if provided, else create temp. */
    const char *prefs_base = (argc > 1) ? argv[1] : NULL;
    char tmpdir[4096];
    if (prefs_base) {
        snprintf(tmpdir, sizeof(tmpdir), "%s", prefs_base);
    } else {
        snprintf(tmpdir, sizeof(tmpdir), "/tmp/w3r-ctx-test-%d", (int)getpid());
        (void)mkdir(tmpdir, 0755);
    }
    setenv("XDG_DATA_HOME", tmpdir, 1);

    /* 1. Startup with alternative identity. */
    game_identity_t alt_id = { .game_id = "alt-game", .display_title = "Alt Game", .prefs_id = "alt-game" };
    game_context_t alt_ctx;
    CHECK(GameContext_Init(&alt_ctx, &alt_id, GameDefinition_Default()));
    CHECK(VL_StartupWithContext(&alt_ctx));

    /* 2. Capture screenshot with active context (alt-game directory). */
    CHECK(VL_Screenshot());

    /* 3. Capture screenshot with override identity (override-game directory). */
    game_identity_t override_id = { .game_id = "override-game", .display_title = "Override", .prefs_id = "override-game" };
    game_context_t override_ctx;
    CHECK(GameContext_Init(&override_ctx, &override_id, GameDefinition_Default()));
    CHECK(VL_ScreenshotWithContext(&override_ctx));

    /* 4. Capture again using VL_Screenshot() (active context, alt-game directory). */
    CHECK(VL_Screenshot());

    /* 5. Verify screenshots in expected directories. */
    char alt_dir[8192], override_dir[8192];
    snprintf(alt_dir, sizeof(alt_dir), "%s/alt-game", tmpdir);
    snprintf(override_dir, sizeof(override_dir), "%s/override-game", tmpdir);

    /* alt-game should have 2 screenshots (steps 2 and 4). */
    CHECK(dir_has_bmp(alt_dir) >= 2);
    /* override-game should have 1 screenshot (step 3). */
    CHECK(dir_has_bmp(override_dir) >= 1);

    /* 6. Verify active context unchanged (override did not alter it). */
    /* We verify by checking that a subsequent VL_Screenshot() goes to alt-game. */
    CHECK(VL_Screenshot());
    /* alt-game should now have 3. */
    CHECK(dir_has_bmp(alt_dir) >= 3);

    /* 7. Shutdown. */
    VL_Shutdown();

    /* 8. After shutdown, VL_Screenshot() should fail (no active context). */
    CHECK(!VL_Screenshot());

    /* 9. Restart with default. */
    CHECK(VL_Startup());
    CHECK(VL_Screenshot());

    /* 10. Invalid context rejection (startup failure). */
    game_context_t bad_ctx;
    memset(&bad_ctx, 0, sizeof(bad_ctx));
    /* Shutdown first so we can test startup failure. */
    VL_Shutdown();
    CHECK(!VL_StartupWithContext(&bad_ctx));
    /* After failed startup, VL_Screenshot should fail. */
    CHECK(!VL_Screenshot());

    /* 11. Restart successfully. */
    CHECK(VL_Startup());
    CHECK(VL_Screenshot());

    VL_Shutdown();

    printf("context_platform_tests: PASS\n");
    return 0;
}