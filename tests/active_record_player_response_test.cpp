#include "recomp.h"
#include "st/ST_ActiveCollisionRecord.h"
#include "st/ST_CollisionQuery.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int queryCount;
static uint32_t queryResult;

// This resolver uses the paired operations only for aligned words.
uint32_t do_lwl(uint8_t*, uint32_t, uint32_t offset, uint32_t base) {
    if (((base + offset) & 3) != 3) std::abort();
    return MEM_W(0, (base + offset) & ~3u);
}
uint32_t do_lwr(uint8_t*, uint32_t, uint32_t offset, uint32_t base) {
    if (((base + offset) & 3) != 0) std::abort();
    return MEM_W(offset, base);
}
void do_swl(uint8_t*, uint32_t offset, uint32_t base, uint32_t value) {
    if (((base + offset) & 3) != 3) std::abort();
    MEM_W(0, (base + offset) & ~3u) = value;
}
void do_swr(uint8_t*, uint32_t offset, uint32_t base, uint32_t value) {
    if (((base + offset) & 3) != 0) std::abort();
    MEM_W(offset, base) = value;
}

void SquareRoot0(uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<uint32_t>(std::sqrt(static_cast<double>(ctx->r4)));
}
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 4096; }
void rsin(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void VectorNormalS(uint8_t*, recomp_context*) {}
void ST_RunCollisionQuery(uint8_t* memory, recomp_context* ctx) {
    ++queryCount;
    MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, ST_COLLISION_QUERY_BASE_ADDRESS) = queryResult;
    MEM_W(ST_COLLISION_QUERY_FIRST_NORMAL_RESULT, ST_COLLISION_QUERY_BASE_ADDRESS) = 0x100;
    MEM_H(ST_COLLISION_QUERY_FIRST_NORMAL_X, ST_COLLISION_QUERY_BASE_ADDRESS) = 0x1000;
    MEM_W(ST_COLLISION_QUERY_SECOND_NORMAL_RESULT, ST_COLLISION_QUERY_BASE_ADDRESS) = 0x100;
    MEM_H(ST_COLLISION_QUERY_SECOND_NORMAL_X, ST_COLLISION_QUERY_BASE_ADDRESS) = 0x1000;
    ctx->r2 = queryResult;
}
void ST_SetCollisionReference(uint8_t* memory, recomp_context*) {
    MEM_W(ST_COLLISION_QUERY_REFERENCE_X, ST_COLLISION_QUERY_BASE_ADDRESS) = 0;
    MEM_W(ST_COLLISION_QUERY_REFERENCE_Z, ST_COLLISION_QUERY_BASE_ADDRESS) = 0;
}

int main() {
    constexpr uint32_t record = 0x801BA138;
    struct Case { uint32_t result; int16_t x; uint8_t hit; };
    const Case cases[] = {
        {ST_COLLISION_RESULT_PLAYER_HIT, 0, 1},
        {ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD, 1, 0},
        {ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD, 1, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        queryCount = 0;
        queryResult = test.result;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_DISPLACEMENT_X, record) = 100;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT, record) = 0x100;
        MEM_B(0x0E, record) = 1;
        recomp_context ctx{};
        ctx.r4 = record;
        ctx.r29 = 0x80100000;
        ST_ResolveActiveCollisionRecordDisplacement(rdram, &ctx);
        if (queryCount != 2 || MEM_BU(ST_ACTIVE_COLLISION_RECORD_HIT_PLAYER_FLAG, record) != test.hit ||
            MEM_HS(ST_ACTIVE_COLLISION_RECORD_DISPLACEMENT_X, record) != test.x ||
            MEM_HS(ST_ACTIVE_COLLISION_RECORD_DISPLACEMENT_Z, record) != 0) {
            std::fprintf(stderr, "repeated collision response case %zu failed\n", &test - cases);
            return 1;
        }
    }
    std::puts("3 repeated collision response cases passed");
}
