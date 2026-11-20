#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerState.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static uint32_t results[4];
static int queryCount;

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
void SquareRoot0(uint8_t*, recomp_context* ctx) { ctx->r2 = (uint32_t)std::sqrt(ctx->r4); }
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 0x1000; }
void rsin(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void VectorNormalS(uint8_t*, recomp_context* ctx) {
    MEM_H(0, ctx->r5) = 0x1000;
    MEM_H(2, ctx->r5) = 0;
    MEM_H(4, ctx->r5) = 0;
}
void ST_SetCollisionReference(uint8_t*, recomp_context*) {
    const uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    MEM_W(ST_COLLISION_QUERY_REFERENCE_X, query) = 0;
    MEM_W(ST_COLLISION_QUERY_REFERENCE_Z, query) = 0;
    MEM_W(ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_DISTANCE, query) = 50;
    MEM_H(ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_EXTENT, query) = 25;
}
void ST_RunCollisionQuery(uint8_t*, recomp_context* ctx) {
    const uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    ctx->r2 = results[queryCount++];
    MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, query) = ctx->r2;
    MEM_W(ST_COLLISION_QUERY_FIRST_NORMAL_RESULT, query) = 256;
    MEM_H(ST_COLLISION_QUERY_FIRST_NORMAL_X, query) = 0x1000;
    MEM_H(ST_COLLISION_QUERY_FIRST_NORMAL_Z, query) = 0;
    MEM_W(ST_COLLISION_QUERY_SECOND_NORMAL_RESULT, query) = 256;
    MEM_H(ST_COLLISION_QUERY_SECOND_NORMAL_X, query) = 0x1000;
    MEM_H(ST_COLLISION_QUERY_SECOND_NORMAL_Z, query) = 0;
}

int main() {
    constexpr uint32_t player = 0x80198F28;
    struct Case { uint32_t first, second; int expectedQueries, expectedX, expectedDisplacement; };
    const Case cases[] = {
        {0, 0, 1, 100, 100},
        {ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD, 0, 1, 100, 100},
        {ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD | ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD,
         ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD, 2, 1, 1},
        {ST_COLLISION_RESULT_ACTIVE_RECORD_HIT, ST_COLLISION_RESULT_ACTIVE_RECORD_HIT, 2, 0, 744},
        {ST_COLLISION_RESULT_INDEXED_RECORD_HIT, 0, 2, 332, 332},
        {ST_COLLISION_RESULT_VERTICAL_THRESHOLD, 0, 1, 0, 100},
    };
    for (const Case& test : cases) {
        std::memset(rdram, 0, sizeof(rdram));
        std::memset(results, 0, sizeof(results));
        results[0] = test.first;
        results[1] = test.second;
        queryCount = 0;
        recomp_context ctx{};
        ctx.r29 = 0x80180000;
        ctx.r4 = 0;
        ctx.r5 = 100;
        ST_ApplyDirectionalMovementWithCollision(rdram, &ctx);
        if (queryCount != test.expectedQueries ||
            S32(MEM_W(ST_PLAYER_POSITION_X, player)) != test.expectedX ||
            MEM_HS(ST_PLAYER_COLLISION_DISPLACEMENT_X, player) != test.expectedDisplacement) {
            std::fprintf(stderr, "directional collision case %zu failed: queries=%d x=%d displacement=%d\n",
                         &test - cases, queryCount, S32(MEM_W(ST_PLAYER_POSITION_X, player)),
                         MEM_HS(ST_PLAYER_COLLISION_DISPLACEMENT_X, player));
            return 1;
        }
    }
    std::memset(rdram, 0, sizeof(rdram));
    results[0] = ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD;
    results[1] = ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD;
    results[2] = ST_COLLISION_RESULT_ACTIVE_RECORD_HIT;
    results[3] = 0;
    queryCount = 0;
    recomp_context ctx{};
    ctx.r29 = 0x80180000;
    ctx.r5 = 100;
    ST_ApplyDirectionalMovementWithCollision(rdram, &ctx);
    if (queryCount != 4 || S32(MEM_W(ST_PLAYER_POSITION_X, player)) != 844) {
        std::fputs("zero fourth query did not apply the corrected movement\n", stderr);
        return 1;
    }
    std::puts("7 directional collision response cases passed");
    return 0;
}
