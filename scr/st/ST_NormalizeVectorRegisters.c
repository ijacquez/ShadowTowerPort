#include "recomp.h"
#include "ST_VectorNormalization.h"

// Normalize the signed low halfwords in emulated registers t0, t1 and t2.
void ST_NormalizeVectorRegisters(uint8_t* rdram, recomp_context* ctx) {
    gte_mtc2(ctx, 8, 9);
    gte_mtc2(ctx, 9, 10);
    gte_mtc2(ctx, 10, 11);
    gte_command(ctx, 0x4AA00428); // SQR

    ctx->r11 = gte_mfc2(ctx, 25);
    ctx->r12 = gte_mfc2(ctx, 26);
    ctx->r13 = gte_mfc2(ctx, 27);
    ctx->r11 = ADD32(ctx->r11, ctx->r12);
    ctx->r2 = ADD32(ctx->r11, ctx->r13);
    gte_mtc2(ctx, 2, 30);

    ctx->r1 = 0xFFFFFFFE;
    ctx->r3 = gte_mfc2(ctx, 31) & ctx->r1;
    ctx->r14 = S32(SUB32(31, ctx->r3)) >> 1;
    const int32_t tableShift = S32(SUB32(ctx->r3, 24));
    if (tableShift >= 0) {
        ctx->r11 = (uint32_t)tableShift;
        ctx->r12 = U32(ctx->r2) << (ctx->r11 & 31);
    } else {
        ctx->r11 = SUB32(24, ctx->r3);
        ctx->r12 = S32(ctx->r2) >> (ctx->r11 & 31);
    }
    ctx->r12 = U32(ADD32(ctx->r12, -64)) << 1;
    ctx->r13 = ADD32(0x80080000, ctx->r12);
    ctx->r13 = MEM_HS(ST_NORMALIZATION_INVERSE_ROOT_TABLE_ADDRESS & 0xFFFF, ctx->r13);

    gte_mtc2(ctx, 13, 8);
    gte_mtc2(ctx, 8, 9);
    gte_mtc2(ctx, 9, 10);
    gte_mtc2(ctx, 10, 11);
    gte_command(ctx, 0x4B90003D); // GPF

    ctx->r8 = S32(gte_mfc2(ctx, 25)) >> (ctx->r14 & 31);
    ctx->r9 = S32(gte_mfc2(ctx, 26)) >> (ctx->r14 & 31);
    ctx->r10 = S32(gte_mfc2(ctx, 27)) >> (ctx->r14 & 31);
}
