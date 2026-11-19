#include "recomp.h"
#include "ST_PlayerState.h"

// Add the supplied X, Y, and Z offsets directly to the player position.
void ST_TranslatePlayerPosition(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    ctx->r3 = player;
    ctx->r2 = ADD32(MEM_W(ST_PLAYER_POSITION_X, player), ctx->r4);
    MEM_W(ST_PLAYER_POSITION_X, player) = ctx->r2;
    ctx->r2 = MEM_W(ST_PLAYER_POSITION_Z, player);
    ctx->r4 = MEM_W(ST_PLAYER_POSITION_Y, player);
    ctx->r2 = ADD32(ctx->r2, ctx->r6);
    ctx->r4 = ADD32(ctx->r4, ctx->r5);
    MEM_W(ST_PLAYER_POSITION_Z, player) = ctx->r2;
    MEM_W(ST_PLAYER_POSITION_Y, player) = ctx->r4;
}
