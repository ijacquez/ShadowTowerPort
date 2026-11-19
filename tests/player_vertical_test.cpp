#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int helperCalls;
void memset_recomp(uint8_t*, recomp_context* ctx) { std::memset(GET_PTR(ctx->r4), ctx->r5, ctx->r6); ctx->r2 = ctx->r4; }
void SquareRoot0(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rsin(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ST_RunCollisionQuery(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void sub_8003BDB0(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void sub_80057B48(uint8_t*, recomp_context*) { ++helperCalls; }
void sub_80057BAC(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ST_UpdatePlayerCollisionYFlags(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }

struct VerticalCase {
    const char* name;
    int32_t difference;
    int16_t initial;
    uint32_t flags;
    int16_t expected;
    uint32_t expectedLimit;
    int expectedHelperCalls;
};

int main() {
    constexpr uint32_t player = 0x80198F28, query = 0x801B5948;
    const VerticalCase cases[] = {
        {"negative difference, positive movement", -100, 100, 0, 25, 192, 1},
        {"negative difference, stationary", -100, 0, 0, -16, 0, 0},
        {"negative difference, continue", -100, -16, 0, -64, 0, 0},
        {"negative difference, clamp", -100, -64, 0, -100, 0, 0},
        {"negative threshold inside", -31, 0, 0, 31, 192, 0},
        {"negative threshold outside", -32, 0, 0, -16, 0, 0},
        {"negative limit threshold", -1025, 0, 0, -16, 192, 0},
        {"positive difference small", 1, 0, 0, 1, 192, 0},
        {"positive threshold inside", 31, 0, 0, 31, 192, 0},
        {"positive threshold outside", 32, 0, 0, 16, 0, 0},
        {"positive limit threshold", 1024, 0, 0, 16, 0, 0},
        {"above positive limit threshold", 1025, 0, 0, 16, 192, 0},
        {"negative movement halves toward zero", 100, -9, 0, -4, 192, 0},
        {"positive movement accelerates", 100, 10, 0, 58, 0, 0},
        {"query flag blocks acceleration", 100, 10, 0x10, 10, 0, 0},
        {"zero difference", 0, 5, 0, 0, 192, 0},
    };
    for (const auto &test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        // Force the path that uses the existing query result without running
        // geometry; this isolates the vertical update from collision stubs.
        MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = 1;
        MEM_B(0x21E, player) = 0;
        MEM_B(0x239, player) = 1;
        MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 192;
        MEM_H(ST_PLAYER_VERTICAL_MOVE, player) = (uint16_t)test.initial;
        MEM_W(0x2C, query) = (uint32_t)(1600 + test.difference);
        MEM_W(0x74, query) = test.flags;
        recomp_context ctx{};
        ctx.r28 = 0x80080000;
        ctx.r29 = 0x801EF000;
        helperCalls = 0;
        ST_PlayerMoveWithCollision(rdram, &ctx);
        const int16_t actual = (int16_t)MEM_HU(ST_PLAYER_VERTICAL_MOVE, player);
        const uint32_t limit = MEM_W(ST_PLAYER_MOVE_LIMIT, player);
        if (actual != test.expected || limit != test.expectedLimit || helperCalls != test.expectedHelperCalls ||
            (test.difference == 0 && MEM_BU(0x239, player) != 0)) {
            std::fprintf(stderr, "%s: value=%d limit=%u helper calls=%d\n", test.name, actual, limit, helperCalls);
            return 1;
        }
    }
    std::printf("%zu vertical cases passed\n", sizeof cases / sizeof cases[0]);
    return 0;
}
