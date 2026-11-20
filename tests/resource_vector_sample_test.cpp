#include "recomp.h"
#include "st/ST_ResourceVectorFormat.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static constexpr uint32_t resource = 0x80100000;

void do_break(uint32_t) { std::abort(); }
void sub_80033DB8(uint8_t*, recomp_context* ctx) { MEM_W(0, ctx->r5) = resource; }
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

int main() {
    constexpr uint32_t output = 0x80110000, stack = 0x80120000;
    for (int interpolated = 0; interpolated < 2; ++interpolated) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_VECTOR_RESOURCE_MODEL_TABLE_OFFSET, resource) = 0x100;
        MEM_W(ST_VECTOR_MODEL_DATA_OFFSET, resource + 0x100) = 0x100;
        const uint32_t base = resource + 0x20C + ST_VECTOR_ENTRY_SIZE;
        MEM_H(0, base) = 100;
        MEM_H(2, base) = 200;
        MEM_H(4, base) = 300;
        MEM_H(6, base) = 0x1234;
        MEM_H(6, output) = 0x7777;
        if (interpolated) {
            MEM_H(ST_VECTOR_RESOURCE_SEQUENCE_COUNT, resource) = 1;
            MEM_W(ST_VECTOR_RESOURCE_SEQUENCE_OFFSETS, resource) = 0x300;
            MEM_W(4, resource + 0x300) = 0x380;
            MEM_B(ST_VECTOR_SEQUENCE_INTERVAL_COUNT, resource + 0x380) = 1;
            MEM_W(ST_VECTOR_SEQUENCE_INTERVAL_OFFSETS, resource + 0x380) = 0x400;
            MEM_H(ST_VECTOR_INTERVAL_DURATION, resource + 0x400) = 10;
            MEM_B(ST_VECTOR_INTERVAL_FIRST_FRAME, resource + 0x400) = 1;
            MEM_B(ST_VECTOR_INTERVAL_SECOND_FRAME, resource + 0x400) = 2;
            MEM_W(ST_VECTOR_RESOURCE_FRAME_OFFSETS, resource) = 0x500;
            MEM_W(4, resource + 0x500) = 0x600;
            MEM_W(8, resource + 0x500) = 0x680;
            const uint32_t first = resource + 0x600 + ST_VECTOR_ENTRY_SIZE;
            const uint32_t second = resource + 0x680 + ST_VECTOR_ENTRY_SIZE;
            MEM_H(0, first) = 10; MEM_H(2, first) = 20; MEM_H(4, first) = -10;
            MEM_H(0, second) = 30; MEM_H(2, second) = 0; MEM_H(4, second) = -20;
        }
        MEM_W(0x10, stack) = output;
        recomp_context ctx{};
        ctx.r4 = 0x80;
        ctx.r5 = 1;
        ctx.r6 = 5;
        ctx.r7 = 1;
        ctx.r29 = stack;
        ST_SampleResourceVector(rdram, &ctx);
        if (ctx.r2 != 0 || MEM_HS(0, output) != (interpolated ? 120 : 100) ||
            MEM_HS(2, output) != (interpolated ? 210 : 200) ||
            MEM_HS(4, output) != (interpolated ? 285 : 300) ||
            MEM_HU(6, output) != (interpolated ? 0x7777 : 0x1234)) {
            std::fprintf(stderr, "resource vector sampling case %d failed\n", interpolated);
            return 1;
        }
    }
    std::puts("2 resource vector sampling cases passed");
}
