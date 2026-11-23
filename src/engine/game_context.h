/* SPDX-License-Identifier: GPL-2.0-only */
/* Application/game context with explicit ownership.
   Holds the selected game identity. The engine uses this to obtain
   game-specific metadata without mutable global state. */
#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include "game_identity.h"

typedef struct {
    const game_identity_t *identity;
} game_context_t;

/* Create a context from the default identity. Returns 0 on failure. */
int GameContext_InitDefault(game_context_t *ctx);

/* Create a context from a caller-supplied identity. Validates the identity
   and filesystem safety. Returns 0 on failure, 1 on success. The caller owns
   the identity memory; the context stores a const pointer. */
int GameContext_Init(game_context_t *ctx, const game_identity_t *identity);

/* Returns the identity held by the context, or NULL if invalid.
   The returned pointer is borrowed from the caller-supplied identity and
   remains valid for as long as the caller's identity memory is valid.
   The context does not copy or own the identity strings. */
const game_identity_t *GameContext_Identity(const game_context_t *ctx);

/* Validates that the context holds a valid identity. */
int GameContext_Valid(const game_context_t *ctx);

#endif