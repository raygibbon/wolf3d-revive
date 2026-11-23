/* SPDX-License-Identifier: GPL-2.0-only */
#include "game_context.h"
#include "game_identity.h"
#include <string.h>

int GameContext_InitDefault(game_context_t *ctx)
{
    if (!ctx) return 0;
    ctx->identity = GameIdentity_Default();
    return GameContext_Valid(ctx);
}

int GameContext_Init(game_context_t *ctx, const game_identity_t *identity)
{
    if (!ctx || !identity) return 0;
    if (!GameIdentity_Valid(identity)) return 0;
    /* Filesystem safety: prefs_id must not contain path separators or
       traversal sequences. */
    if (strchr(identity->prefs_id, '/') || strchr(identity->prefs_id, '\\'))
        return 0;
    if (strstr(identity->prefs_id, "..")) return 0;
    if (strchr(identity->game_id, '/') || strchr(identity->game_id, '\\'))
        return 0;
    if (strstr(identity->game_id, "..")) return 0;
    ctx->identity = identity;
    return 1;
}

const game_identity_t *GameContext_Identity(const game_context_t *ctx)
{
    if (!ctx || !GameContext_Valid(ctx)) return NULL;
    return ctx->identity;
}

int GameContext_Valid(const game_context_t *ctx)
{
    if (!ctx) return 0;
    return GameIdentity_Valid(ctx->identity);
}