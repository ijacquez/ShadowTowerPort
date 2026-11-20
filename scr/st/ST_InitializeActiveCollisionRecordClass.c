#include "recomp.h"
#include "disable_warnings.h"
#include "ST_ActiveCollisionClass.h"
#include "ST_ActiveCollisionRecord.h"

void ST_InitializeActiveCollisionRecordClass(uint8_t* rdram, recomp_context* ctx) {
    uint64_t hi = 0, lo = 0, result = 0;
    unsigned int rounding_mode = DEFAULT_ROUNDING_MODE;
    int c1cs = 0; 
    // lbu         $v0, 0x2($a0)
    ctx->r2 = MEM_BU(ST_ACTIVE_COLLISION_RECORD_CLASS_INDEX, ctx->r4);
    // nop

    // sll         $v1, $v0, 1
    ctx->r3 = S32(ctx->r2) << 1;
    // addu        $v1, $v1, $v0
    ctx->r3 = ADD32(ctx->r3, ctx->r2);
    // sll         $v1, $v1, 6
    ctx->r3 = S32(ctx->r3) << 6;
    // lui         $v0, 0x801C
    ctx->r2 = S32(0X801C << 16);
    // addiu       $v0, $v0, -0x29C8
    ctx->r2 = ST_ACTIVE_COLLISION_CLASS_TABLE_ADDRESS;
    // addu        $v1, $v1, $v0
    ctx->r3 = ADD32(ctx->r3, ctx->r2);
    // lbu         $v0, 0x10($v1)
    ctx->r2 = MEM_BU(0X10, ctx->r3);
    // nop

    // sb          $v0, 0x1($a0)
    MEM_B(0X1, ctx->r4) = ctx->r2;
    // lhu         $v0, 0x32($v1)
    ctx->r2 = MEM_HU(0X32, ctx->r3);
    // nop

    // sh          $v0, 0x2C($a0)
    MEM_H(0X2C, ctx->r4) = ctx->r2;
    // lhu         $v0, 0x16($v1)
    ctx->r2 = MEM_HU(ST_ACTIVE_COLLISION_CLASS_HORIZONTAL_EXTENT, ctx->r3);
    // nop

    // sh          $v0, 0x2E($a0)
    MEM_H(ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT, ctx->r4) = ctx->r2;
    // lhu         $v0, 0x18($v1)
    ctx->r2 = MEM_HU(ST_ACTIVE_COLLISION_CLASS_VERTICAL_EXTENT, ctx->r3);
    // nop

    // sh          $v0, 0x30($a0)
    MEM_H(ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT, ctx->r4) = ctx->r2;
    // lhu         $v0, 0x1A($v1)
    ctx->r2 = MEM_HU(0X1A, ctx->r3);
    // nop

    // sh          $v0, 0x32($a0)
    MEM_H(0X32, ctx->r4) = ctx->r2;
    // lw          $v1, 0x20($v1)
    ctx->r3 = MEM_W(ST_ACTIVE_COLLISION_CLASS_FILTER_FLAGS, ctx->r3);
    // addiu       $v0, $zero, -0x1
    ctx->r2 = ADD32(0, -0X1);
    // sh          $v0, 0x34($a0)
    MEM_H(0X34, ctx->r4) = ctx->r2;
    // sh          $v0, 0x36($a0)
    MEM_H(0X36, ctx->r4) = ctx->r2;
    // addiu       $v0, $zero, 0x1000
    ctx->r2 = ADD32(0, 0X1000);
    // sh          $v0, 0x98($a0)
    MEM_H(0X98, ctx->r4) = ctx->r2;
    // sh          $v0, 0x96($a0)
    MEM_H(0X96, ctx->r4) = ctx->r2;
    // sh          $v0, 0x94($a0)
    MEM_H(0X94, ctx->r4) = ctx->r2;
    // jr          $ra
    // sw          $v1, 0x70($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS, ctx->r4) = ctx->r3;
    return;
    // sw          $v1, 0x70($a0)
    MEM_W(ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS, ctx->r4) = ctx->r3;
;}
