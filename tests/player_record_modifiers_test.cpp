#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void do_break(uint32_t) { std::abort(); }

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t records = 0x801C7810;
    for (int i = 0; i < 17; ++i) MEM_H(0x38 + i * 2, player) = 0xFFFF;
    recomp_context ctx{};
    ctx.r29 = 0x80180000;
    ST_RecalculatePlayerRecordModifiers(rdram, &ctx);
    if (MEM_BU(ST_PLAYER_EFFECT_CAP_PERCENT, player) != 100) return 1;

    MEM_H(0x38, player) = 0;
    MEM_BU(0x20, records) = 0xA5;
    MEM_BU(0x22, records) = 5;
    MEM_BU(0x23, records) = 50;
    ST_RecalculatePlayerRecordModifiers(rdram, &ctx);
    if (MEM_BU(ST_PLAYER_EFFECT_CAP_PERCENT, player) != 52) return 1;

    MEM_BU(0x23, records) = 200;
    ST_RecalculatePlayerRecordModifiers(rdram, &ctx);
    if (MEM_BU(ST_PLAYER_EFFECT_CAP_PERCENT, player) != 20) return 1;
    std::puts("3 player record modifier cases passed");
    return 0;
}
