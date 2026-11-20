#include "recomp.h"
#include "psx/libgte.h"
#include "ST_VectorNormalization.h"

// Preserve the game routine's register and GTE effects, including signed
// input indexing before the square-root table in emulated memory.
void ST_SquareRoot0(uint8_t* rdram, recomp_context* ctx) {
    gte_mtc2(ctx, 4, 30);
    ctx->r2 = gte_mfc2(ctx, 31);
    ctx->r1 = 32;
    if (ctx->r2 == 32) {
        ctx->r2 = 0;
        return;
    }

    ctx->r8 = ctx->r2 & 1;
    ctx->r1 = 0xFFFFFFFE;
    ctx->r10 = ctx->r2 & ctx->r1;
    ctx->r9 = S32(SUB32(31, ctx->r10)) >> 1;
    const int32_t tableShift = S32(SUB32(ctx->r10, 24));
    if (tableShift >= 0) {
        ctx->r12 = U32(ctx->r4) << (tableShift & 31);
        ctx->r11 = 24;
    } else {
        ctx->r11 = SUB32(24, ctx->r10);
        ctx->r12 = S32(ctx->r4) >> (ctx->r11 & 31);
    }
    ctx->r12 = U32(ADD32(ctx->r12, -64)) << 1;
    ctx->r13 = ADD32(0x80080000, ctx->r12);
    ctx->r13 = MEM_HS(ST_SQUARE_ROOT_TABLE_ADDRESS & 0xFFFF, ctx->r13);
    ctx->r13 = U32(ctx->r13) << (ctx->r9 & 31);
    ctx->r2 = U32(ctx->r13) >> 12;
}

void SquareRoot0(uint8_t* rdram, recomp_context* ctx) {
    // Negative wrapped sums address memory before the game's lookup table.
    if (S32(ctx->r4) < 0) {
        ST_SquareRoot0(rdram, ctx);
        return;
    }

    const int input = (int)ctx->r4;
    ctx_to_gte(ctx);
    const int result = SquareRoot0(input);
    gte_to_ctx(ctx);
    ctx->r2 = (uint32_t)result;
}
