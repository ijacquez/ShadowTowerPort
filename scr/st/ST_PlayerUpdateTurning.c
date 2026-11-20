#include "recomp.h"
#include "disable_warnings.h"
#include "ST_PlayerState.h"
#include "ST_PlayerControls.h"

void ST_PlayerUpdateTurning(uint8_t* rdram, recomp_context* ctx) {
    // Preserve the emulated callee-saved registers used by this routine.
    ctx->r29 = ADD32(ctx->r29, -0x18);
    MEM_W(0x14, ctx->r29) = ctx->r31;
    MEM_W(0x10, ctx->r29) = ctx->r16;
    // The active button mask and bindings are stored separately, so the
    // same update works with remapped controls. Positive turning wins when
    // both turn bindings are held.
    const uint32_t controls = 0x801CD180;
    const uint32_t player = 0x80198F28;
    const uint16_t held = MEM_HU(ST_CONTROL_HELD, controls);
    const int32_t limit = S32(MEM_W(ST_PLAYER_TURN_LIMIT, player));
    const int32_t step = limit >> 2;
    uint16_t rate = MEM_HU(ST_PLAYER_TURN_RATE, player);

    if (held & MEM_HU(ST_CONTROL_TURN_POSITIVE, controls)) {
        rate = (uint16_t)(rate + (uint32_t)step);
        if ((int16_t)rate > limit)
            rate = (uint16_t)limit;
    } else if (held & MEM_HU(ST_CONTROL_TURN_NEGATIVE, controls)) {
        rate = (uint16_t)(rate - (uint32_t)step);
        if ((int16_t)rate < S32(SUB32(0, limit)))
            rate = (uint16_t)SUB32(0, limit);
    } else if ((int16_t)rate > 0) {
        rate = (uint16_t)(rate - (uint32_t)step);
        if ((int16_t)rate < 0)
            rate = 0;
    } else if ((int16_t)rate < 0) {
        rate = (uint16_t)(rate + (uint32_t)step);
        if ((int16_t)rate > 0)
            rate = 0;
    }
    MEM_H(ST_PLAYER_TURN_RATE, player) = rate;

    // Apply yaw before updating the second angular control.
    MEM_H(ST_PLAYER_YAW, player) =
        (MEM_HU(ST_PLAYER_YAW, player) + rate) & 0xFFF;
    const bool pitchNegative = (held & MEM_HU(ST_CONTROL_PITCH_NEGATIVE, controls)) != 0;
    const bool pitchPositive = (held & MEM_HU(ST_CONTROL_PITCH_POSITIVE, controls)) != 0;
    const int16_t pitch = (int16_t)MEM_HU(ST_PLAYER_PITCH, player);
    uint16_t pitchRate = MEM_HU(ST_PLAYER_PITCH_RATE, player);
    bool skipPositiveRateCheck = false;

    if (pitchNegative && pitchPositive) {
        if (pitch > 0x800) {
            const int32_t target = (0x1080 - pitch) >> 2;
            pitchRate = (uint16_t)(pitchRate + 8u);
            if ((int16_t)pitchRate > target)
                pitchRate = (uint16_t)(target != 0 ? target : 0x1080 - (uint16_t)pitch);
            ctx->r7 = 0x1080;
        } else if (pitch < 0x800) {
            const int32_t target = -((pitch - 0x80) >> 2);
            pitchRate = (uint16_t)(pitchRate - 8u);
            if ((int16_t)pitchRate < target)
                pitchRate = (uint16_t)(target != 0 ? target : 0x80 - (uint16_t)pitch);
        }
    } else if (pitchNegative) {
        pitchRate = (uint16_t)(pitchRate - 3u);
        if ((int16_t)pitchRate < -0x20) pitchRate = (uint16_t)-0x20;
    } else if (pitchPositive) {
        pitchRate = (uint16_t)(pitchRate + 3u);
        if ((int16_t)pitchRate > 0x20) pitchRate = 0x20;
    } else if ((int16_t)pitchRate > 0) {
        pitchRate = (uint16_t)(pitchRate - 3u);
        if ((int16_t)pitchRate < 0) pitchRate = 0;
    } else if ((int16_t)pitchRate < 0) {
        pitchRate = (uint16_t)(pitchRate + 3u);
        if ((int16_t)pitchRate > 0) pitchRate = 0;
        else skipPositiveRateCheck = true;
    }
    MEM_H(ST_PLAYER_PITCH_RATE, player) = pitchRate;
    ctx->r6 = player;
    ctx->r5 = controls;
    // Keep the original emulated-register arguments and return values when
    // invoking the angle comparison helper.
    ctx->r16 = player;
    ctx->r2 = (uint32_t)(int32_t)(int16_t)pitchRate;
    ctx->r3 = pitchRate;
    if (!skipPositiveRateCheck)
        ctx->r5 = 0x2BC;

    if ((int16_t)pitchRate > 0) {
        ctx->r4 = (MEM_HU(ST_PLAYER_PITCH, player) + pitchRate) & 0xFFF;
        MEM_H(ST_PLAYER_PITCH, player) = ctx->r4;
        ST_IsShiftedAngleGreater(rdram, ctx);
        const bool outOfRange = ctx->r2 != 0;
        ctx->r2 = 0x2BC;
        if (outOfRange) MEM_H(ST_PLAYER_PITCH, player) = ctx->r2;
    } else {
        ctx->r4 = 0xD44;
        if ((int16_t)pitchRate < 0) {
            ctx->r5 = (MEM_HU(ST_PLAYER_PITCH, player) + pitchRate) & 0xFFF;
            MEM_H(ST_PLAYER_PITCH, player) = ctx->r5;
            ST_IsShiftedAngleGreater(rdram, ctx);
            const bool outOfRange = ctx->r2 != 0;
            ctx->r2 = 0xD44;
            if (outOfRange) MEM_H(ST_PLAYER_PITCH, player) = ctx->r2;
        }
    }

    ctx->r31 = MEM_W(0x14, ctx->r29);
    ctx->r16 = MEM_W(0x10, ctx->r29);
    ctx->r29 = ADD32(ctx->r29, 0x18);
}
