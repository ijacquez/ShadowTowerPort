#include "recomp.h"
#include "ST_PlayerState.h"
#include "ST_CollisionQuery.h"

void ST_ResetPlayerViewAndVerticalMovement(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t player = 0x80198F28;
    ctx->r2 = player;
    const int clearedHalfwords[] = {
        ST_PLAYER_VIEW_OSCILLATION_Y, ST_PLAYER_VIEW_OSCILLATION_PHASE,
        ST_PLAYER_VIEW_Y_ADJUSTMENT_B, ST_PLAYER_VERTICAL_MOVE,
        ST_PLAYER_VIEW_ADDITIONAL_ROTATION_Z, ST_PLAYER_VIEW_ADDITIONAL_ROTATION_Y,
        ST_PLAYER_VIEW_ADDITIONAL_ROTATION_X, ST_PLAYER_VIEW_TRANSIENT_ROTATION_Z,
        ST_PLAYER_VIEW_TRANSIENT_ROTATION_Y, ST_PLAYER_VIEW_TRANSIENT_ROTATION_X,
        ST_PLAYER_VIEW_STATE_ROTATION_Z, ST_PLAYER_VIEW_STATE_ROTATION_Y,
        ST_PLAYER_VIEW_STATE_ROTATION_X
    };
    for (int offset : clearedHalfwords) MEM_H(offset, player) = 0;
    MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = 0;
    MEM_B(ST_PLAYER_STATE, player) = 0;

    // Copy the two aligned words containing pitch, yaw, and the next angles.
    MEM_W(ST_PLAYER_VIEW_ROTATION_X, player) = MEM_W(ST_PLAYER_PITCH, player);
    ctx->r4 = MEM_W(ST_PLAYER_BASE_ROTATION_Z, player);
    MEM_W(ST_PLAYER_VIEW_ROTATION_Z, player) = ctx->r4;
    MEM_B(ST_PLAYER_FIRST_ACTION_REPEAT, player) = 0xFF;
    MEM_B(ST_PLAYER_SECOND_ACTION_REPEAT, player) = 0xFF;
    MEM_W(ST_PLAYER_SECOND_COLLISION_Y_FLAG, player) = 0;
    MEM_W(ST_PLAYER_FIRST_COLLISION_Y_FLAG, player) = 0;
    MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, ST_COLLISION_QUERY_BASE_ADDRESS) = 0;
    ctx->r3 = 0xFFFFFFFF;
    MEM_W(0x2F4, player) = ctx->r3;
}
