#include "recomp.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void do_break(uint32_t code) { std::fprintf(stderr, "unexpected break %u\n", code); }

struct Case {
    int16_t current, target;
    bool hasTarget, writeBack;
    int16_t expectedCurrent, expectedTarget, expectedResult;
};

int main() {
    constexpr uint32_t current = 0x80100000, target = 0x80100010, stack = 0x80100100;
    const Case cases[] = {
        {0, 0, true, true, 0, 0, -1},
        {0, 20, true, true, 1, 20, 204},
        {19, 20, true, true, 20, 20, 4096},
        {20, 20, true, true, 19, 0, 3891},
        {-2, 0, true, true, -1, 0, -204},
        {10, 20, true, false, 10, 20, 2252},
        {1, 0, false, true, 0, 0, 0},
        {-32768, -32768, true, true, 32767, 0, 4096},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(0, current) = test.current;
        MEM_H(0, target) = test.target;
        MEM_W(0x10, stack) = test.writeBack;
        recomp_context ctx{};
        ctx.r4 = current;
        ctx.r5 = test.hasTarget ? target : 0;
        ctx.r6 = 20;
        ctx.r7 = 4096;
        ctx.r29 = stack;
        ST_UpdateCounterAndTarget(rdram, &ctx);
        if (MEM_HS(0, current) != test.expectedCurrent ||
            MEM_HS(0, target) != test.expectedTarget ||
            (int32_t)ctx.r2 != test.expectedResult) {
            std::fprintf(stderr, "counter case failed\n");
            return 1;
        }
    }
    std::printf("%zu counter cases passed\n", sizeof cases / sizeof cases[0]);
}
