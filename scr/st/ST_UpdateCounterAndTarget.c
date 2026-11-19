#include "recomp.h"

// Update a signed halfword counter and optional target, and return its scaled
// value. The fifth argument controls whether the calculated values are stored.
void ST_UpdateCounterAndTarget(uint8_t* rdram, recomp_context* ctx) {
    ctx->r8 = MEM_HU(0, ctx->r4);
    ctx->r11 = MEM_W(0x10, ctx->r29);
    ctx->r10 = ctx->r5 ? MEM_HU(0, ctx->r5) : 0;
    ctx->r9 = (uint32_t)(int32_t)(int16_t)ctx->r8;

    if (ctx->r9 == 0 && ctx->r10 == 0) {
        ctx->r2 = 0xFFFFFFFF;
        return;
    }

    if (S32(ctx->r9) < (int16_t)ctx->r10) {
        ctx->r8 = ADD32(ctx->r8, 1);
    } else {
        ctx->r10 = 0;
        ctx->r8 = ADD32(ctx->r8, -1);
    }
    ctx->r3 = (uint32_t)(int32_t)(int16_t)ctx->r8;

    if (S32(ctx->r3) < S32(ctx->r6)) {
        // MIPS multiplication retains its low word; division truncates
        // toward zero. Keep the original emulated break and register effects.
        ctx->r2 = (uint32_t)((int64_t)S32(ctx->r3) * (int64_t)S32(ctx->r7));
        const int64_t quotient = (int64_t)S32(ctx->r2) / (int64_t)S32(ctx->r6);
        ctx->r1 = 0x80000000;
        if (ctx->r6 == 0xFFFFFFFF && ctx->r2 == 0x80000000)
            do_break(2147729932);
        ctx->r2 = (uint32_t)quotient;
    } else {
        ctx->r2 = ctx->r7;
    }

    if (ctx->r11 != 0) {
        MEM_H(0, ctx->r4) = ctx->r8;
        if (ctx->r5) MEM_H(0, ctx->r5) = ctx->r10;
    }
    ctx->r2 = (uint32_t)(int32_t)(int16_t)ctx->r2;
}
