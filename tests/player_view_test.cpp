#include "recomp.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t position = 0x80100000;
    constexpr uint32_t rotation = 0x80100020;
    struct Case { uint32_t y; int16_t a, b; uint32_t result; };
    const Case cases[] = {
        {10000, 0, 0, 8400},
        {10000, -100, 250, 8550},
        {0, -32768, -1, (uint32_t)-34369},
        {0x7fffffff, 32767, 32767, 0x8000f9bd},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(0x248, player) = 0x12345678;
        MEM_W(0x24C, player) = test.y;
        MEM_W(0x250, player) = 0x87654321;
        MEM_H(0x2CE, player) = test.a;
        MEM_H(0x2D2, player) = test.b;
        MEM_H(0x268, player) = 100;
        MEM_H(0x26A, player) = 200;
        MEM_H(0x26C, player) = 300;
        recomp_context ctx{};
        ctx.r4 = position;
        ctx.r5 = rotation;
        ST_BuildPlayerView(rdram, &ctx);
        if (MEM_W(0, position) != 0x12345678 ||
            MEM_W(4, position) != test.result ||
            MEM_W(8, position) != 0x87654321 ||
            MEM_HU(0, rotation) != 100 || MEM_HU(2, rotation) != 200 ||
            MEM_HU(4, rotation) != 300 || ctx.r2 != 300 || ctx.r3 != player ||
            ctx.r6 != (uint32_t)test.b || ctx.r7 != (uint32_t)test.a) {
            std::fprintf(stderr, "player view case failed\n");
            return 1;
        }
    }
    MEM_W(0x248, player) = 0x7fffffff;
    MEM_W(0x24C, player) = 10;
    MEM_W(0x250, player) = 0;
    recomp_context translation{};
    translation.r4 = 1;
    translation.r5 = (uint32_t)-20;
    translation.r6 = (uint32_t)-1;
    ST_TranslatePlayerPosition(rdram, &translation);
    if (MEM_W(0x248, player) != 0x80000000 ||
        MEM_W(0x24C, player) != (uint32_t)-10 ||
        MEM_W(0x250, player) != 0xffffffff || translation.r3 != player ||
        translation.r2 != 0xffffffff || translation.r4 != (uint32_t)-10) {
        std::fprintf(stderr, "player position translation failed\n");
        return 1;
    }
    std::printf("%zu player view cases and position translation passed\n",
                sizeof cases / sizeof cases[0]);
}
