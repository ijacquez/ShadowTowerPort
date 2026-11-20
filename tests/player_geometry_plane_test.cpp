#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_LevelGeometry.h"
#include "st/ST_LevelPlacement.h"
#include "st/ST_SpatialCandidate.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    // One synthetic placement uses the initial stage's horizontal plane.
    constexpr uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    constexpr uint32_t base = 0x800F0000;
    constexpr uint32_t model = base + 12;
    constexpr uint32_t placement = ST_LEVEL_PLACEMENT_TABLE_ADDRESS + 3 * 12;
    constexpr uint32_t vertices = base + 12 + 0x100;
    constexpr uint32_t normal = base + 12 + 0x140;
    constexpr uint32_t record = base + 12 + 0x148;
    constexpr uint32_t matrices = ST_LEVEL_QUARTER_TURN_MATRICES_ADDRESS;
    MEM_W(0, ST_SPATIAL_FILTER_DESCRIPTOR_POINTER_ADDRESS) = base;
    MEM_W(ST_GEOMETRY_MODEL_VERTEX_OFFSET, model) = 0x100;
    MEM_W(ST_GEOMETRY_MODEL_VERTEX_COUNT, model) = 4;
    MEM_W(ST_GEOMETRY_MODEL_VECTOR_OFFSET, model) = 0x140;
    MEM_W(ST_GEOMETRY_MODEL_RECORD_OFFSET, model) = 0x148;
    MEM_H(ST_GEOMETRY_MODEL_FOUR_VERTEX_COUNT, model) = 1;
    const int16_t vertex[4][3] = {
        {-1024, -1024, -2048}, {-1024, -1024, 2048},
        {1024, -1024, -2048}, {1024, -1024, 2048}
    };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j) MEM_H(i * 8 + j * 2, vertices) = vertex[i][j];
    MEM_H(2, normal) = -4096;
    MEM_H(ST_GEOMETRY_RECORD_FLAGS, record) = 1;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_0_OFFSET, record) = 0;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_1_OFFSET, record) = 8;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_2_OFFSET, record) = 16;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_3_OFFSET, record) = 24;
    MEM_B(ST_LEVEL_PLACEMENT_ORIENTATION, placement) = 1;
    MEM_B(ST_LEVEL_PLACEMENT_X_CELL, placement) = 64;
    MEM_B(ST_LEVEL_PLACEMENT_Y_CELL, placement) = 66;
    MEM_B(ST_LEVEL_PLACEMENT_Z_CELL, placement) = 63;
    MEM_B(ST_LEVEL_PLACEMENT_Y_SUBCELL, placement) = 128;
    MEM_H(4, matrices + 32) = -4096;
    MEM_H(8, matrices + 32) = 4096;
    MEM_H(12, matrices + 32) = 4096;
    constexpr uint32_t x = ST_SPATIAL_CANDIDATE_X_BITSETS_ADDRESS;
    MEM_W(64 * 64, x) = 1u << 3;
    MEM_W(ST_SPATIAL_CANDIDATE_Y_BITSETS_OFFSET + 65 * 64, x) = 1u << 3;
    MEM_W(ST_SPATIAL_CANDIDATE_Z_BITSETS_OFFSET + 63 * 64, x) = 1u << 3;
    MEM_W(ST_COLLISION_QUERY_POSITION_X, query) = 0x40000;
    MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = 0x419C0;
    MEM_W(ST_COLLISION_QUERY_POSITION_Z, query) = 0x3F000;
    MEM_W(ST_COLLISION_QUERY_INPUT_FLAGS, query) = ST_COLLISION_QUERY_PLAYER_MOVEMENT_CHECKS;
    MEM_H(ST_COLLISION_QUERY_HORIZONTAL_PARAMETER, query) = 0x100;
    MEM_H(ST_COLLISION_QUERY_VERTICAL_PARAMETER, query) = 0x640;
    MEM_H(ST_COLLISION_QUERY_DIRECTION, query) = ST_COLLISION_QUERY_DIRECTION_FROM_DISPLACEMENT;
    static uint8_t initial[sizeof rdram];
    std::memcpy(initial, rdram, sizeof rdram);
    struct Case { int dy, proposed; };
    const Case cases[] = {
        {0, 576}, {256, 320}, {576, 0}, {-256, 832}, {832, -256}, {833, 576}
    };
    for (const auto& test : cases) {
        std::memcpy(rdram, initial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_H(ST_COLLISION_QUERY_DISPLACEMENT_Y, query) = test.dy;
        recomp_context ctx{};
        ctx.r4 = query;
        ctx.r29 = 0x801EF000;
        ST_QueryLevelGeometry(rdram, &ctx);
        if (ctx.r2 != ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD ||
            MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 1 ||
            S32(MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query)) != test.proposed ||
            S32(MEM_W(ST_COLLISION_QUERY_CURRENT_Y_RESULT, query)) != 576 ||
            MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, query) != 0) {
            std::fprintf(stderr, "plane geometry query failed for displacement %d\n", test.dy);
            return 1;
        }
    }
    std::puts("6 isolated plane geometry queries passed");
}
