#include "recomp.h"
#include "disable_warnings.h"
#include "ST_PlayerState.h"

void ST_ResetPlayerStateAndViewRotationOffsets(uint8_t* rdram, recomp_context* ctx) {
    uint64_t hi = 0, lo = 0, result = 0;
    unsigned int rounding_mode = DEFAULT_ROUNDING_MODE;
    int c1cs = 0;
    // lui         $v0, 0x801A
    ctx->r2 = S32(0X801A << 16);
    // addiu       $v0, $v0, -0x70D8
    ctx->r2 = ADD32(ctx->r2, -0X70D8);
    // sb          $zero, 0x218($v0)
    MEM_B(ST_PLAYER_STATE, ctx->r2) = 0;
    // sh          $zero, 0x284($v0)
    MEM_H(ST_PLAYER_VIEW_STATE_ROTATION_Z, ctx->r2) = 0;
    // sh          $zero, 0x282($v0)
    MEM_H(ST_PLAYER_VIEW_STATE_ROTATION_Y, ctx->r2) = 0;
    // sh          $zero, 0x280($v0)
    MEM_H(ST_PLAYER_VIEW_STATE_ROTATION_X, ctx->r2) = 0;
    // sh          $zero, 0x28C($v0)
    MEM_H(ST_PLAYER_VIEW_TRANSIENT_ROTATION_Z, ctx->r2) = 0;
    // sh          $zero, 0x28A($v0)
    MEM_H(ST_PLAYER_VIEW_TRANSIENT_ROTATION_Y, ctx->r2) = 0;
    // sh          $zero, 0x288($v0)
    MEM_H(ST_PLAYER_VIEW_TRANSIENT_ROTATION_X, ctx->r2) = 0;
    // sh          $zero, 0x294($v0)
    MEM_H(ST_PLAYER_VIEW_ADDITIONAL_ROTATION_Z, ctx->r2) = 0;
    // sh          $zero, 0x292($v0)
    MEM_H(ST_PLAYER_VIEW_ADDITIONAL_ROTATION_Y, ctx->r2) = 0;
    // jr          $ra
    // sh          $zero, 0x290($v0)
    MEM_H(ST_PLAYER_VIEW_ADDITIONAL_ROTATION_X, ctx->r2) = 0;
    return;
    // sh          $zero, 0x290($v0)
    MEM_H(ST_PLAYER_VIEW_ADDITIONAL_ROTATION_X, ctx->r2) = 0;
;}
