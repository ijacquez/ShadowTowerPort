#pragma once

// Offsets into the player state in emulated RAM (base address 0x80198F28).
// Access through MEM_* preserves the recompiler's memory and signedness rules.
enum ST_PlayerStateOffset {
    ST_PLAYER_MOVE_X = 0x1CC,
    ST_PLAYER_MOVE_Y = 0x1CE,
    ST_PLAYER_MOVE_Z = 0x1D0,
    ST_PLAYER_POSITION_X = 0x248,
    ST_PLAYER_POSITION_Y = 0x24C, // World Y increases downward.
    ST_PLAYER_POSITION_Z = 0x250,
    ST_PLAYER_YAW = 0x27A,
    ST_PLAYER_LOCAL_MOVE_X = 0x2B8,
    ST_PLAYER_LOCAL_MOVE_Z = 0x2C0,
    ST_PLAYER_MOVE_MAGNITUDE = 0x2C8,
    ST_PLAYER_TURN_RATE = 0x2CA,
    ST_PLAYER_VERTICAL_MOVE = 0x2D4, // Positive moves downward.
    ST_PLAYER_MOVE_LIMIT = 0x2E4,
    ST_PLAYER_TURN_LIMIT = 0x2E8
};

// Y offset used when querying the player's collision geometry. Its exact
// relationship to the player's full height remains unknown.
enum { ST_PLAYER_COLLISION_Y_OFFSET = 0x640 };
