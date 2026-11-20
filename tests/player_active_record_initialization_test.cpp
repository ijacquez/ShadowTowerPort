#include "recomp.h"
#include "st/ST_ActiveCollisionClass.h"
#include "st/ST_ActiveCollisionRecord.h"
#include "st/ST_LevelPlacement.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    constexpr uint32_t record = 0x801BA138;
    constexpr uint32_t classEntry = ST_ACTIVE_COLLISION_CLASS_TABLE_ADDRESS + 2 * 192;
    constexpr uint32_t placement = ST_LEVEL_PLACEMENT_TABLE_ADDRESS + 3 * 12;
    std::memset(rdram, 0, sizeof rdram);
    MEM_B(ST_ACTIVE_COLLISION_RECORD_CLASS_INDEX, record) = 2;
    MEM_H(ST_ACTIVE_COLLISION_CLASS_HORIZONTAL_EXTENT, classEntry) = 0x120;
    MEM_H(ST_ACTIVE_COLLISION_CLASS_VERTICAL_EXTENT, classEntry) = 0x345;
    MEM_W(ST_ACTIVE_COLLISION_CLASS_FILTER_FLAGS, classEntry) = 0x12345678;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_PLACEMENT_INDEX, record) = 3;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_LOCAL_X, record) = -100;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_LOCAL_Y, record) = -200;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_LOCAL_Z, record) = 300;
    MEM_B(ST_LEVEL_PLACEMENT_X_CELL, placement) = 5;
    MEM_B(ST_LEVEL_PLACEMENT_Y_CELL, placement) = 6;
    MEM_B(ST_LEVEL_PLACEMENT_Z_CELL, placement) = 7;
    MEM_B(ST_LEVEL_PLACEMENT_Y_SUBCELL, placement) = 255;
    recomp_context ctx{};
    ctx.r4 = record;
    ST_InitializeActiveCollisionRecordClass(rdram, &ctx);
    ST_SetActiveCollisionRecordPositionFromPlacement(rdram, &ctx);
    if (MEM_HU(ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT, record) != 0x120 ||
        MEM_HU(ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT, record) != 0x345 ||
        MEM_W(ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS, record) != 0x12345678 ||
        MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, record) != 5 * 4096 - 100 ||
        MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, record) != 6 * 4096 - 200 ||
        MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Z, record) != 7 * 4096 + 300) {
        std::fprintf(stderr, "active collision record initialization failed\n");
        return 1;
    }
    std::puts("active collision record class and position passed");
}
