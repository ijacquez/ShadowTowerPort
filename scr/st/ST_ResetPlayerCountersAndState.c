#include "recomp.h"

void ST_ResetPlayerCountersAndState(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    ctx->r2 = player;
    ctx->r3 = MEM_HU(0, player);
    ctx->r29 = ADD32(ctx->r29, -0x18);
    MEM_W(0x10, ctx->r29) = ctx->r31;
    ctx->r4 = MEM_HU(4, player);

    for (int offset = 0x1C2; offset >= 0x190; offset -= 2)
        MEM_H(offset, player) = 0;
    MEM_B(0x238, player) = 0;
    MEM_H(0x18A, player) = ctx->r3;
    MEM_H(0x02, player) = ctx->r3;
    MEM_H(0x18C, player) = ctx->r4;
    MEM_H(0x06, player) = ctx->r4;
    MEM_W(0x2C, player) = 0;
    ST_ResetPlayerViewAndVerticalMovement(rdram, ctx);

    ctx->r31 = MEM_W(0x10, ctx->r29);
    ctx->r29 = ADD32(ctx->r29, 0x18);
}
