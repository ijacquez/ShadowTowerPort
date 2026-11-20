#include "recomp.h"
#include "st/ST_VectorNormalization.h"
#include <cstdio>

uint8_t rdram[2 * 1024 * 1024];

int main() {
    // For this input the game's signed index is -320 halfwords.
    MEM_H(-640, ST_SQUARE_ROOT_TABLE_ADDRESS) = 0x1000;
    recomp_context ctx{};
    ctx.r4 = 0xC0000000;
    SquareRoot0(rdram, &ctx);
    if (ctx.r2 != 0x4000 || ctx.cp2d[31] != 2) {
        std::fprintf(stderr, "wrapped square root returned %u, leading count %u\n",
                     ctx.r2, ctx.cp2d[31]);
        return 1;
    }
    ctx.r4 = 0xFFFFFFFF;
    SquareRoot0(rdram, &ctx);
    if (ctx.r2 != 0 || ctx.cp2d[31] != 32) {
        std::fputs("all-ones square root did not follow the game's zero branch\n", stderr);
        return 1;
    }
    MEM_H(-384, ST_SQUARE_ROOT_TABLE_ADDRESS) = 0x2000;
    ctx.r4 = 0xFFFFFFFE;
    SquareRoot0(rdram, &ctx);
    if (ctx.r2 != 2 || ctx.cp2d[31] != 31) {
        std::fputs("negative left-shift square root case failed\n", stderr);
        return 1;
    }
    ctx.r4 = 0x80000000;
    SquareRoot0(rdram, &ctx);
    if (ctx.r2 != 0x10000 || ctx.cp2d[31] != 1) {
        std::fputs("minimum signed square root case failed\n", stderr);
        return 1;
    }
    MEM_H(0, ST_SQUARE_ROOT_TABLE_ADDRESS) = 0x1000;
    ctx.r4 = 1;
    SquareRoot0(rdram, &ctx);
    if (ctx.r2 != 1 || ctx.cp2d[30] != 1 || ctx.cp2d[31] != 31) {
        std::fputs("positive square root did not update GTE leading-bit registers\n", stderr);
        return 1;
    }
    std::puts("5 square root input and GTE state cases passed");
}
