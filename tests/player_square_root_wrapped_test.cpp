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
    std::puts("2 wrapped square root cases passed");
}
