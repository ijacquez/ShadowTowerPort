#include "recomp.h"
#include "ST_PlayerState.h"

void ST_ClearPlayerMovementLimits(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 0;
    MEM_W(ST_PLAYER_TURN_LIMIT, player) = 0;
    MEM_H(ST_PLAYER_TURN_RATE, player) = 0;
    MEM_H(ST_PLAYER_PITCH_RATE, player) = 0;
    MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = 0;
    MEM_B(ST_PLAYER_DIRECTIONAL_MOVE_DISABLED, player) = 1;
    ctx->r2 = player;
    ctx->r3 = 1;
}
