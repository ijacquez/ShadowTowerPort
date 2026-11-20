#pragma once

// Offsets in the 88-byte records traversed by the level-record update.
enum {
    ST_LEVEL_RECORD_TYPE = 0x05,
    ST_LEVEL_RECORD_POSITION_X = 0x24,
    ST_LEVEL_RECORD_POSITION_Y = 0x28,
    ST_LEVEL_RECORD_POSITION_Z = 0x2C,
    ST_LEVEL_RECORD_CURSOR_OFFSET = 0x50
};
