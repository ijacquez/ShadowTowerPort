#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static uint32_t soundId;
static int32_t soundParameter;
void ST_RequestSoundEffect(uint8_t*, recomp_context* ctx) {
    soundId = ctx->r4;
    soundParameter = S32(ctx->r5);
}

int main() {
    constexpr uint32_t query = 0x801B5948;
    struct Case { uint32_t flags; int32_t magnitude; uint32_t id; int32_t parameter; };
    const Case cases[] = {
        {0, 48, 0x11, 76},
        {1, 48, 0x11, 76},
        {2, 48, 0x12, 76},
        {3, 48, 0x11, 76},
        {0x04, 48, 0x13, 76},
        {0x32, -1, 0x13, 63},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, query) = test.flags;
        recomp_context ctx{};
        ctx.r4 = test.magnitude;
        ctx.r29 = 0x801EF000;
        ST_RequestMovementSound(rdram, &ctx);
        if (soundId != test.id || soundParameter != test.parameter) {
            std::fprintf(stderr, "movement sound case failed for flags 0x%X\n", test.flags);
            return 1;
        }
    }
    std::puts("6 movement sound selection cases passed");
}
