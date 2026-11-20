#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

void memset_recomp(uint8_t*, recomp_context* ctx) {
    for (uint32_t i = 0; i < ctx->r6; ++i) MEM_B(i, ctx->r4) = ctx->r5;
}
void rand_recomp(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ST_AdjustPlayerHp(uint8_t*, recomp_context*) {}
void ST_RequestSoundEffect(uint8_t*, recomp_context*) {}
void ST_RecalculatePlayerDerivedAttributes(uint8_t*, recomp_context*) {}
void ST_ApplyPlayerDamageAndViewResponse(uint8_t*, recomp_context*) {}
void do_break(uint32_t) { std::abort(); }

int main() {
    constexpr uint32_t player = 0x80198F28;
    for (int attribute = 50; attribute <= 112; ++attribute) {
        for (int percent = 0; percent <= 255; ++percent) {
            std::memset(GET_PTR(player), 0, 0x300);
            MEM_H(ST_PLAYER_EFFECT_CAP_ATTRIBUTE, player) = attribute;
            MEM_B(ST_PLAYER_EFFECT_CAP_PERCENT, player) = percent;
            MEM_H(ST_PLAYER_MOVE_REDUCTION_TARGET, player) = 32767;
            recomp_context ctx{};
            ctx.r29 = 0x80180000;
            ctx.r6 = 8;
            ST_ApplyPlayerDamageAndEffects(rdram, &ctx);
            const int base = 50 + 50 * (112 - attribute) / 59;
            const int suppression = (base * 75 / 100) * percent / 100;
            const int reduction = (base * 300 / 100) * percent / 100;
            if (MEM_HS(ST_PLAYER_MOVE_SUPPRESSION_TARGET, player) != std::min(75, suppression) ||
                MEM_HS(ST_PLAYER_MOVE_REDUCTION_TARGET, player) != reduction) {
                std::fprintf(stderr, "effect caps mismatch for attribute %d, percent %d\n", attribute, percent);
                return 1;
            }
        }
    }
    std::puts("16128 player effect cap combinations passed");
    return 0;
}
