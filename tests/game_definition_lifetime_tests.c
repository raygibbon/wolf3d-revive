/* SPDX-License-Identifier: GPL-2.0-only */
/* Ownership/lifetime and legacy jukebox-adapter checks for the selected
   game definition. WL_MAIN.c is linked with section garbage collection so
   only the small definition-selection API is retained. */
#include "engine/WL_MAIN.h"
#include "engine/game_definition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static const char *alt_eps[] = { "One", "Two" };
static const char *alt_diff[] = { "Soft", "Hard" };
static const char *alt_juke[] = { "T1", "T2", "T3", "T4" };

int main(void)
{
    /* Default selection before any caller supplies a definition. */
    CHECK(WL_GameDefinition() == GameDefinition_Default());

    /* Invalid definitions are rejected by resetting to the default rather
       than storing a pointer the engine cannot safely consume. */
    game_definition_t invalid = {
        .episode_names = NULL, .episode_count = 1,
        .difficulty_names = NULL, .difficulty_count = 0,
        .jukebox_names = NULL, .jukebox_count = 0,
    };
    WL_SetGameDefinition(&invalid);
    CHECK(WL_GameDefinition() == GameDefinition_Default());

    {
        game_definition_t alt = {
            .episode_names = alt_eps, .episode_count = 2,
            .difficulty_names = alt_diff, .difficulty_count = 2,
            .jukebox_names = alt_juke, .jukebox_count = 4,
        };
        CHECK(GameDefinition_Valid(&alt));
        WL_SetGameDefinition(&alt);
        CHECK(WL_GameDefinition() == &alt);

        /* Replacement while a definition is selected. */
        game_definition_t alt2 = {
            .episode_names = alt_eps, .episode_count = 2,
            .difficulty_names = alt_diff, .difficulty_count = 1,
            .jukebox_names = alt_juke, .jukebox_count = 4,
        };
        WL_SetGameDefinition(&alt2);
        CHECK(WL_GameDefinition() == &alt2);
    }

    /* Explicit shutdown/clear cannot leave the borrowed pointer selected. */
    WL_SetGameDefinition(NULL);
    CHECK(WL_GameDefinition() == GameDefinition_Default());

    /* Legacy jukebox adapter: fixed 3x6 slots, no invented resource indices. */
    const game_definition_t *def = GameDefinition_Default();
    for (unsigned bank = 0; bank < 3; ++bank) {
        for (unsigned slot = 0; slot < 6; ++slot) {
            const char *name = JukeboxTrackName(def, bank, slot);
            CHECK(name != NULL);
            CHECK(strcmp(name, GameDefinition_JukeboxName(def, bank * 6 + slot)) == 0);
        }
    }
    CHECK(JukeboxTrackName(def, 3, 0) == NULL);
    CHECK(JukeboxTrackName(def, 0, 6) == NULL);

    game_definition_t short_juke = {
        .episode_names = alt_eps, .episode_count = 2,
        .difficulty_names = alt_diff, .difficulty_count = 2,
        .jukebox_names = alt_juke, .jukebox_count = 4,
    };
    CHECK(JukeboxTrackName(&short_juke, 0, 3) != NULL);
    CHECK(JukeboxTrackName(&short_juke, 0, 4) == NULL);
    CHECK(JukeboxTrackName(&short_juke, 0, 5) == NULL);
    CHECK(JukeboxTrackName(&short_juke, 1, 0) == NULL);
    CHECK(JukeboxTrackName(NULL, 0, 0) != NULL);

    puts("game_definition_lifetime_tests: PASS");
    return 0;
}
