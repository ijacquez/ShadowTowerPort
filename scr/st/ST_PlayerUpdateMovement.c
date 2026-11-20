#include "recomp.h"
#include "disable_warnings.h"
#include "ST_PlayerState.h"
#include "ST_PlayerControls.h"
#include "ST_PlayerMovementContributions.h"

enum { ST_PLAYER_ROTATION_INPUT_Y_STACK_OFFSET = 0x1C };

// Update one signed 32-bit input accumulator, preserving the game's
// 32-bit wrapping and arithmetic-shift behavior.
static uint32_t ST_UpdateMovementAxis(uint32_t value, int32_t limit, int direction) {
    const uint32_t acceleration = (uint32_t)(limit >> 2);
    const uint32_t deceleration = (uint32_t)(limit >> 3);
    if (direction > 0) {
        value += acceleration;
        if (S32(value) > limit) value = (uint32_t)limit;
    } else if (direction < 0) {
        value -= acceleration;
        const uint32_t negativeLimit = 0u - (uint32_t)limit;
        if (S32(value) < S32(negativeLimit)) value = negativeLimit;
    } else if (S32(value) > 0) {
        value -= deceleration;
        if (S32(value) < 0) value = 0;
    } else if (S32(value) < 0) {
        value += deceleration;
        if (S32(value) > 0) value = 0;
    }
    return value;
}

// Dampen one stored component by one quarter, rounding toward zero.
static uint16_t ST_DampenMovementComponent(uint16_t raw) {
    const int32_t value = (int16_t)raw;
    const int16_t damped = (int16_t)(raw - value / 4);
    return damped >= -3 && damped <= 3 ? 0 : (uint16_t)damped;
}

