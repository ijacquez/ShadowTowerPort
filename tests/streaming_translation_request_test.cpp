#include "recomp.h"
#include "st/ST_LevelStreaming.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void sub_80055CF4(uint8_t*, recomp_context*) {}
void AsyncStructForcedLoadSync(uint8_t*, recomp_context*) {}
void AsyncDataLoad(uint8_t*, recomp_context*) {}
void ST_UpdateLevelStreaming(uint8_t*, recomp_context*) {}
void ExitCriticalSection(uint8_t*, recomp_context*) {}

int main() {
    constexpr uint32_t stream = ST_LEVEL_STREAMING_STATUS_ADDRESS;
    constexpr uint32_t stack = 0x80150000;
    for (int scenario = 0; scenario < 4; ++scenario) {
        std::memset(rdram, 0, sizeof(rdram));
        MEM_H(0, stream) = scenario == 2 ? 0 : 1;
        MEM_B(ST_LEVEL_STREAMING_TRANSLATION_APPLIED, stream) = scenario == 1 ? 0 : 1;
        MEM_B(ST_LEVEL_STREAMING_REQUEST_PRIMARY_ID, stream) = scenario == 3 ? 2 : 1;
        MEM_B(ST_LEVEL_STREAMING_TRANSLATION_COARSE_X, stream) = 3;
        MEM_B(ST_LEVEL_STREAMING_TRANSLATION_COARSE_Z, stream) = (uint8_t)-4;
        MEM_B(ST_LEVEL_STREAMING_TRANSLATION_COARSE_Y, stream) = 127;
        MEM_W(0x10, stack) = 2;
        MEM_W(0x14, stack) = ST_LEVEL_STREAMING_NO_TRANSLATION;
        MEM_W(0x18, stack) = 8;
        MEM_W(0x1C, stack) = 9;
        MEM_W(0x20, stack) = 5;
        MEM_W(0x24, stack) = (uint8_t)-6;
        MEM_W(0x28, stack) = 7;
        recomp_context ctx{};
        ctx.r29 = stack;
        ctx.r4 = ctx.r5 = ctx.r6 = ctx.r7 = 2;
        ST_RequestLevelStreaming(rdram, &ctx);
        const int expectedX = scenario == 0 ? -3 : 127;
        const int expectedZ = scenario == 0 ? 4 : 8;
        const int expectedY = scenario == 0 ? -127 : 9;
        if (MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_COARSE_X, stream) != expectedX ||
            MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_COARSE_Z, stream) != expectedZ ||
            MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_COARSE_Y, stream) != expectedY ||
            MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_FINE_X, stream) != 5 ||
            MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_FINE_Y, stream) != -6 ||
            MEM_BS(ST_LEVEL_STREAMING_TRANSLATION_FINE_Z, stream) != 7) {
            std::fprintf(stderr, "streaming translation request scenario %d failed\n", scenario);
            return 1;
        }
    }
    std::puts("4 streaming translation request cases passed");
    return 0;
}
