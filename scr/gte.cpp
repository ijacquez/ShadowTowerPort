#include "gte.h"
#include "psx/gtereg.h"
#include "gte/PsyX_GTE.h"
#include <cstring>
    





void ctx_to_gte(recomp_context* ctx)
{
    memcpy(&gteRegs.CP2D, ctx->cp2d, 32 * 4);
    memcpy(&gteRegs.CP2C, ctx->cp2c, 32 * 4);
}
void gte_to_ctx(recomp_context* ctx)
{
    memcpy(ctx->cp2d, &gteRegs.CP2D, 32 * 4);
    memcpy(ctx->cp2c, &gteRegs.CP2C, 32 * 4);
}










void gte_command(recomp_context* ctx, uint32_t cmd) {
    ctx_to_gte(ctx);
    GTE_operator(cmd);
    gte_to_ctx(ctx);
}

void gte_mtc2(recomp_context* ctx, int rt, int rd) {
    const uint32_t value = (&ctx->r0)[rt];
    if (rd == 31) return;
    if (rd == 30) ctx->cp2d[31] = gte_leadingzerocount(value);
    ctx->cp2d[rd] = value;
}

void gte_lwc2(uint8_t* rdram, recomp_context* ctx, int rt, int rs, int imm) {
    uint32_t addr = (uint32_t)((int32_t)(&ctx->r0)[rs] + imm);
    ctx->cp2d[rt] = MEM_W(0, addr);
}

void gte_swc2(uint8_t* rdram, recomp_context* ctx, int rt, int rs, int imm) {
    uint32_t addr = (uint32_t)((int32_t)(&ctx->r0)[rs] + imm);
    WRITE_W(addr, ctx->cp2d[rt]);
}

uint32_t gte_mfc2(recomp_context* ctx, int rd) {
    switch (rd) {
    case 1: case 3: case 5: case 8: case 9: case 10: case 11:
        ctx->cp2d[rd] = (uint32_t)(int32_t)(int16_t)ctx->cp2d[rd];
        break;
    case 7: case 16: case 17: case 18: case 19:
        ctx->cp2d[rd] = (uint16_t)ctx->cp2d[rd];
        break;
    }
    return ctx->cp2d[rd];
}


void gte_ctc2(recomp_context* ctx, int rt, int rd) {
    ctx->cp2c[rd] = (&ctx->r0)[rt]; // rt — индекс GPR
}

uint32_t gte_cfc2(recomp_context* ctx, int rd) {
    return ctx->cp2c[rd];
}

