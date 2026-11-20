#pragma once

// Fields read from the position request passed to
// ST_SetPlayerPositionAndRequestLevelStreaming.
enum ST_PlayerPositionRequestOffset {
    ST_PLAYER_POSITION_REQUEST_GRID_X = 0x01,
    ST_PLAYER_POSITION_REQUEST_GRID_Y = 0x02,
    ST_PLAYER_POSITION_REQUEST_GRID_Z = 0x03,
    ST_PLAYER_POSITION_REQUEST_FINE_X = 0x04,
    ST_PLAYER_POSITION_REQUEST_FINE_Y = 0x06,
    ST_PLAYER_POSITION_REQUEST_FINE_Z = 0x08
};
