/* SPDX-License-Identifier: GPL-2.0-only */
#include "engine/game_definition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

int main(void)
{
    /* 1. Default episode names and counts. */
    const game_definition_t *def = GameDefinition_Default();
    CHECK(def != NULL);
    CHECK(GameDefinition_Valid(def));
    CHECK(GameDefinition_EpisodeCount(def) == 6);
    CHECK(strcmp(GameDefinition_EpisodeName(def, 0), "Escape from Wolfenstein") == 0);
    CHECK(strcmp(GameDefinition_EpisodeName(def, 5), "Confrontation") == 0);

    /* 2. Default difficulty names and counts. */
    CHECK(GameDefinition_DifficultyCount(def) == 4);
    CHECK(strcmp(GameDefinition_DifficultyName(def, 0), "Can I play, Daddy?") == 0);
    CHECK(strcmp(GameDefinition_DifficultyName(def, 3), "I am Death incarnate!") == 0);

    /* 3. Default jukebox track names and counts. */
    CHECK(GameDefinition_JukeboxCount(def) == 18);
    CHECK(strcmp(GameDefinition_JukeboxName(def, 0), "Get Them!") == 0);
    CHECK(strcmp(GameDefinition_JukeboxName(def, 17), "Wolfpack") == 0);

    /* 4. Correct indexed access. */
    CHECK(strcmp(GameDefinition_EpisodeName(def, 1), "Operation: Eisenfaust") == 0);
    CHECK(strcmp(GameDefinition_DifficultyName(def, 1), "Don't hurt me.") == 0);
    CHECK(strcmp(GameDefinition_JukeboxName(def, 6), "Nazi Anthem") == 0);

    /* 5. Out-of-range access. */
    CHECK(GameDefinition_EpisodeName(def, 6) == NULL);
    CHECK(GameDefinition_DifficultyName(def, 4) == NULL);
    CHECK(GameDefinition_JukeboxName(def, 18) == NULL);

    /* 6. Alternative game definitions. */
    static const char *alt_eps[] = { "Alt Episode" };
    static const char *alt_diff[] = { "Easy" };
    static const char *alt_juke[] = { "Track 1" };
    game_definition_t alt = {
        .episode_names = alt_eps, .episode_count = 1,
        .difficulty_names = alt_diff, .difficulty_count = 1,
        .jukebox_names = alt_juke, .jukebox_count = 1,
    };
    CHECK(GameDefinition_Valid(&alt));
    CHECK(strcmp(GameDefinition_EpisodeName(&alt, 0), "Alt Episode") == 0);
    CHECK(GameDefinition_EpisodeCount(&alt) == 1);

    /* 7. Invalid and incomplete definitions. */
    CHECK(!GameDefinition_Valid(NULL));
    game_definition_t bad = { .episode_names = NULL, .episode_count = 1, .difficulty_names = NULL, .difficulty_count = 0, .jukebox_names = NULL, .jukebox_count = 0 };
    CHECK(!GameDefinition_Valid(&bad));
    static const char *empty_eps[] = { "" };
    game_definition_t empty = { .episode_names = empty_eps, .episode_count = 1, .difficulty_names = NULL, .difficulty_count = 0, .jukebox_names = NULL, .jukebox_count = 0 };
    CHECK(!GameDefinition_Valid(&empty));

    /* 8. Ownership and lifetime. */
    /* Default definition pointer is stable. */
    CHECK(GameDefinition_Default() == GameDefinition_Default());
    /* Borrowed pointers, not copies. */
    CHECK(GameDefinition_EpisodeName(def, 0) == def->episode_names[0]);

    /* 9. Legacy default compatibility. */
    CHECK(GameDefinition_EpisodeCount(def) == 6);
    CHECK(GameDefinition_DifficultyCount(def) == 4);
    CHECK(GameDefinition_JukeboxCount(def) == 18);

    printf("game_definition_tests: PASS\n");
    return 0;
}