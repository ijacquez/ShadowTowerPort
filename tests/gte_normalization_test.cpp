#include "recomp.h"
#include "st/ST_VectorNormalization.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    struct LeadingCase { uint32_t value, count; };
    const LeadingCase leadingCases[] = {
        {0, 32}, {1, 31}, {0x01000000, 7}, {0x7FFFFFFF, 1}
    };
    for (const auto& test : leadingCases) {
        recomp_context ctx{};
        ctx.r4 = test.value;
        ctx.cp2d[31] = 123;
        gte_mtc2(&ctx, 4, 30);
        if (ctx.cp2d[30] != test.value || gte_mfc2(&ctx, 31) != test.count) {
            std::fprintf(stderr, "leading-zero count failed for %08x\n", test.value);
            return 1;
        }
        ctx.r4 = 77;
        gte_mtc2(&ctx, 4, 31);
        if (gte_mfc2(&ctx, 31) != test.count) {
            std::fputs("leading-zero result was not read-only\n", stderr);
            return 1;
        }
    }

    constexpr uint32_t input = 0x80100000;
    constexpr uint32_t output = 0x80100020;
    const int values[] = {4096, -4096, -2048};
    for (int value : values) {
        std::memset(rdram, 0, sizeof(rdram));
        MEM_H(0, ST_NORMALIZATION_INVERSE_ROOT_TABLE_ADDRESS) = 0x1000;
        MEM_W(0, input) = value;
        recomp_context ctx{};
        ctx.r4 = input;
        ctx.r5 = output;
        ctx.cp2d[31] = 0;
        VectorNormalS(rdram, &ctx);
        const int expected = value > 0 ? 4096 : -4096;
        if (MEM_HS(0, output) != expected || MEM_HS(2, output) != 0 ||
            MEM_HS(4, output) != 0) {
            std::fprintf(stderr, "normalization failed for %d: %d\n", value, MEM_HS(0, output));
            return 1;
        }
    }
    std::puts("4 GTE leading-zero and 3 normalization cases passed");
    return 0;
}
