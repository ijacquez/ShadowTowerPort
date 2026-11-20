#include "recomp.h"
#include "st/ST_PlayerAttributes.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

void sub_80014D5C(uint8_t*, recomp_context* ctx) {
    for (uint32_t i = 0; i < ctx->r6; ++i) MEM_B(i, ctx->r4) = ctx->r5;
}
void sub_8005975C(uint8_t*, recomp_context*) {}
void sub_80059CEC(uint8_t*, recomp_context*) {}
void sub_8005A36C(uint8_t*, recomp_context*) {}

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t records = 0x801C7810;
    struct Case { uint16_t base; uint8_t record; uint16_t clamped; uint16_t cap; };
    const Case cases[] = {
        {32, 16, 48, 53},
        {1000, 0, 999, 112},
        {0x8000, 0, 0, 50},
        {0xFFF0, 32, 16, 51},
    };
    for (const Case& test : cases) {
        std::memset(rdram, 0, sizeof(rdram));
        for (int i = 0; i < 17; ++i) MEM_H(0x38 + i * 2, player) = 0xFFFF;
        MEM_H(0x38, player) = 0;
        MEM_H(ST_PLAYER_BASE_ATTRIBUTES + 2 * ST_EFFECT_CAP_ATTRIBUTE_INDEX, player) = test.base;
        MEM_B(ST_RECORD_ATTRIBUTE_VALUES + ST_EFFECT_CAP_ATTRIBUTE_INDEX, records) = test.record;
        recomp_context ctx{};
        ctx.r29 = 0x80180000;
        ST_RecalculatePlayerDerivedAttributes(rdram, &ctx);
        if (MEM_HU(ST_ATTRIBUTE_SCRATCH_EFFECT_CAP, 0x1F800000) != test.clamped ||
            MEM_HU(ST_PLAYER_EFFECT_CAP_ATTRIBUTE, player) != test.cap) {
            std::fprintf(stderr, "attribute calculation failed for base %u, record %u\n",
                         test.base, test.record);
            return 1;
        }
    }
    std::memset(rdram, 0, sizeof(rdram));
    for (int i = 0; i < 17; ++i) MEM_H(0x38 + i * 2, player) = 0xFFFF;
    MEM_H(0x38 + 4 * 2, player) = 0;
    MEM_H(0x38 + 16 * 2, player) = 1;
    MEM_B(ST_RECORD_ATTRIBUTE_VALUES + ST_EFFECT_CAP_ATTRIBUTE_INDEX, records) = 200;
    MEM_B(44 + ST_RECORD_ATTRIBUTE_VALUES + ST_EFFECT_CAP_ATTRIBUTE_INDEX, records) = 32;
    recomp_context ctx{};
    ctx.r29 = 0x80180000;
    ST_RecalculatePlayerDerivedAttributes(rdram, &ctx);
    if (MEM_HU(ST_PLAYER_EFFECT_CAP_ATTRIBUTE, player) != 52) {
        std::fputs("attribute record slot selection failed\n", stderr);
        return 1;
    }
    std::puts("5 player attribute scratch cases passed");
    return 0;
}
