#include "recomp.h"
#include "ST_PlayerState.h"

// Build the position and rotation passed to scene rendering.
void ST_BuildPlayerView(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    const uint32_t position = ctx->r4;
    const uint32_t rotation = ctx->r5;

    MEM_W(0, position) = MEM_W(ST_PLAYER_POSITION_X, player);
    MEM_W(8, position) = MEM_W(ST_PLAYER_POSITION_Z, player);
    ctx->r7 = MEM_HS(ST_PLAYER_VIEW_Y_ADJUSTMENT_A, player);
    ctx->r2 = MEM_W(ST_PLAYER_POSITION_Y, player);
    ctx->r6 = MEM_HS(ST_PLAYER_VIEW_Y_ADJUSTMENT_B, player);
    ctx->r2 = ADD32(ctx->r2, ctx->r7);
    ctx->r2 = ADD32(ctx->r2, ctx->r6);
    ctx->r2 = ADD32(ctx->r2, -ST_PLAYER_VIEW_Y_OFFSET);
    MEM_W(4, position) = ctx->r2;

    MEM_H(0, rotation) = MEM_HU(ST_PLAYER_VIEW_ROTATION_X, player);
    MEM_H(2, rotation) = MEM_HU(ST_PLAYER_VIEW_ROTATION_Y, player);
    ctx->r2 = MEM_HU(ST_PLAYER_VIEW_ROTATION_Z, player);
    MEM_H(4, rotation) = ctx->r2;
    ctx->r3 = player;
}
