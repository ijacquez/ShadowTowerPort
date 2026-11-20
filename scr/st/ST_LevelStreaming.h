#pragma once

// Shared state used by the level-streaming and player-update routines.
enum {
    ST_LEVEL_STREAMING_STATUS_ADDRESS = 0x801B5920,
    ST_PLAYER_UPDATE_GATE_ADDRESS = 0x801C02A1,
    ST_LEVEL_STREAMING_CALLBACK_TABLE_OFFSET = 0x0C
};

enum {
    ST_LEVEL_CALLBACK_ACTIVE_RECORD_PERIODIC = 0x40,
    ST_LEVEL_CALLBACK_ACTIVE_RECORD_UPDATE = 0x44
};

// Callback addresses in stage 0's loaded slot-2 resource.
enum {
    ST_STAGE_ZERO_ACTIVE_RECORD_PERIODIC_CALLBACK = 0x801ED4A0,
    ST_STAGE_ZERO_ACTIVE_RECORD_UPDATE_CALLBACK = 0x801ED6E4
};

// Offsets from the emulated global pointer used by the level-record path.
enum {
    ST_LEVEL_RECORD_DIRECTION_X_GP_OFFSET = 0x320,
    ST_LEVEL_RECORD_DIRECTION_Z_GP_OFFSET = 0x324
};
