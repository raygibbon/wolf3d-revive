/* SPDX-License-Identifier: GPL-2.0-only */
#include "game_definition.h"
#include <string.h>

static const char *const default_episodes[] = {
    "Escape from Wolfenstein",
    "Operation: Eisenfaust",
    "Die, Fuhrer, Die!",
    "A Dark Secret",
    "Trail of the Madman",
    "Confrontation",
};

static const char *const default_difficulties[] = {
    "Can I play, Daddy?",
    "Don't hurt me.",
    "Bring 'em on!",
    "I am Death incarnate!",
};

static const char *const default_jukebox[] = {
    "Get Them!", "Searching", "P.O.W.", "Suspense", "War March", "Around The Corner!",
    "Nazi Anthem", "Lurking...", "Going After Hitler", "Pounding Headache", "Into the Dungeons", "Ultimate Conquest",
    "Kill the S.O.B.", "The Nazi Rap", "Twelfth Hour", "Zero Hour", "Ultimate Conquest", "Wolfpack",
};

static const game_definition_t default_definition = {
    .episode_names = default_episodes,
    .episode_count = 6,
    .difficulty_names = default_difficulties,
    .difficulty_count = 4,
    .jukebox_names = default_jukebox,
    .jukebox_count = 18,
};

const game_definition_t *GameDefinition_Default(void)
{
    return &default_definition;
}

int GameDefinition_Valid(const game_definition_t *def)
{
    if (!def) return 0;
    if (def->episode_count > 0 && !def->episode_names) return 0;
    if (def->difficulty_count > 0 && !def->difficulty_names) return 0;
    if (def->jukebox_count > 0 && !def->jukebox_names) return 0;

    for (unsigned i = 0; i < def->episode_count; i++) {
        if (!def->episode_names[i] || def->episode_names[i][0] == '\0') return 0;
    }
    for (unsigned i = 0; i < def->difficulty_count; i++) {
        if (!def->difficulty_names[i] || def->difficulty_names[i][0] == '\0') return 0;
    }
    for (unsigned i = 0; i < def->jukebox_count; i++) {
        if (!def->jukebox_names[i] || def->jukebox_names[i][0] == '\0') return 0;
    }
    return 1;
}

const char *GameDefinition_EpisodeName(const game_definition_t *def, unsigned index)
{
    if (!def || !GameDefinition_Valid(def)) return NULL;
    if (index >= def->episode_count) return NULL;
    return def->episode_names[index];
}

const char *GameDefinition_DifficultyName(const game_definition_t *def, unsigned index)
{
    if (!def || !GameDefinition_Valid(def)) return NULL;
    if (index >= def->difficulty_count) return NULL;
    return def->difficulty_names[index];
}

const char *GameDefinition_JukeboxName(const game_definition_t *def, unsigned index)
{
    if (!def || !GameDefinition_Valid(def)) return NULL;
    if (index >= def->jukebox_count) return NULL;
    return def->jukebox_names[index];
}

unsigned GameDefinition_EpisodeCount(const game_definition_t *def)
{
    if (!def || !GameDefinition_Valid(def)) return 0;
    return def->episode_count;
}

unsigned GameDefinition_DifficultyCount(const game_definition_t *def)
{
    if (!def || !GameDefinition_Valid(def)) return 0;
    return def->difficulty_count;
}

unsigned GameDefinition_JukeboxCount(const game_definition_t *def)
{
    if (!def || !GameDefinition_Valid(def)) return 0;
    return def->jukebox_count;
}