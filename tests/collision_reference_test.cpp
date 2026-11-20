#include "recomp.h"
#include "st/ST_ActiveCollisionRecord.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerState.h"
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
    std::puts("3 collision reference selection cases passed");
}
