#pragma once

// Halfword offsets in the configurable control state at 0x801CD180.
enum ST_PlayerControlOffset {
    ST_CONTROL_HELD = 0x02,
    ST_CONTROL_MOVE_Z_POSITIVE = 0x22,
    ST_CONTROL_MOVE_Z_NEGATIVE = 0x24,
    ST_CONTROL_TURN_POSITIVE = 0x26,
    ST_CONTROL_TURN_NEGATIVE = 0x28,
    ST_CONTROL_MOVE_X_NEGATIVE = 0x2A,
    ST_CONTROL_MOVE_X_POSITIVE = 0x2C,
    ST_CONTROL_PITCH_NEGATIVE = 0x2E,
    ST_CONTROL_PITCH_POSITIVE = 0x30
};

// Option labeled ADJUST WALK in the game's settings.
enum { ST_ADJUST_WALK_SETTING_ADDRESS = 0x801CD122 };
