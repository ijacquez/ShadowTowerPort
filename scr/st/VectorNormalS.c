#include "recomp.h"

// Normalize three input words into three output halfwords.
void VectorNormalS(uint8_t* rdram, recomp_context* ctx) {
    ctx->r8 = MEM_W(0, ctx->r4);
    ctx->r9 = MEM_W(4, ctx->r4);
    ctx->r10 = MEM_W(8, ctx->r4);
    ctx->r7 = ctx->r31;
    ST_NormalizeVectorRegisters(rdram, ctx);
    MEM_H(0, ctx->r5) = ctx->r8;
    MEM_H(2, ctx->r5) = ctx->r9;
    MEM_H(4, ctx->r5) = ctx->r10;
    ctx->r31 = ctx->r7;
}
