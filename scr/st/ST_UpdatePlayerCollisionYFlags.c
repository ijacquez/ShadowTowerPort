#include "recomp.h"
#include "ST_PlayerState.h"

// Update two flags from signed comparisons against collision-query Y values.
void ST_UpdatePlayerCollisionYFlags(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    const uint32_t query = 0x801B5948;
    const uint32_t missing = 0x80000;
    const int32_t playerY = S32(MEM_W(0x24C, player));
    const uint32_t firstY = MEM_W(0x4C, query);
    const uint32_t secondY = MEM_W(0x58, query);

    if (firstY != missing)
        MEM_W(0x2EC, player) = S32(firstY) < playerY;
    if (secondY != missing)
        MEM_W(0x2F0, player) = S32(secondY) < playerY;

    // Preserve the original routine's observable emulated-register results.
    ctx->r2 = 0x801A0000;
    ctx->r3 = MEM_BU(0x5924, 0x801B0000);
    ctx->r4 = secondY;
    if (ctx->r3 == 10) {
        ctx->r4 = player;
        ctx->r3 = MEM_HS(0x178, player);
        ctx->r2 = 0x147;
        if (ctx->r3 == 0x147)
            MEM_W(0x2F0, player) = 0;
    }
}
