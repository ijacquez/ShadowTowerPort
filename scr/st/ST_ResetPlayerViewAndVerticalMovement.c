#include "recomp.h"
#include "ST_PlayerState.h"

void ST_ResetPlayerViewAndVerticalMovement(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    ctx->r2 = player;
    const int clearedHalfwords[] = {
        ST_PLAYER_VIEW_OSCILLATION_Y, ST_PLAYER_VIEW_OSCILLATION_PHASE,
        ST_PLAYER_VIEW_Y_ADJUSTMENT_B, ST_PLAYER_VERTICAL_MOVE,
        0x294, 0x292, 0x290, 0x28C, 0x28A, 0x288, 0x284, 0x282, 0x280
    };
    for (int offset : clearedHalfwords) MEM_H(offset, player) = 0;
    MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = 0;
    MEM_B(0x218, player) = 0;

    // Copy the two aligned words containing pitch, yaw, and the next angles.
    MEM_W(0x268, player) = MEM_W(0x278, player);
    ctx->r4 = MEM_W(0x27C, player);
    MEM_W(0x26C, player) = ctx->r4;
    MEM_B(0x155, player) = 0xFF;
    MEM_B(0x156, player) = 0xFF;
    MEM_W(0x2F0, player) = 0;
    MEM_W(0x2EC, player) = 0;
    MEM_W(0, 0x801BA124) = 0;
    ctx->r3 = 0xFFFFFFFF;
    MEM_W(0x2F4, player) = ctx->r3;
}