void ST_PlayerUpdateMovement(uint8_t* rdram, recomp_context* ctx) {

    // Local X/Z accumulators approach +/-move limit by limit >> 2 while
    // pressed, and approach zero by limit >> 3 when released. They are
    // processed and rotated by yaw before collision-tested movement; they
    // are not world-space position increments. See docs/PLAYER_MOVEMENT.md.
    uint64_t hi = 0, lo = 0, result = 0;
    unsigned int rounding_mode = DEFAULT_ROUNDING_MODE;
    int c1cs = 0; 
    // addiu       $sp, $sp, -0x70
    ctx->r29 = ADD32(ctx->r29, -0X70);
    // lui         $v0, 0x801D
    ctx->r2 = S32(0X801D << 16);
    // addiu       $v1, $v0, -0x2E80
    ctx->r3 = ADD32(ctx->r2, -0X2E80);
    // sw          $ra, 0x6C($sp)
    MEM_W(0X6C, ctx->r29) = ctx->r31;
    // sw          $s2, 0x68($sp)
    MEM_W(0X68, ctx->r29) = ctx->r18;
    // sw          $s1, 0x64($sp)
    MEM_W(0X64, ctx->r29) = ctx->r17;
    // sw          $s0, 0x60($sp)
    MEM_W(0X60, ctx->r29) = ctx->r16;
    // Binding masks are configurable. Z gives the positive binding priority;
    // X treats simultaneous positive and negative bindings as neutral.
    const uint32_t controls = 0x801CD180;
    const uint32_t player = 0x80198F28;
    const uint16_t held = MEM_HU(ST_CONTROL_HELD, controls);
    const int32_t limit = S32(MEM_W(ST_PLAYER_MOVE_LIMIT, player));
    const bool zPositive = (held & MEM_HU(ST_CONTROL_MOVE_Z_POSITIVE, controls)) != 0;
    const bool zNegative = (held & MEM_HU(ST_CONTROL_MOVE_Z_NEGATIVE, controls)) != 0;
    const bool xPositive = (held & MEM_HU(ST_CONTROL_MOVE_X_POSITIVE, controls)) != 0;
    const bool xNegative = (held & MEM_HU(ST_CONTROL_MOVE_X_NEGATIVE, controls)) != 0;
    const int zDirection = zPositive ? 1 : (zNegative ? -1 : 0);
    const int xDirection = xPositive == xNegative ? 0 : (xPositive ? 1 : -1);
    MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player) = ST_UpdateMovementAxis(
        MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player), limit, zDirection);
    MEM_W(ST_PLAYER_LOCAL_MOVE_X, player) = ST_UpdateMovementAxis(
        MEM_W(ST_PLAYER_LOCAL_MOVE_X, player), limit, xDirection);

    // Square the signed low halfwords of the local X/Z accumulators in the GTE.
    ctx->r8 = player + ST_PLAYER_LOCAL_MOVE_X;
    gte_lwc2(rdram, ctx, 9, 8, 0);
    gte_lwc2(rdram, ctx, 10, 8, 4);
    gte_lwc2(rdram, ctx, 11, 8, 8);
    gte_command(ctx, 0x4AA00428);
    ctx->r2 = ADD32(ctx->r29, 0x28);
    gte_swc2(rdram, ctx, 25, 2, 0);
    gte_swc2(rdram, ctx, 26, 2, 4);
    gte_swc2(rdram, ctx, 27, 2, 8);
    ctx->r2 = MEM_W(0x28, ctx->r29);
    ctx->r4 = MEM_W(0x30, ctx->r29);
    ctx->r4 = ADD32(ctx->r2, ctx->r4);
    SquareRoot0(rdram, ctx);
    // The GTE produced squared signed-halfword components in the
    // stack slots. SquareRoot0 supplies a signed-halfword divisor.
    ctx->r3 = player;
    MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = ctx->r2;
    ctx->r2 = U32(ctx->r2) << 16;
    ctx->r4 = S32(ctx->r2) >> 16;
    if (ctx->r4 == 0) {
        MEM_W(0x18, ctx->r29) = 0;
        MEM_W(0x20, ctx->r29) = 0;
    } else {
        uint32_t xSquared = MEM_W(0x28, ctx->r29);
        uint32_t zSquared = MEM_W(0x30, ctx->r29);
        if (S32(MEM_W(ST_PLAYER_LOCAL_MOVE_X, player)) < 0) {
            xSquared = 0u - xSquared;
            MEM_W(0x28, ctx->r29) = xSquared;
        }
        if (S32(MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player)) < 0) {
            zSquared = 0u - zSquared;
            MEM_W(0x30, ctx->r29) = zSquared;
        }
        const int32_t divisor = S32(ctx->r4);
        ctx->r1 = 0x80000000;
        if (divisor == -1 && xSquared == 0x80000000)
            do_break(2147754896);
        ctx->r3 = (uint32_t)((int64_t)S32(xSquared) / divisor);
        if (divisor == -1 && zSquared == 0x80000000)
            do_break(2147754948);
        ctx->r2 = (uint32_t)((int64_t)S32(zSquared) / divisor);
        MEM_W(0x18, ctx->r29) = ctx->r3;
        MEM_W(0x20, ctx->r29) = ctx->r2;
    }
