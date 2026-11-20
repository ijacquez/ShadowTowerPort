#include "recomp.h"
#include "ST_PlayerState.h"
#include "ST_CollisionQuery.h"

void ST_CheckHighDownwardMovement(uint8_t* rdram, recomp_context* ctx) {
    ctx->r29 = ADD32(ctx->r29, -0x18);
    MEM_W(0x10, ctx->r29) = ctx->r16;
    ctx->r16 = 0x80198F28;
    MEM_W(0x14, ctx->r29) = ctx->r31;

    const int32_t vertical = MEM_HS(ST_PLAYER_VERTICAL_MOVE, ctx->r16);
    ctx->r2 = 0x801B0000;
    if (vertical >= ST_PLAYER_HIGH_DOWNWARD_SPEED) {
        ctx->r2 = MEM_W(0x5948 + ST_COLLISION_QUERY_CURRENT_Y_RESULT, ctx->r2);
        ctx->r2 = SIGNED(ctx->r2) < ST_PLAYER_COLLISION_Y_OFFSET ? 1 : 0;
        if (ctx->r2 == 0) {
            ctx->r4 = 0;
            sub_8003BD14(rdram, ctx);
            ctx->r2 = 1;
            MEM_B(0x238, ctx->r16) = ctx->r2;
        }
    }

    ctx->r31 = MEM_W(0x14, ctx->r29);
    ctx->r16 = MEM_W(0x10, ctx->r29);
    ctx->r29 = ADD32(ctx->r29, 0x18);
}
