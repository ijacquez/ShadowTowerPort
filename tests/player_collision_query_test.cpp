#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerState.h"
#include <cmath>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

void SquareRoot0(uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<uint32_t>(std::sqrt(static_cast<double>(ctx->r4)));
}
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 4096; }

struct Case {
    int32_t playerX, queryY;
    int16_t magnitude;
    uint32_t expected;
};

int main() {
    constexpr uint32_t player = 0x80198F28, query = 0x801B5948;
    const Case cases[] = {
        {511, 0, 0, 511},
        {512, 0, 0, 0xFFFFFFFF},
        {512, 0, 1, 512},
        {0, -1, 0, 0xFFFFFFFF},
        {0, 0, 0, 0},
        {0, 2240, 0, 0},
        {0, 2241, 0, 0xFFFFFFFF},
        {0x2801, 0, 0, 0xFFFFFFFF},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_PLAYER_POSITION_X, player) = test.playerX;
        MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = test.queryY;
        MEM_H(ST_COLLISION_QUERY_HORIZONTAL_PARAMETER, query) = 0x100;
        MEM_H(ST_COLLISION_QUERY_VERTICAL_PARAMETER, query) = 0x280;
        MEM_H(ST_COLLISION_QUERY_HORIZONTAL_MAGNITUDE, query) = test.magnitude;
        recomp_context ctx{};
        ctx.r4 = query;
        ctx.r29 = 0x80100000;
        ST_QueryPlayerCollision(rdram, &ctx);
        if (ctx.r2 != test.expected) {
            std::fprintf(stderr, "player collision query case %zu failed\n", &test - cases);
            return 1;
        }
    }
    std::printf("%zu player collision query cases passed\n", sizeof cases / sizeof cases[0]);
}
