/* SPDX-License-Identifier: GPL-2.0-only */
/* Application/game context with explicit ownership.
   Holds the selected game identity and game definition. The engine uses
   this to obtain game-specific metadata without mutable global state. */
#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "game_identity.h"
#include "game_definition.h"

typedef struct {
    const game_identity_t *identity;
    const game_definition_t *definition;
} game_context_t;

/* Create a context from the default identity and default definition.
   Returns 0 on failure. */
int GameContext_InitDefault(game_context_t *ctx);

/* Create a context from a caller-supplied identity and definition.
   Validates both. Returns 0 on failure, 1 on success. The caller owns
   the identity and definition memory; the context stores const pointers. */
int GameContext_Init(game_context_t *ctx, const game_identity_t *identity,
                     const game_definition_t *definition);

/* Returns the identity held by the context, or NULL if invalid. */
const game_identity_t *GameContext_Identity(const game_context_t *ctx);

/* Returns the definition held by the context, or NULL if invalid. */
const game_definition_t *GameContext_Definition(const game_context_t *ctx);

/* Validates that the context holds a valid identity and definition. */
int GameContext_Valid(const game_context_t *ctx);

#endif