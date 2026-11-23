/* SPDX-License-Identifier: GPL-2.0-only */
#include "game_context.h"
#include "game_identity.h"
#include "game_definition.h"
#include <string.h>

int GameContext_InitDefault(game_context_t *ctx)
{
    if (!ctx) return 0;
    ctx->identity = GameIdentity_Default();
    ctx->definition = GameDefinition_Default();
    return GameContext_Valid(ctx);
}

int GameContext_Init(game_context_t *ctx, const game_identity_t *identity,
                     const game_definition_t *definition)
{
    if (!ctx || !identity || !definition) return 0;
    if (!GameIdentity_Valid(identity)) return 0;
    if (!GameDefinition_Valid(definition)) return 0;
    /* Filesystem safety: prefs_id must not contain path separators or
       traversal sequences. */
    if (strchr(identity->prefs_id, '/') || strchr(identity->prefs_id, '\\'))
        return 0;
    if (strstr(identity->prefs_id, "..")) return 0;
    if (strchr(identity->game_id, '/') || strchr(identity->game_id, '\\'))
        return 0;
    if (strstr(identity->game_id, "..")) return 0;
    ctx->identity = identity;
    ctx->definition = definition;
    return 1;
}

const game_identity_t *GameContext_Identity(const game_context_t *ctx)
{
    if (!ctx || !GameContext_Valid(ctx)) return NULL;
    return ctx->identity;
}

const game_definition_t *GameContext_Definition(const game_context_t *ctx)
{
    if (!ctx || !GameContext_Valid(ctx)) return NULL;
    return ctx->definition;
}

int GameContext_Valid(const game_context_t *ctx)
{
    if (!ctx) return 0;
    if (!GameIdentity_Valid(ctx->identity)) return 0;
    if (!GameDefinition_Valid(ctx->definition)) return 0;
    return 1;
}