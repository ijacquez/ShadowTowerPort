#include "recomp.h"
#include "ST_PlayerControls.h"

void ST_InitDefaultControlBindings(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t controls = 0x801CD180;
    MEM_H(ST_CONTROL_MOVE_Z_POSITIVE, controls) = 0x10;
    MEM_H(ST_CONTROL_MOVE_Z_NEGATIVE, controls) = 0x40;
    MEM_H(ST_CONTROL_TURN_POSITIVE, controls) = 0x80;
    MEM_H(ST_CONTROL_TURN_NEGATIVE, controls) = 0x20;
    MEM_H(ST_CONTROL_MOVE_X_NEGATIVE, controls) = 0x400;
    MEM_H(ST_CONTROL_MOVE_X_POSITIVE, controls) = 0x800;
    MEM_H(ST_CONTROL_PITCH_NEGATIVE, controls) = 0x100;
    MEM_H(ST_CONTROL_PITCH_POSITIVE, controls) = 0x200;
    MEM_H(0x32, controls) = 0x1000;
    MEM_H(0x34, controls) = 0x8000;
    MEM_H(0x36, controls) = 0x4000;
    MEM_H(0x38, controls) = 0x2000;
    ctx->r2 = 0x2000;
    ctx->r3 = controls;
}
