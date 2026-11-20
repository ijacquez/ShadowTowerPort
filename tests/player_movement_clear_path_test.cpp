#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_PlayerControls.h"
#include "st/ST_PlayerState.h"
#include "st/ST_VectorNormalization.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
extern short SQRT[];
static int queryCount;

void ST_RunCollisionQuery(uint8_t*, recomp_context* ctx) {
    ++queryCount;
    MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, ST_COLLISION_QUERY_BASE_ADDRESS) = 0;
    ctx->r2 = 0;
}
void ST_SetCollisionReference(uint8_t*, recomp_context*) {}
void VectorNormalS(uint8_t*, recomp_context*) {}

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t controls = 0x801CD180;
    constexpr uint32_t sineTable = 0x80081710;
    constexpr uint32_t rotationTable = 0x80082280;
    constexpr uint32_t atanTable = 0x80086280;

    // Values used by the cardinal directions, from the supplied executable.
    for (int i = 0; i < 192; ++i)
        MEM_H(i * 2, ST_SQUARE_ROOT_TABLE_ADDRESS) = (uint16_t)SQRT[i];
    MEM_H(0, sineTable) = 0;
    MEM_H(0x800, sineTable) = 0x1000;
    MEM_W(0, rotationTable) = 0x10000000;
    MEM_W(0x1000, rotationTable) = 0x00001000;
    MEM_W(0x2000, rotationTable) = 0xF0000000;
    MEM_W(0x3000, rotationTable) = 0x0000F000;
    MEM_H(0, atanTable) = 0;

    recomp_context ctx{};
    ctx.r29 = 0x801EF000;
    ST_InitDefaultControlBindings(rdram, &ctx);
    MEM_H(ST_CONTROL_HELD, controls) = MEM_HU(ST_CONTROL_MOVE_Z_POSITIVE, controls);
    MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 0xC0;
    const int expectedStep[] = {48, 96, 144, 192, 192};
    int position = 0;
    for (int step : expectedStep) {
        position += step;
        ST_PlayerUpdateMovement(rdram, &ctx);
        if (S32(MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player)) != step ||
            S32(MEM_W(ST_PLAYER_POSITION_Z, player)) != position ||
            MEM_W(ST_PLAYER_POSITION_X, player) != 0) {
            std::fprintf(stderr, "clear movement mismatch at step %d\n", step);
            return 1;
        }
    }
    if (queryCount != 5) {
        std::fprintf(stderr, "expected five collision queries, got %d\n", queryCount);
        return 1;
    }
    MEM_H(ST_CONTROL_HELD, controls) = 0;
    const int releaseSteps[] = {168, 144, 120, 96, 72, 48, 24, 0};
    for (int step : releaseSteps) {
        position += step;
        ST_PlayerUpdateMovement(rdram, &ctx);
        if (S32(MEM_W(ST_PLAYER_LOCAL_MOVE_Z, player)) != step ||
            S32(MEM_W(ST_PLAYER_POSITION_Z, player)) != position ||
            MEM_W(ST_PLAYER_POSITION_X, player) != 0) {
            std::fprintf(stderr, "clear movement release mismatch at step %d\n", step);
            return 1;
        }
    }
    if (queryCount != 12) {
        std::fprintf(stderr, "expected twelve collision queries, got %d\n", queryCount);
        return 1;
    }
    struct CardinalCase { uint16_t yaw; int32_t x, z; };
    const CardinalCase cardinalCases[] = {
        {0, 0, 48}, {0x400, -48, 0}, {0x800, 0, -48}, {0xC00, 48, 0}
    };
    for (const auto& test : cardinalCases) {
        std::memset(GET_PTR(player), 0, 0x300);
        MEM_H(ST_PLAYER_YAW, player) = test.yaw;
        MEM_H(ST_CONTROL_HELD, controls) = MEM_HU(ST_CONTROL_MOVE_Z_POSITIVE, controls);
        MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 0xC0;
        ST_PlayerUpdateMovement(rdram, &ctx);
        if (S32(MEM_W(ST_PLAYER_POSITION_X, player)) != test.x ||
            S32(MEM_W(ST_PLAYER_POSITION_Z, player)) != test.z) {
            std::fprintf(stderr, "clear movement failed for yaw %x\n", test.yaw);
            return 1;
        }
    }
    std::puts("5 held, 8 release, and 4 cardinal clear-path movement updates passed");
}
