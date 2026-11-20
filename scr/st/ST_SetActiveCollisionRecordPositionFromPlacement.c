#include "recomp.h"
#include "disable_warnings.h"
#include "ST_ActiveCollisionRecord.h"
#include "ST_LevelPlacement.h"

void ST_SetActiveCollisionRecordPositionFromPlacement(uint8_t* rdram, recomp_context* ctx) {
    uint64_t hi = 0, lo = 0, result = 0;
    unsigned int rounding_mode = DEFAULT_ROUNDING_MODE;
    int c1cs = 0; 
    // lhu         $v0, 0x28($a0)
    ctx->r2 = MEM_HU(ST_ACTIVE_COLLISION_RECORD_PLACEMENT_INDEX, ctx->r4);
    // nop

    // sll         $v1, $v0, 1
    ctx->r3 = S32(ctx->r2) << 1;
    // addu        $v1, $v1, $v0
    ctx->r3 = ADD32(ctx->r3, ctx->r2);
    // sll         $v1, $v1, 2
    ctx->r3 = S32(ctx->r3) << 2;
    // sh          $v0, 0x26($a0)
    MEM_H(0X26, ctx->r4) = ctx->r2;
    // lui         $v0, 0x8018
    ctx->r2 = S32(0X8018 << 16);
    // addiu       $v0, $v0, -0x4130
    ctx->r2 = ST_LEVEL_PLACEMENT_TABLE_ADDRESS;
    // addu        $v1, $v1, $v0
    ctx->r3 = ADD32(ctx->r3, ctx->r2);
    // lbu         $v0, 0x8($v1)
    ctx->r2 = MEM_BU(ST_LEVEL_PLACEMENT_X_CELL, ctx->r3);
    // lh          $a1, 0x6A($a0)
    ctx->r5 = MEM_HS(ST_ACTIVE_COLLISION_RECORD_LOCAL_X, ctx->r4);
    // sll         $v0, $v0, 12
    ctx->r2 = S32(ctx->r2) << 12;
    // addu        $v0, $v0, $a1
    ctx->r2 = ADD32(ctx->r2, ctx->r5);
    // sw          $v0, 0x74($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, ctx->r4) = ctx->r2;
    // lbu         $v0, 0x9($v1)
    ctx->r2 = MEM_BU(ST_LEVEL_PLACEMENT_Y_CELL, ctx->r3);
    // lh          $a1, 0x6C($a0)
    ctx->r5 = MEM_HS(ST_ACTIVE_COLLISION_RECORD_LOCAL_Y, ctx->r4);
    // sll         $v0, $v0, 12
    ctx->r2 = S32(ctx->r2) << 12;
    // addu        $v0, $v0, $a1
    ctx->r2 = ADD32(ctx->r2, ctx->r5);
    // sw          $v0, 0x78($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, ctx->r4) = ctx->r2;
    // lbu         $v0, 0xA($v1)
    ctx->r2 = MEM_BU(ST_LEVEL_PLACEMENT_Z_CELL, ctx->r3);
    // lh          $v1, 0x6E($a0)
    ctx->r3 = MEM_HS(ST_ACTIVE_COLLISION_RECORD_LOCAL_Z, ctx->r4);
    // sll         $v0, $v0, 12
    ctx->r2 = S32(ctx->r2) << 12;
    // addu        $v0, $v0, $v1
    ctx->r2 = ADD32(ctx->r2, ctx->r3);
    // jr          $ra
    // sw          $v0, 0x7C($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Z, ctx->r4) = ctx->r2;
    return;
    // sw          $v0, 0x7C($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Z, ctx->r4) = ctx->r2;
;}
