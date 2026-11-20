#include "recomp.h"
#include <cmath>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

void SquareRoot0(uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<uint32_t>(std::sqrt(static_cast<double>(ctx->r4)));
}

struct Case {
    int32_t candidateX, candidateY, queryY;
    uint32_t expected;
};

int main() {
    constexpr uint32_t candidate = 0x80100000, stack = 0x80110000;
    const Case cases[] = {
        {7, 100, 120, 0},
        {-1, 100, 120, 8},
        {8, 100, 120, 8},
        {9, 100, 120, 0xFFFFFFFF},
        {0, 100, 121, 0xFFFFFFFF},
        {0, 100, 90, 0},
        {0, 100, 89, 0xFFFFFFFF},
        {0, 100, 0xFFFF, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(0, candidate) = test.candidateX;
        MEM_W(4, candidate) = test.candidateY;
        MEM_W(0x10, stack) = 8;
        MEM_W(0x14, stack) = 10;
        MEM_W(0x18, stack) = 20;
        recomp_context ctx{};
        ctx.r4 = candidate;
        ctx.r6 = test.queryY;
        ctx.r29 = stack;
        ST_QueryHorizontalDistanceWithVerticalBounds(rdram, &ctx);
        if (ctx.r2 != test.expected) {
            std::fprintf(stderr, "collision distance case %zu failed\n", &test - cases);
            return 1;
        }
    }
    std::printf("%zu collision distance cases passed\n", sizeof cases / sizeof cases[0]);
}
