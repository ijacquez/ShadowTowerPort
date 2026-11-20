#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_LevelRecord.h"
#include <cmath>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void SquareRoot0(uint8_t*, recomp_context* ctx) { ctx->r2 = (uint32_t)std::sqrt(ctx->r4); }
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 0x1000; }

int main() {
    constexpr uint32_t query = 0x80100000;
    constexpr uint32_t records = ST_LEVEL_RECORD_TABLE_ADDRESS;
    for (int scenario = 0; scenario < 5; ++scenario) {
        std::memset(rdram, 0, sizeof(rdram));
        for (int i = 0; i < 240; ++i)
            MEM_H(i * 88 + ST_LEVEL_RECORD_INDEXED_DATA_INDEX, records) = 0xFFFF;
        MEM_H(ST_LEVEL_RECORD_INDEXED_DATA_INDEX, records) = 0;
        MEM_H(ST_LEVEL_RECORD_VERTICAL_EXTENT, records) = 100;
        MEM_H(ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT,
              ST_LEVEL_RECORD_INDEXED_DATA_TABLE_ADDRESS) = 100;
        MEM_H(ST_COLLISION_QUERY_HORIZONTAL_PARAMETER, query) = 100;
        MEM_H(ST_COLLISION_QUERY_VERTICAL_PARAMETER, query) = 100;
        if (scenario == 1) MEM_W(0, ST_LEVEL_RECORD_QUERY_EXCLUDED_ADDRESS) = records;
        if (scenario == 2) MEM_H(ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT,
                                  ST_LEVEL_RECORD_INDEXED_DATA_TABLE_ADDRESS) = 0;
        if (scenario == 3 || scenario == 4) {
            MEM_H(88 + ST_LEVEL_RECORD_INDEXED_DATA_INDEX, records) = 0;
            MEM_W(ST_LEVEL_RECORD_POSITION_Y, records) = scenario == 3 ? -101 : 101;
        }
        recomp_context ctx{};
        ctx.r29 = 0x80180000;
        ctx.r4 = query;
        ST_QueryIndexedCollisionRecords(rdram, &ctx);
        const int expected = scenario == 0 ? 0 : -1;
        if (S32(ctx.r2) != expected) {
            std::fprintf(stderr, "indexed collision scenario %d returned %d\n",
                         scenario, S32(ctx.r2));
            return 1;
        }
    }
    std::puts("5 indexed collision record cases passed");
    return 0;
}
