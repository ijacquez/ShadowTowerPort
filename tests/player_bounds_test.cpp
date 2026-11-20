#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

struct Case {
    bool grid;
    int32_t x, y, z;
    int32_t lowerX, lowerZ, widthX, widthZ, limitY;
    uint32_t expected;
};

int main() {
    constexpr uint32_t player = 0x80198F28, stack = 0x80100000;
    const Case cases[] = {
        {true, -2048, 0, 0, 0, 0, 1, 1, 0x8000, 1},
        {true, -2049, 0, 0, 0, 0, 1, 1, 0x8000, 0},
        {true, 2047, 0, 2047, 0, 0, 1, 1, 0x8000, 1},
        {true, 2048, 0, 0, 0, 0, 1, 1, 0x8000, 0},
        {true, 0, -4096, 0, 0, 0, 1, 1, 0, 1},
        {true, 0, -4097, 0, 0, 0, 1, 1, 0, 0},
        {true, 0, 0, 0, 0, 0, 1, 1, 0, 1},
        {true, 0, 1, 0, 0, 0, 1, 1, 0, 0},
        {false, -1536, 0, 0, 0, 0, 3072, 3072, 0x8000, 1},
        {false, -1537, 0, 0, 0, 0, 3072, 3072, 0x8000, 0},
        {false, 1535, 0, 1535, 0, 0, 3072, 3072, 0x8000, 1},
        {false, 1536, 0, 0, 0, 0, 3072, 3072, 0x8000, 0},
        {false, 0, -4096, 0, 0, 0, 3072, 3072, 0, 1},
        {false, 0, -4097, 0, 0, 0, 3072, 3072, 0, 0},
        {false, 0, 1, 0, 0, 0, 3072, 3072, 0, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_PLAYER_POSITION_X, player) = test.x;
        MEM_W(ST_PLAYER_POSITION_Y, player) = test.y;
        MEM_W(ST_PLAYER_POSITION_Z, player) = test.z;
        MEM_W(0x10, stack) = test.limitY;
        recomp_context ctx{};
        ctx.r29 = stack;
        ctx.r4 = test.lowerX;
        ctx.r5 = test.lowerZ;
        ctx.r6 = test.widthX;
        ctx.r7 = test.widthZ;
        if (test.grid) ST_IsPlayerWithinGridBounds(rdram, &ctx);
        else ST_IsPlayerWithinWorldBounds(rdram, &ctx);
        if (ctx.r2 != test.expected) {
            std::fprintf(stderr, "player bounds case %zu failed\n", &test - cases);
            return 1;
        }
    }
    std::printf("%zu player bounds cases passed\n", sizeof cases / sizeof cases[0]);
}
