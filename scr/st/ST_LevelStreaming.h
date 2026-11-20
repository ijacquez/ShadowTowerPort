#pragma once

// Shared state used by the level-streaming and player-update routines.
enum {
    ST_LEVEL_STREAMING_STATUS_ADDRESS = 0x801B5920,
    ST_PLAYER_UPDATE_GATE_ADDRESS = 0x801C02A1
};

// Offsets from the emulated global pointer used by the level-record path.
enum {
    ST_LEVEL_RECORD_DIRECTION_X_GP_OFFSET = 0x320,
    ST_LEVEL_RECORD_DIRECTION_Z_GP_OFFSET = 0x324,
    ST_LEVEL_RECORD_FIRST_SLOT_ADDRESS = 0x801BCF98,
    ST_LEVEL_RECORD_SLOT_DIRECTION_ANGLE_OFFSET = 0x56
};
