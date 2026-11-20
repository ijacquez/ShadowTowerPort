#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static unsigned randomCall;
void rand_recomp(uint8_t*, recomp_context* ctx) {
    ctx->r2 = randomCall++ == 0 ? 0x5000 : 0x3000;
}

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t gp = 0x80080000;
    struct Case {
        int32_t timer;
        int16_t x, z;
        uint16_t transientX, transientZ;
        int16_t expectedX, expectedZ;
        uint16_t expectedTransientX, expectedTransientZ;
        int32_t expectedTimer;
        unsigned calls;
    };
    const Case cases[] = {
        {-1, 5, -5, 7, 8, 5, -5, 7, 8, -1, 0},
        {3, 0, 0, 0, 0, 4, -4, 4, 0xFFFC, 2, 2},
        {2, 5, -5, 0, 0, 2, -3, 0, 0xFFFD, 1, 0},
        {1, 0, 0, 0x100, 0xF00, 0, 0, 0x40, 0xFC0, 0, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_PLAYER_RANDOM_VIEW_TIMER, player) = (uint32_t)test.timer;
        MEM_H(ST_PLAYER_RANDOM_VIEW_X_GP_OFFSET, gp) = (uint16_t)test.x;
        MEM_H(ST_PLAYER_RANDOM_VIEW_Z_GP_OFFSET, gp) = (uint16_t)test.z;
        MEM_H(ST_PLAYER_VIEW_TRANSIENT_ROTATION_X, player) = test.transientX;
        MEM_H(ST_PLAYER_VIEW_TRANSIENT_ROTATION_Z, player) = test.transientZ;
        recomp_context ctx{};
        ctx.r28 = gp;
        ctx.r29 = 0x801EF000;
        randomCall = 0;
        ST_UpdateRandomPlayerViewRotation(rdram, &ctx);
        if (MEM_HS(ST_PLAYER_RANDOM_VIEW_X_GP_OFFSET, gp) != test.expectedX ||
            MEM_HS(ST_PLAYER_RANDOM_VIEW_Z_GP_OFFSET, gp) != test.expectedZ ||
            MEM_HU(ST_PLAYER_VIEW_TRANSIENT_ROTATION_X, player) != test.expectedTransientX ||
            MEM_HU(ST_PLAYER_VIEW_TRANSIENT_ROTATION_Z, player) != test.expectedTransientZ ||
            S32(MEM_W(ST_PLAYER_RANDOM_VIEW_TIMER, player)) != test.expectedTimer ||
            randomCall != test.calls) {
            std::fprintf(stderr, "random view rotation failed for timer %d\n", test.timer);
            return 1;
        }
    }
    std::printf("%zu random view rotation cases passed\n", sizeof cases / sizeof cases[0]);
}
