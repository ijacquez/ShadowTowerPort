#include "recomp.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

struct Case {
    int32_t playerY;
    uint32_t firstY, secondY;
    uint32_t firstBefore, secondBefore;
    uint8_t state;
    int16_t playerIndex;
    uint32_t firstAfter, secondAfter;
};

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t query = 0x801B5948;
    const Case cases[] = {
        {100, 99, 101, 7, 8, 0, 0, 1, 0},
        {100, 100, 100, 7, 8, 0, 0, 0, 0},
        {-100, (uint32_t)-101, (uint32_t)-99, 7, 8, 0, 0, 1, 0},
        {100, 0x80000, 0x80000, 7, 8, 0, 0, 7, 8},
        {100, 0x80000, 99, 7, 8, 10, 0x147, 7, 0},
        {100, 99, 99, 7, 8, 10, 0x146, 1, 1},
        {100, 99, 0x80000, 7, 8, 10, 0x147, 1, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(0x24C, player) = test.playerY;
        MEM_W(0x4C, query) = test.firstY;
        MEM_W(0x58, query) = test.secondY;
        MEM_W(0x2EC, player) = test.firstBefore;
        MEM_W(0x2F0, player) = test.secondBefore;
        MEM_B(0x5924, 0x801B0000) = test.state;
        MEM_H(0x178, player) = test.playerIndex;
        recomp_context ctx{};
        ST_UpdatePlayerCollisionYFlags(rdram, &ctx);
        const uint32_t expectedR2 = test.state == 10 ? 0x147 : 0x801A0000;
        const uint32_t expectedR3 = test.state == 10 ? (uint32_t)test.playerIndex : test.state;
        const uint32_t expectedR4 = test.state == 10 ? player : test.secondY;
        if (MEM_W(0x2EC, player) != test.firstAfter ||
            MEM_W(0x2F0, player) != test.secondAfter ||
            ctx.r2 != expectedR2 || ctx.r3 != expectedR3 || ctx.r4 != expectedR4) {
            std::fprintf(stderr, "collision Y flag case failed\n");
            return 1;
        }
    }
    std::printf("%zu collision Y flag cases passed\n", sizeof cases / sizeof cases[0]);
}