L_800423D4:
    // addiu       $a0, $sp, 0x58
    ctx->r4 = ADD32(ctx->r29, 0X58);
    // lui         $v0, 0x801A
    ctx->r2 = S32(0X801A << 16);
    // addiu       $s1, $v0, -0x70D8
    ctx->r17 = ADD32(ctx->r2, -0X70D8);
    // addiu       $s0, $sp, 0x38
    ctx->r16 = ADD32(ctx->r29, 0X38);
    // lhu         $v0, 0x27A($s1)
    ctx->r2 = MEM_HU(ST_PLAYER_YAW, ctx->r17);
    // addu        $a1, $s0, $zero
    ctx->r5 = ADD32(ctx->r16, 0);
    // sh          $zero, 0x58($sp)
    MEM_H(0X58, ctx->r29) = 0;
    // sh          $zero, 0x5C($sp)
    MEM_H(0X5C, ctx->r29) = 0;
    // negu        $v0, $v0
    ctx->r2 = SUB32(0, ctx->r2);
    // jal         0x80074EC4
    // sh          $v0, 0x5A($sp)
    MEM_H(0X5A, ctx->r29) = ctx->r2;
    RotMatrix(rdram, ctx);
    goto after_1;
    // sh          $v0, 0x5A($sp)
    MEM_H(0X5A, ctx->r29) = ctx->r2;
    after_1:
    // lhu         $v0, 0x18($sp)
    ctx->r2 = MEM_HU(0X18, ctx->r29);
    // Some control paths leave a copied table pointer here; other prior
    // writers remain unknown. This contributes only to rotated Y when the
    // executable's zero-angle table entry is intact.
    // lhu         $v1, 0x1C($sp)
    ctx->r3 = MEM_HU(ST_PLAYER_ROTATION_INPUT_Y_STACK_OFFSET, ctx->r29);
    // lhu         $a0, 0x20($sp)
    ctx->r4 = MEM_HU(0X20, ctx->r29);
    // sh          $v0, 0x10($sp)
    MEM_H(0X10, ctx->r29) = ctx->r2;
    // sh          $v1, 0x12($sp)
    MEM_H(0X12, ctx->r29) = ctx->r3;
    // sh          $a0, 0x14($sp)
    MEM_H(0X14, ctx->r29) = ctx->r4;
    // lw          $t4, 0x0($s0)
    ctx->r12 = MEM_W(0X0, ctx->r16);
    // lw          $t5, 0x4($s0)
    ctx->r13 = MEM_W(0X4, ctx->r16);
    // ctc2        $t4, $0
    gte_ctc2(ctx, 12, 0);
    // ctc2        $t5, $1
    gte_ctc2(ctx, 13, 1);
    // lw          $t4, 0x8($s0)
    ctx->r12 = MEM_W(0X8, ctx->r16);
    // lw          $t5, 0xC($s0)
    ctx->r13 = MEM_W(0XC, ctx->r16);
    // lw          $t6, 0x10($s0)
    ctx->r14 = MEM_W(0X10, ctx->r16);
    // ctc2        $t4, $2
    gte_ctc2(ctx, 12, 2);
    // ctc2        $t5, $3
    gte_ctc2(ctx, 13, 3);
    // ctc2        $t6, $4
    gte_ctc2(ctx, 14, 4);
    // addiu       $v0, $sp, 0x10
    ctx->r2 = ADD32(ctx->r29, 0X10);
    // lwc2        $0, 0x0($v0)
    gte_lwc2(rdram, ctx, 0, 2, 0);
    // lwc2        $1, 0x4($v0)
    gte_lwc2(rdram, ctx, 1, 2, 4);
    // nop

    // nop

    // .word       0x4A486012                   # INVALID     $s2, $t0, 0x6012 # 00000000 <InstrIdType: CPU_COP2>
    gte_command(ctx, 0x4A486012);
    // addiu       $s0, $sp, 0x18
    ctx->r16 = ADD32(ctx->r29, 0X18);
    // swc2        $25, 0x0($s0)
    gte_swc2(rdram, ctx, 25, 16, 0);
    // swc2        $26, 0x4($s0)
    gte_swc2(rdram, ctx, 26, 16, 4);
    // swc2        $27, 0x8($s0)
    gte_swc2(rdram, ctx, 27, 16, 8);
    // lw          $a0, 0x20($sp)
    ctx->r4 = MEM_W(0X20, ctx->r29);
    // lw          $a1, 0x18($sp)
    ctx->r5 = MEM_W(0X18, ctx->r29);
    // jal         0x80076EA4
    // nop

    ratan2(rdram, ctx);
    goto after_2;
    // nop

    after_2:
    // addu        $s2, $v0, $zero
    ctx->r18 = ADD32(ctx->r2, 0);
    // lwc2        $9, 0x0($s0)
    gte_lwc2(rdram, ctx, 9, 16, 0);
    // lwc2        $10, 0x4($s0)
    gte_lwc2(rdram, ctx, 10, 16, 4);
    // lwc2        $11, 0x8($s0)
    gte_lwc2(rdram, ctx, 11, 16, 8);
    // nop

    // nop

    // .word       0x4AA00428                   # INVALID     $s5, $zero, 0x428 # 00000000 <InstrIdType: CPU_COP2>
    gte_command(ctx, 0x4AA00428);
    // addiu       $v0, $sp, 0x28
    ctx->r2 = ADD32(ctx->r29, 0X28);
    // swc2        $25, 0x0($v0)
    gte_swc2(rdram, ctx, 25, 2, 0);
    // swc2        $26, 0x4($v0)
    gte_swc2(rdram, ctx, 26, 2, 4);
    // swc2        $27, 0x8($v0)
    gte_swc2(rdram, ctx, 27, 2, 8);
    // lw          $v0, 0x28($sp)
    ctx->r2 = MEM_W(0X28, ctx->r29);
    // lw          $a0, 0x30($sp)
    ctx->r4 = MEM_W(0X30, ctx->r29);
    // jal         0x800746C4
    // addu        $a0, $v0, $a0
    ctx->r4 = ADD32(ctx->r2, ctx->r4);
    SquareRoot0(rdram, ctx);
    goto after_3;
    // addu        $a0, $v0, $a0
    ctx->r4 = ADD32(ctx->r2, ctx->r4);
