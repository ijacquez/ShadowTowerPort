#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_LevelPlacement.h"
#include "st/ST_LevelStreaming.h"
#include "st/ST_SpatialCandidate.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    constexpr uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    constexpr uint32_t vectors = 0x80100000;
    constexpr uint32_t x = ST_SPATIAL_CANDIDATE_X_BITSETS_ADDRESS;
    constexpr uint32_t y = x + ST_SPATIAL_CANDIDATE_Y_BITSETS_OFFSET;
    constexpr uint32_t z = x + ST_SPATIAL_CANDIDATE_Z_BITSETS_OFFSET;
    constexpr uint32_t descriptor = 0x80101000;
    for (int scenario = 0; scenario < 3; ++scenario) {
        std::memset(rdram, 0, sizeof(rdram));
        MEM_W(0, x) = (1u << 3) | (1u << 5);
        MEM_W(64 + 4, x) = 1u << 3;       // index 35
        MEM_W(128 + 12, x) = 1u << 4;     // index 100
        MEM_W(-64 + 4, x) = 1u << 28;     // index 60
        for (int i = 0; i < 16; ++i) {
            MEM_W(i * 4, y) = 0xFFFFFFFF;
            MEM_W(i * 4, z) = 0xFFFFFFFF;
        }
        MEM_W(0, y) &= ~(1u << 5);
        MEM_W(0, vectors) = scenario == 2 ? (uint32_t)-4095 : 0;
        MEM_W(0x10, vectors) = scenario == 2 ? (uint32_t)-4096 : 4096;
        MEM_W(0x20, vectors) = 8192;
        if (scenario == 1) {
            MEM_H(0, ST_LEVEL_STREAMING_STATUS_ADDRESS) = 1;
            MEM_B(ST_LEVEL_STREAMING_CANDIDATE_FILTER_ENABLED,
                  ST_LEVEL_STREAMING_STATUS_ADDRESS) = 1;
            MEM_W(0, ST_SPATIAL_FILTER_DESCRIPTOR_POINTER_ADDRESS) = descriptor;
            MEM_W(4, descriptor) = 8;
            MEM_B(12 * 3 + ST_LEVEL_PLACEMENT_RESOURCE_INDEX, ST_LEVEL_PLACEMENT_TABLE_ADDRESS) = 3;
            MEM_B(12 * 35 + ST_LEVEL_PLACEMENT_RESOURCE_INDEX, ST_LEVEL_PLACEMENT_TABLE_ADDRESS) = 4;
            MEM_B(12 * 100 + ST_LEVEL_PLACEMENT_RESOURCE_INDEX, ST_LEVEL_PLACEMENT_TABLE_ADDRESS) = 5;
        }
        recomp_context ctx{};
        ctx.r29 = 0x80180000;
        ctx.r4 = vectors;
        ctx.r5 = vectors + 0x10;
        ctx.r6 = vectors + 0x20;
        ST_SelectSpatialCandidates(rdram, &ctx);
        const uint16_t expected[][3] = {{3, 35, 100}, {3, 0, 0}, {3, 60, 100}};
        const int count = scenario == 1 ? 1 : 3;
        if (MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != count) {
            std::fprintf(stderr, "candidate count mismatch in scenario %d\n", scenario);
            return 1;
        }
        for (int i = 0; i < count; ++i) {
            if (MEM_HU(ST_COLLISION_QUERY_CANDIDATE_INDICES + 2 * i, query) != expected[scenario][i]) {
                std::fprintf(stderr, "candidate index mismatch in scenario %d\n", scenario);
                return 1;
            }
        }
    }
    std::puts("3 spatial candidate selection cases passed");
    return 0;
}
