#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int soundCalls;
void sub_80057C0C(uint8_t*, recomp_context*) { ++soundCalls; }

struct Case {
    int16_t speed;
    int32_t queryY;
    uint8_t state;
    uint16_t hp;
    uint8_t expectedState;
    uint16_t expectedHp;
    uint8_t expectedFlag;
    int expectedSoundCalls;
};

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t query = 0x801B5948;
    const Case cases[] = {
        {0x47F, 0x640, 0, 100, 0, 100, 0, 0},
        {0x480, 0x63F, 0, 100, 0, 100, 0, 0},
        {0x480, 0x640, 0, 100, 4, 0, 1, 1},
        {0x480, 0x640, 4, 100, 4, 100, 1, 0},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        soundCalls = 0;
        MEM_H(ST_PLAYER_VERTICAL_MOVE, player) = test.speed;
        MEM_W(ST_COLLISION_QUERY_CURRENT_Y_RESULT, query) = test.queryY;
        MEM_B(ST_PLAYER_STATE, player) = test.state;
        MEM_H(ST_PLAYER_CURRENT_HP, player) = test.hp;
        recomp_context ctx{};
        ctx.r29 = 0x801EF000;
        ST_CheckHighDownwardMovement(rdram, &ctx);
        if (MEM_BU(ST_PLAYER_STATE, player) != test.expectedState ||
            MEM_HU(ST_PLAYER_CURRENT_HP, player) != test.expectedHp ||
            MEM_BU(ST_PLAYER_HIGH_DOWNWARD_TRIGGERED, player) != test.expectedFlag ||
            soundCalls != test.expectedSoundCalls) {
            std::fprintf(stderr, "high downward check failed at speed %d, query Y %d\n",
                         test.speed, test.queryY);
            return 1;
        }
    }
    std::puts("4 high downward movement cases passed");
}
