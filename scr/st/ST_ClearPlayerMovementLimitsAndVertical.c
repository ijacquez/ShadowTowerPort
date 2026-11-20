#include "recomp.h"
#include "ST_PlayerState.h"

void ST_ClearPlayerMovementLimitsAndVertical(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    ctx->r29 = ADD32(ctx->r29, -0x18);
    MEM_W(0x10, ctx->r29) = ctx->r31;
    MEM_W(0x23C, player) = 0;
    MEM_B(0x21C, player) = 1;
    MEM_B(0x239, player) = 0;
    MEM_H(ST_PLAYER_VERTICAL_MOVE, player) = 0;
    MEM_B(0x218, player) = 0;
    ST_ClearPlayerMovementLimits(rdram, ctx);
    ctx->r31 = MEM_W(0x10, ctx->r29);
    ctx->r29 = ADD32(ctx->r29, 0x18);
}
