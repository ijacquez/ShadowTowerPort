#include "recomp.h"
#include "st/ST_ResourceVectorFormat.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void do_break(uint32_t) { std::abort(); }

struct Case { int32_t time; uint32_t interval, index, fraction; };

int main() {
    constexpr uint32_t resource = 0x80100000, stack = 0x80110000;
    constexpr uint32_t indexOut = 0x80120000, fractionOut = 0x80120004;
    const Case cases[] = {
        {0, 0x300, 0, 0},
        {9, 0x300, 0, 3686},
        {10, 0x310, 1, 0},
        {29, 0x310, 1, 3891},
        {30, 0x310, 1, 0x1000},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_VECTOR_RESOURCE_SEQUENCE_OFFSETS, resource) = 0x100;
        MEM_W(4, resource + 0x100) = 0x200;
        MEM_B(ST_VECTOR_SEQUENCE_INTERVAL_COUNT, resource + 0x200) = 2;
        MEM_W(ST_VECTOR_SEQUENCE_INTERVAL_OFFSETS, resource + 0x200) = 0x300;
        MEM_W(ST_VECTOR_SEQUENCE_INTERVAL_OFFSETS + 4, resource + 0x200) = 0x310;
        MEM_H(ST_VECTOR_INTERVAL_DURATION, resource + 0x300) = 10;
        MEM_H(ST_VECTOR_INTERVAL_DURATION, resource + 0x310) = 20;
        MEM_W(0x10, stack) = fractionOut;
        recomp_context ctx{};
        ctx.r4 = resource;
        ctx.r5 = 1;
        ctx.r6 = test.time;
        ctx.r7 = indexOut;
        ctx.r29 = stack;
        ST_SelectResourceSequenceInterval(rdram, &ctx);
        if (ctx.r2 != resource + test.interval || MEM_W(0, indexOut) != test.index ||
            MEM_W(0, fractionOut) != test.fraction) {
            std::fprintf(stderr, "resource sequence interval case %zu failed\n", &test - cases);
            return 1;
        }
    }
    std::printf("%zu resource sequence interval cases passed\n", sizeof cases / sizeof cases[0]);
}
