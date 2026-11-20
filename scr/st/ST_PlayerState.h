#pragma once

// Offsets into the player state in emulated RAM (base address 0x80198F28).
// Access through MEM_* preserves the recompiler's memory and signedness rules.
enum ST_PlayerStateOffset {
    ST_PLAYER_MOVE_PENALTY_TOTAL = 0x22,
    ST_PLAYER_MOVE_PENALTY_THRESHOLD = 0x24,
    ST_PLAYER_MOVE_X = 0x1CC,
    ST_PLAYER_MOVE_Y = 0x1CE,
    ST_PLAYER_MOVE_Z = 0x1D0,
    ST_PLAYER_DIRECTIONAL_MOVE_DISABLED = 0x21E,
    ST_PLAYER_POSITION_X = 0x248,
    // World Y increases downward.
    ST_PLAYER_POSITION_Y = 0x24C,
    ST_PLAYER_POSITION_Z = 0x250,
    ST_PLAYER_VIEW_ROTATION_X = 0x268,
    ST_PLAYER_VIEW_ROTATION_Y = 0x26A,
    ST_PLAYER_VIEW_ROTATION_Z = 0x26C,
    ST_PLAYER_PITCH = 0x278,
    ST_PLAYER_YAW = 0x27A,
    ST_PLAYER_LOCAL_MOVE_X = 0x2B8,
    ST_PLAYER_LOCAL_MOVE_Z = 0x2C0,
    ST_PLAYER_MOVE_MAGNITUDE = 0x2C8,
    ST_PLAYER_TURN_RATE = 0x2CA,
    ST_PLAYER_PITCH_RATE = 0x2CC,
    ST_PLAYER_VIEW_OSCILLATION_Y = 0x2CE,
    ST_PLAYER_VIEW_OSCILLATION_PHASE = 0x2D0,
    ST_PLAYER_VIEW_Y_ADJUSTMENT_B = 0x2D2,
    ST_PLAYER_VERTICAL_MOVE = 0x2D4,
    ST_PLAYER_MOVE_LIMIT = 0x2E4,
    ST_PLAYER_TURN_LIMIT = 0x2E8
};

// Y offset 0x640 (0.390625 spatial cells). Its relationship to the player's
// full height remains unknown.
enum { ST_PLAYER_COLLISION_Y_OFFSET = 0x640 };
enum { ST_PLAYER_VIEW_Y_OFFSET = 0x640 };

// Vertical movement values use 12 fractional bits in spatial-cell units.
enum {
    ST_PLAYER_VERTICAL_START_SPEED = 0x10,       // 0.00390625
    ST_PLAYER_VERTICAL_SNAP_DISTANCE = 0x20,     // 0.0078125
    ST_PLAYER_VERTICAL_ACCELERATION = 0x30,      // 0.01171875
    ST_PLAYER_VERTICAL_LIMIT_DISTANCE = 0x400,   // 0.25
    ST_PLAYER_HIGH_DOWNWARD_SPEED = 0x480       // 0.28125 per update
};
