#include "recomp.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

// Verify the actual RotMatrix/GTE path with all 4096 yaw values and different
// unknown stack halfwords. Only the executable's zero-angle entry is fixed;
// other table entries deliberately contain varied halfword values.
int main() {
    constexpr uint32_t table = 0x80082280;
    constexpr uint32_t angles = 0x80100000;
    constexpr uint32_t matrix = 0x80100020;
    uint32_t value = 1;
    for (int i = 0; i < 4096; ++i) {
        value = value * 1664525u + 1013904223u;
        MEM_W(i * 4, table) = value;
    }
    MEM_W(0, table) = 0x10000000;
    MEM_H(0, angles) = 0;
    MEM_H(4, angles) = 0;

    for (int yaw = 0; yaw < 4096; ++yaw) {
        MEM_H(2, angles) = (uint16_t)-yaw;
        recomp_context rotation{};
        rotation.r4 = angles;
        rotation.r5 = matrix;
        RotMatrix(rdram, &rotation);
        if (MEM_HS(2, matrix) != 0 || MEM_HS(14, matrix) != 0) {
            std::fprintf(stderr, "yaw %d: nonzero horizontal Y coefficient\n", yaw);
            return 1;
        }

        uint32_t expectedX = 0, expectedZ = 0;
        const int16_t stackValues[] = {0, -32768, 32767};
        for (int i = 0; i < 3; ++i) {
            recomp_context gte{};
            for (int j = 0; j < 5; ++j)
                gte.cp2c[j] = MEM_W(j * 4, matrix);
            gte.cp2d[0] = (uint16_t)1234 | ((uint32_t)(uint16_t)stackValues[i] << 16);
            gte.cp2d[1] = (uint16_t)-2345;
            gte_command(&gte, 0x4A486012);
            if (i == 0) {
                expectedX = gte.cp2d[25];
                expectedZ = gte.cp2d[27];
            } else if (gte.cp2d[25] != expectedX || gte.cp2d[27] != expectedZ) {
                std::fprintf(stderr, "yaw %d: stack Y changed horizontal output\n", yaw);
                return 1;
            }
        }
    }
    MEM_H(2, angles) = 0;
    recomp_context rotation{};
    rotation.r4 = angles;
    rotation.r5 = matrix;
    RotMatrix(rdram, &rotation);
    int16_t screenY[2];
    for (int i = 0; i < 2; ++i) {
        recomp_context gte{};
        for (int j = 0; j < 5; ++j)
            gte.cp2c[j] = MEM_W(j * 4, matrix);
        gte.cp2c[25] = 120u << 16;
        gte.cp2c[26] = 300;
        gte.cp2d[0] = (uint32_t)(i * 128) << 16;
        gte.cp2d[1] = 4096;
        gte_command(&gte, 0x4A180001);
        screenY[i] = (int16_t)(gte.cp2d[14] >> 16);
    }
    if (screenY[1] <= screenY[0]) {
        std::fprintf(stderr, "positive camera Y did not increase projected Y: %d, %d\n",
                     screenY[0], screenY[1]);
        return 1;
    }
    std::puts("4096 yaw values preserve horizontal output across stack Y cases");
    return 0;
}
