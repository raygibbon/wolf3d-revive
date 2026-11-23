/* SPDX-License-Identifier: GPL-2.0-only */
/* Game-definition interface.
   Separates Wolfenstein-specific display names from the reusable engine.
   The engine consumes definitions through this SDL-free interface. */
#ifndef GAME_DEFINITION_H
#define GAME_DEFINITION_H

typedef struct {
    const char *const *episode_names;
    unsigned episode_count;
    const char *const *difficulty_names;
    unsigned difficulty_count;
    const char *const *jukebox_names;
    unsigned jukebox_count;
} game_definition_t;

/* Returns the default (original Wolf3D) game definition. Process lifetime;
   must not be freed. */
const game_definition_t *GameDefinition_Default(void);

/* Validates that all required arrays are non-null with matching counts,
   and that no individual name is NULL or empty. */
int GameDefinition_Valid(const game_definition_t *def);

/* Bounded accessors. Return NULL for out-of-range or invalid input. */
const char *GameDefinition_EpisodeName(const game_definition_t *def, unsigned index);
const char *GameDefinition_DifficultyName(const game_definition_t *def, unsigned index);
const char *GameDefinition_JukeboxName(const game_definition_t *def, unsigned index);

unsigned GameDefinition_EpisodeCount(const game_definition_t *def);
unsigned GameDefinition_DifficultyCount(const game_definition_t *def);
unsigned GameDefinition_JukeboxCount(const game_definition_t *def);

#endif