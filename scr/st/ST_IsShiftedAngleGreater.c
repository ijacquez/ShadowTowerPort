#include "recomp.h"

// Compare 12-bit angles after rotating both by half a turn.
void ST_IsShiftedAngleGreater(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t first = (ctx->r4 + 0x800) & 0xFFF;
    ctx->r5 = (ctx->r5 + 0x800) & 0xFFF;
    ctx->r2 = first > ctx->r5;
}
