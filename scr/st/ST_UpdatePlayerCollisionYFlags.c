#include "recomp.h"
#include "ST_PlayerState.h"
#include "ST_CollisionQuery.h"
#include "ST_LevelStreaming.h"

// Update two flags from signed comparisons against collision-query Y values.
void ST_UpdatePlayerCollisionYFlags(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    const uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    const uint32_t missing = 0x80000;
    const int32_t playerY = S32(MEM_W(ST_PLAYER_POSITION_Y, player));
    const uint32_t firstY = MEM_W(ST_COLLISION_QUERY_FIRST_PLAYER_Y_COMPARISON, query);
    const uint32_t secondY = MEM_W(ST_COLLISION_QUERY_SECOND_PLAYER_Y_COMPARISON, query);

    if (firstY != missing)
        MEM_W(ST_PLAYER_FIRST_COLLISION_Y_FLAG, player) = S32(firstY) < playerY;
    if (secondY != missing)
        MEM_W(ST_PLAYER_SECOND_COLLISION_Y_FLAG, player) = S32(secondY) < playerY;

    // Preserve the original routine's observable emulated-register results.
    ctx->r2 = 0x801A0000;
    ctx->r3 = MEM_BU(ST_LEVEL_STREAMING_APPLIED_ID_FIRST, ST_LEVEL_STREAMING_STATUS_ADDRESS);
    ctx->r4 = secondY;
    if (ctx->r3 == 10) {
        ctx->r4 = player;
        ctx->r3 = MEM_HS(ST_PLAYER_STREAMING_RESET_HALFWORD, player);
        ctx->r2 = 0x147;
        if (ctx->r3 == 0x147)
            MEM_W(ST_PLAYER_SECOND_COLLISION_Y_FLAG, player) = 0;
    }
}
