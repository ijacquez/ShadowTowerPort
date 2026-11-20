#include "recomp.h"
#include "st/ST_PlayerState.h"
#include "st/ST_VectorNormalization.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int32_t normalizedX, normalizedZ;
static int directionCalls;
extern short SQRT[];

static void initializeSquareRootTable() {
    for (int i = 0; i < 192; ++i)
        MEM_H(i * 2, ST_SQUARE_ROOT_TABLE_ADDRESS) = (uint16_t)SQRT[i];
}

void do_break(uint32_t) { std::abort(); }
void RotMatrix(uint8_t*, recomp_context* ctx) {
    normalizedX = S32(MEM_W(0x18, ctx->r29));
    normalizedZ = S32(MEM_W(0x20, ctx->r29));
    std::memset(GET_PTR(ctx->r5), 0, 32);
    MEM_H(0, ctx->r5) = 4096;
    MEM_H(8, ctx->r5) = 4096;
    MEM_H(16, ctx->r5) = 4096;
    ctx->r2 = ctx->r5;
}
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ST_ApplyDirectionalMovementWithCollision(uint8_t*, recomp_context*) { ++directionCalls; }

int main() {
    constexpr uint32_t player = 0x80198F28;
    struct Case { int32_t x, z, expectedX, expectedZ; int calls; };
    const Case cases[] = {
        {0, 0, 0, 0, 0},
        {96, 0, 96, 0, 1},
        {0, -96, 0, -96, 1},
        {96, 96, 68, 68, 1},
        {-96, -96, -68, -68, 1},
    };
    for (const auto& test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        initializeSquareRootTable();
        MEM_W(ST_PLAYER_LOCAL_MOVE_X, player) = test.x;
        MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player) = test.z;
        recomp_context ctx{};
        ctx.r29 = 0x801EF000;
        directionCalls = 0;
        ST_PlayerUpdateMovement(rdram, &ctx);
        if (normalizedX != test.expectedX || normalizedZ != test.expectedZ ||
            directionCalls != test.calls) {
            std::fprintf(stderr, "player normalization case failed\n");
            return 1;
        }
    }
    std::memset(rdram, 0, sizeof rdram);
    initializeSquareRootTable();
    MEM_H(ST_PLAYER_MOVE_X, player) = 8;
    MEM_H(ST_PLAYER_MOVE_Y, player) = -8;
    MEM_H(ST_PLAYER_MOVE_Z, player) = 8;
    MEM_H(0, 0x801C74B4) = -9;
    MEM_H(0, 0x801C74B8) = 11;
    MEM_H(0, 0x801C0298) = -5;
    MEM_H(0, 0x801C029C) = -5;
    recomp_context ctx{};
    ctx.r29 = 0x801EF000;
    directionCalls = 0;
    ST_PlayerUpdateMovement(rdram, &ctx);
    if (MEM_HS(ST_PLAYER_MOVE_X, player) != 2 ||
        MEM_HS(ST_PLAYER_MOVE_Y, player) != -6 ||
        MEM_HS(ST_PLAYER_MOVE_Z, player) != 6 || directionCalls != 0) {
        std::fprintf(stderr, "player movement contribution case failed\n");
        return 1;
    }
    std::memset(rdram, 0, sizeof rdram);
    initializeSquareRootTable();
    MEM_H(ST_PLAYER_MOVE_X, player) = 3;
    MEM_H(ST_PLAYER_MOVE_Y, player) = -3;
    MEM_H(ST_PLAYER_MOVE_Z, player) = 4;
    ctx = {};
    ctx.r29 = 0x801EF000;
    ST_PlayerUpdateMovement(rdram, &ctx);
    if (MEM_HS(ST_PLAYER_MOVE_X, player) != 0 ||
        MEM_HS(ST_PLAYER_MOVE_Y, player) != 0 ||
        MEM_HS(ST_PLAYER_MOVE_Z, player) != 0) {
        std::fprintf(stderr, "player movement damping threshold case failed\n");
        return 1;
    }
    std::printf("%zu player normalization cases passed\n", sizeof cases / sizeof cases[0]);
}
