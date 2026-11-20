#include "recomp.h"
#include "st/ST_PlayerState.h"
#include "st/ST_CollisionQuery.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int helperCalls;
static int32_t sineResult;
static int soundCalls;
static int32_t soundMagnitude;
static uint32_t collisionResult;
static uint32_t secondCollisionResult;
static int collisionQueries;
void memset_recomp(uint8_t*, recomp_context* ctx) { std::memset(GET_PTR(ctx->r4), ctx->r5, ctx->r6); ctx->r2 = ctx->r4; }
void SquareRoot0(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rsin(uint8_t*, recomp_context* ctx) { ctx->r2 = sineResult; }
void ST_RunCollisionQuery(uint8_t*, recomp_context* ctx) {
    ctx->r2 = collisionQueries++ == 0 ? collisionResult : secondCollisionResult;
    MEM_W(ST_COLLISION_QUERY_FIRST_NORMAL_RESULT, ST_COLLISION_QUERY_BASE_ADDRESS) = 256;
    MEM_H(ST_COLLISION_QUERY_FIRST_NORMAL_X, ST_COLLISION_QUERY_BASE_ADDRESS) = 0x1000;
}
void ST_CheckHighDownwardMovement(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void ST_HandleDownwardCollisionFeedback(uint8_t*, recomp_context*) { ++helperCalls; }
void ST_RequestMovementSound(uint8_t*, recomp_context* ctx) {
    ++soundCalls;
    soundMagnitude = S32(ctx->r4);
    ctx->r2 = 0;
}
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
    struct ViewPitchCase {
        int16_t initialVertical, previousPitch, expectedPitch;
        int32_t difference;
    };
    const ViewPitchCase pitchCases[] = {
        {128, -3, -1, 1025},
        {129, -3, 0, 1025},
        {131, -3, 1, 1025},
        {-80, 3, 1, -4096},
        {-81, 3, 0, -4096},
        {-83, 3, -1, -4096},
    };
    for (const auto& test : pitchCases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = 1;
        MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = 1;
        MEM_H(ST_PLAYER_VERTICAL_MOVE, player) = (uint16_t)test.initialVertical;
        MEM_W(0x2C, query) = (uint32_t)(1600 + test.difference);
        MEM_W(0x74, query) = 0x10;
        recomp_context ctx{};
        ctx.r28 = 0x80080000;
        ctx.r29 = 0x801EF000;
        MEM_H(ST_PLAYER_VERTICAL_VIEW_PITCH_GP_OFFSET, ctx.r28) = (uint16_t)test.previousPitch;
        ST_PlayerMoveWithCollision(rdram, &ctx);
        if (MEM_HS(ST_PLAYER_VERTICAL_VIEW_PITCH_GP_OFFSET, ctx.r28) != test.expectedPitch ||
            MEM_HS(ST_PLAYER_VIEW_ADDITIONAL_ROTATION_X, player) != test.expectedPitch) {
            std::fprintf(stderr, "vertical view pitch case failed for %d\n", test.initialVertical);
            return 1;
        }
    }
    struct CollisionResultCase { uint32_t result; int expectedX; };
    const CollisionResultCase collisionCases[] = {
        {0, 100},
        {ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD, 100},
        {ST_COLLISION_RESULT_ACTIVE_RECORD_HIT, 0},
        {ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD | ST_COLLISION_RESULT_ACTIVE_RECORD_HIT, 100},
    };
    for (const auto& test : collisionCases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(ST_PLAYER_MOVE_X, player) = 100;
        MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query) = ST_PLAYER_COLLISION_Y_OFFSET;
        recomp_context ctx{};
        ctx.r28 = 0x80080000;
        ctx.r29 = 0x801EF000;
        collisionResult = test.result;
        collisionQueries = 0;
        ST_PlayerMoveWithCollision(rdram, &ctx);
        if (S32(MEM_W(ST_PLAYER_POSITION_X, player)) != test.expectedX || collisionQueries != 1) {
            std::fprintf(stderr, "collision result %x produced position %d after %d queries\n",
                         test.result, S32(MEM_W(ST_PLAYER_POSITION_X, player)), collisionQueries);
            return 1;
        }
    }
    std::memset(rdram, 0, sizeof rdram);
    MEM_H(ST_PLAYER_MOVE_X, player) = 100;
    MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query) = ST_PLAYER_COLLISION_Y_OFFSET;
    recomp_context correction{};
    correction.r28 = 0x80080000;
    correction.r29 = 0x801EF000;
    collisionResult = ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD | ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD;
    secondCollisionResult = 0;
    collisionQueries = 0;
    ST_PlayerMoveWithCollision(rdram, &correction);
    if (collisionQueries != 2 || S32(MEM_W(ST_PLAYER_POSITION_X, player)) != 101) {
        std::fputs("combined Y and normal flags did not apply both candidates\n", stderr);
        return 1;
    }
    collisionResult = 0;
    struct OscillationCase {
        uint8_t disabled, enabled;
        uint16_t phase, magnitude;
        int32_t sine;
        uint16_t expectedPhase, expectedOffset;
    };
    const OscillationCase oscillationCases[] = {
        {0, 1, 4090, 10, 4096, 4, 96},
        {0, 1, 10, 10, -4096, 20, 96},
        {0, 1, 10, 10, -1, 20, 1},
        {0, 0, 10, 10, 4096, 10, 0},
        {1, 1, 10, 10, 4096, 10, 77},
    };
    for (const auto& test : oscillationCases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = test.magnitude;
        MEM_B(0x21E, player) = 0;
        MEM_B(0x239, player) = test.disabled;
        MEM_B(0, 0x801CD122) = test.enabled;
        MEM_H(ST_PLAYER_VIEW_OSCILLATION_PHASE, player) = test.phase;
        MEM_H(ST_PLAYER_VIEW_OSCILLATION_Y, player) = 77;
        MEM_W(0x2C, query) = 1700;
        recomp_context ctx{};
        ctx.r28 = 0x80080000;
        ctx.r29 = 0x801EF000;
        sineResult = test.sine;
        ST_PlayerMoveWithCollision(rdram, &ctx);
        if (MEM_HU(ST_PLAYER_VIEW_OSCILLATION_PHASE, player) != test.expectedPhase ||
            MEM_HU(ST_PLAYER_VIEW_OSCILLATION_Y, player) != test.expectedOffset) {
            std::fprintf(stderr, "player view oscillation case failed\n");
            return 1;
        }
    }
    struct SoundCase {
        int16_t magnitude;
        int32_t countdown;
        uint8_t disabled;
        int32_t expectedCountdown;
        int expectedCalls;
    };
    const SoundCase soundCases[] = {
        {10, 30, 0, 0, 0},
        {10, 29, 0, 0x800, 1},
        {25, 37, 0, 0, 0},
        {25, 36, 0, 0x800, 1},
        {50, 49, 0, 0x800, 1},
        {50, 49, 1, 49, 0},
    };
    for (const auto& test : soundCases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(ST_PLAYER_MOVE_MAGNITUDE, player) = test.magnitude;
        MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 80;
        MEM_W(ST_PLAYER_MOVEMENT_SOUND_COUNTDOWN, player) = test.countdown;
        MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = test.disabled;
        MEM_W(0x2C, query) = 1601;
        recomp_context ctx{};
        ctx.r28 = 0x80080000;
        ctx.r29 = 0x801EF000;
        soundCalls = 0;
        soundMagnitude = -1;
        ST_PlayerMoveWithCollision(rdram, &ctx);
        if (S32(MEM_W(ST_PLAYER_MOVEMENT_SOUND_COUNTDOWN, player)) != test.expectedCountdown ||
            soundCalls != test.expectedCalls ||
            (soundCalls && soundMagnitude != test.magnitude)) {
            std::fprintf(stderr, "movement sound countdown case failed for magnitude %d\n", test.magnitude);
            return 1;
        }
    }
    std::printf("%zu vertical cases passed\n", sizeof cases / sizeof cases[0]);
    std::printf("%zu vertical view pitch cases passed\n", sizeof pitchCases / sizeof pitchCases[0]);
    std::printf("%zu collision result application cases passed\n", 1 + sizeof collisionCases / sizeof collisionCases[0]);
    std::printf("%zu view oscillation cases passed\n", sizeof oscillationCases / sizeof oscillationCases[0]);
    std::printf("%zu movement sound countdown cases passed\n", sizeof soundCases / sizeof soundCases[0]);
    return 0;
}