after_3:
    // The rotated X/Z magnitude and existing three-axis movement are
    // stored separately from the local input accumulators.
    ctx->r7 = ctx->r2;
    MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = ctx->r7;
    MEM_H(ST_PLAYER_MOVE_X, player) = ST_DampenMovementComponent(MEM_HU(ST_PLAYER_MOVE_X, player));
    MEM_H(ST_PLAYER_MOVE_Y, player) = ST_DampenMovementComponent(MEM_HU(ST_PLAYER_MOVE_Y, player));
    MEM_H(ST_PLAYER_MOVE_Z, player) = ST_DampenMovementComponent(MEM_HU(ST_PLAYER_MOVE_Z, player));

    // Add the two independent X/Z contributions with their original
    // signed division, halfword stores, and emulated-register values.
    ctx->r6 = MEM_HU(ST_PLAYER_MOVE_X, player) +
              (uint32_t)(MEM_HS(0, ST_PLAYER_QUARTER_CONTRIBUTION_X_ADDRESS) / 4);
    MEM_H(ST_PLAYER_MOVE_X, player) = ctx->r6;
    ctx->r5 = MEM_HU(ST_PLAYER_MOVE_Z, player) +
              (uint32_t)(MEM_HS(0, ST_PLAYER_QUARTER_CONTRIBUTION_Z_ADDRESS) / 4);
    MEM_H(ST_PLAYER_MOVE_Z, player) = ctx->r5;
    MEM_H(ST_PLAYER_MOVE_X, player) = ctx->r6 +
                                     (uint32_t)(MEM_HS(0, ST_PLAYER_HALF_CONTRIBUTION_X_ADDRESS) / 2);
    const int32_t externalZ = MEM_HS(0, ST_PLAYER_HALF_CONTRIBUTION_Z_ADDRESS);
    ctx->r5 += (uint32_t)(externalZ / 2);
    MEM_H(ST_PLAYER_MOVE_Z, player) = ctx->r5;
    ctx->r3 = externalZ < 0 ? 1 : 0;
    ctx->r4 = 0x801BA138;
    ctx->r2 = U32(ctx->r7) << 16;
    ctx->r5 = S32(ctx->r2) >> 16;
    if (ctx->r5 != 0) {
        ctx->r2 = MEM_BU(ST_PLAYER_DIRECTIONAL_MOVE_DISABLED, player);
        if (ctx->r2 == 0) {
            ctx->r4 = ctx->r18;
            ST_ApplyDirectionalMovementWithCollision(rdram, ctx);
        }
    }
L_80042604:
    // lw          $ra, 0x6C($sp)
    ctx->r31 = MEM_W(0X6C, ctx->r29);
    // lw          $s2, 0x68($sp)
    ctx->r18 = MEM_W(0X68, ctx->r29);
    // lw          $s1, 0x64($sp)
    ctx->r17 = MEM_W(0X64, ctx->r29);
    // lw          $s0, 0x60($sp)
    ctx->r16 = MEM_W(0X60, ctx->r29);
    // addiu       $sp, $sp, 0x70
    ctx->r29 = ADD32(ctx->r29, 0X70);
    // jr          $ra
    // nop

    return;
    // nop

;}
