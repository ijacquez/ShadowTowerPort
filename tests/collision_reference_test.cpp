#include "recomp.h"
#include "st/ST_ActiveCollisionRecord.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerState.h"
#include "st/ST_LevelRecord.h"
#include <cmath>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

void SquareRoot0(uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<uint32_t>(std::sqrt(static_cast<double>(ctx->r4)));
}

int main() {
    constexpr uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t source = 0x80100000;
    constexpr uint32_t record = 0x801BA138 + 2 * 212;
    struct Case { uint32_t flags, active, indexed, expectedX; };
    const Case cases[] = {
        {ST_COLLISION_RESULT_PLAYER_HIT, 2, 0xFFFFFFFF, 123},
        {0, 2, 0xFFFFFFFF, 456},
        {0, 0xFFFFFFFF, 0xFFFFFFFF, 789},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, query) = test.flags;
        MEM_W(ST_COLLISION_QUERY_ACTIVE_RECORD_INDEX, query) = test.active;
        MEM_W(ST_COLLISION_QUERY_SECOND_ACTIVE_RECORD_INDEX, query) = 2;
        MEM_W(ST_COLLISION_QUERY_INDEXED_RECORD_INDEX, query) = test.indexed;
        MEM_W(ST_COLLISION_QUERY_REFERENCE_X, query) = 789;
        MEM_W(ST_PLAYER_POSITION_X, player) = 123;
        MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, record) = 456;
        recomp_context ctx{};
        ctx.r4 = source;
        ctx.r29 = 0x80110000;
        ST_SetCollisionReference(rdram, &ctx);
        if (MEM_W(ST_COLLISION_QUERY_REFERENCE_X, query) != test.expectedX) {
            std::fprintf(stderr, "collision reference case %zu failed\n", &test - cases);
            return 1;
        }
    }
    constexpr uint32_t levelRecord = ST_LEVEL_RECORD_TABLE_ADDRESS + 3 * 88;
    constexpr uint32_t indexedData = ST_LEVEL_RECORD_INDEXED_DATA_TABLE_ADDRESS + 5 * 24;
    for (int scaled = 0; scaled < 2; ++scaled) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_COLLISION_QUERY_ACTIVE_RECORD_INDEX, query) = 0xFFFFFFFF;
        MEM_W(ST_COLLISION_QUERY_INDEXED_RECORD_INDEX, query) = 3;
        MEM_W(ST_LEVEL_RECORD_POSITION_X, levelRecord) = 30;
        MEM_W(ST_LEVEL_RECORD_POSITION_Z, levelRecord) = 40;
        MEM_W(ST_LEVEL_RECORD_POSITION_FOURTH_WORD, levelRecord) = 0x12345678;
        MEM_H(ST_LEVEL_RECORD_INDEXED_DATA_INDEX, levelRecord) = 5;
        MEM_B(ST_LEVEL_RECORD_REFERENCE_FLAGS, levelRecord) =
            scaled ? ST_LEVEL_RECORD_FLAG_SCALE_REFERENCE_EXTENT : 0;
        MEM_B(ST_LEVEL_RECORD_REFERENCE_EXTENT_SCALE, levelRecord) = 63;
        MEM_H(ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT, indexedData) = 257;
        MEM_H(ST_LEVEL_RECORD_INDEXED_REFERENCE_SECOND_VALUE, indexedData) = 321;
        recomp_context ctx{};
        ctx.r4 = source;
        ctx.r29 = 0x80110000;
        ST_SetCollisionReference(rdram, &ctx);
        if (MEM_W(ST_COLLISION_QUERY_REFERENCE_X, query) != 30 ||
            MEM_W(ST_COLLISION_QUERY_REFERENCE_Z, query) != 40 ||
            MEM_W(ST_COLLISION_QUERY_REFERENCE_FOURTH_WORD, query) != 0x12345678 ||
            MEM_W(ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_DISTANCE, query) != 50 ||
            MEM_HU(ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_EXTENT, query) != (scaled ? 126 : 257) ||
            MEM_HU(ST_COLLISION_QUERY_REFERENCE_SECOND_VALUE, query) != 321) {
            std::fprintf(stderr, "indexed collision reference case %d failed\n", scaled);
            return 1;
        }
    }
    std::puts("5 collision reference selection cases passed");
}
