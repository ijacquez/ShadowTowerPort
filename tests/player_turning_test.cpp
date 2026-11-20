#include "recomp.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

struct TurnCase {
    const char* name;
    uint16_t held;
    int16_t initialRate;
    uint16_t initialYaw;
    int32_t limit;
    int16_t expectedRate;
    uint16_t expectedYaw;
};

int main() {
    struct AngleCase { uint32_t first, second; bool greater; };
    const AngleCase angleCases[] = {
        {0, 0, false}, {1, 0, true}, {0, 1, false},
        {0x7FF, 0x800, true}, {0x800, 0x7FF, false},
        {0xFFF, 0, false}, {0x1001, 0, true},
    };
    for (const auto& test : angleCases) {
        recomp_context ctx{};
        ctx.r4 = test.first;
        ctx.r5 = test.second;
        ST_IsShiftedAngleGreater(rdram, &ctx);
        if (ctx.r2 != test.greater || ctx.r5 != ((test.second + 0x800) & 0xFFF)) {
            std::fprintf(stderr, "shifted angle comparison failed\n");
            return 1;
        }
    }
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t controls = 0x801CD180;
    const TurnCase cases[] = {
        {"positive from rest", 0x80, 0, 0, 40, 10, 10},
        {"negative from rest", 0x20, 0, 0, 40, -10, 4086},
        {"positive limit", 0x80, 35, 100, 40, 40, 140},
        {"negative limit", 0x20, -35, 100, 40, -40, 60},
        {"release positive", 0, 7, 100, 40, 0, 100},
        {"release negative", 0, -7, 100, 40, 0, 100},
        {"reverse direction", 0x20, 20, 100, 40, 10, 110},
        {"both bindings", 0xA0, 0, 100, 40, 10, 110},
        {"yaw wraps", 0x80, 0, 4090, 40, 10, 4},
        {"zero limit", 0x80, 0, 100, 0, 0, 100},
    };
    for (const auto &test : cases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(2, controls) = test.held;
        MEM_H(0x26, controls) = 0x80;
        MEM_H(0x28, controls) = 0x20;
        MEM_H(ST_PLAYER_TURN_RATE, player) = (uint16_t)test.initialRate;
        MEM_H(ST_PLAYER_YAW, player) = test.initialYaw;
        MEM_W(ST_PLAYER_TURN_LIMIT, player) = (uint32_t)test.limit;
        recomp_context ctx{};
        ctx.r29 = 0x801EF000;
        ST_PlayerUpdateTurning(rdram, &ctx);
        int16_t rate = (int16_t)MEM_HU(ST_PLAYER_TURN_RATE, player);
        uint16_t yaw = MEM_HU(ST_PLAYER_YAW, player);
        if (rate != test.expectedRate || yaw != test.expectedYaw) {
            std::fprintf(stderr, "%s: rate %d (expected %d), yaw %u (expected %u)\n",
                         test.name, rate, test.expectedRate, yaw, test.expectedYaw);
            return 1;
        }
    }
    struct PitchCase {
        const char* name;
        uint16_t held;
        int16_t initialRate;
        uint16_t initialPitch;
        int16_t expectedRate;
        uint16_t expectedPitch;
    };
    const PitchCase pitchCases[] = {
        {"negative pitch", 0x100, 0, 0, -3, 4093},
        {"positive pitch", 0x200, 0, 0, 3, 3},
        {"paired pitch from zero", 0x300, 0, 0, 32, 32},
        {"paired pitch near target", 0x300, 0, 100, 7, 107},
        {"paired pitch at target", 0x300, 0, 128, 0, 128},
        {"release positive pitch", 0, 2, 0, 0, 0},
        {"release negative pitch", 0, -2, 0, 0, 0},
        {"negative pitch rate limit", 0x100, -31, 0, -32, 4064},
        {"positive pitch rate limit", 0x200, 31, 0, 32, 32},
        {"positive pitch angle limit", 0x200, 0, 699, 3, 700},
        {"negative pitch angle limit", 0x100, 0, 3397, -3, 3396},
        {"paired pitch wraps", 0x300, 0, 4090, 8, 2},
    };
    for (const auto &test : pitchCases) {
        std::memset(rdram, 0, sizeof rdram);
        MEM_H(2, controls) = test.held;
        MEM_H(0x2E, controls) = 0x100;
        MEM_H(0x30, controls) = 0x200;
        MEM_H(ST_PLAYER_PITCH_RATE, player) = (uint16_t)test.initialRate;
        MEM_H(ST_PLAYER_PITCH, player) = test.initialPitch;
        recomp_context ctx{};
        ctx.r29 = 0x801EF000;
        ST_PlayerUpdateTurning(rdram, &ctx);
        int16_t rate = (int16_t)MEM_HU(ST_PLAYER_PITCH_RATE, player);
        uint16_t pitch = MEM_HU(ST_PLAYER_PITCH, player);
        if (rate != test.expectedRate || pitch != test.expectedPitch) {
            std::fprintf(stderr, "%s: rate %d (expected %d), pitch %u (expected %u)\n",
                         test.name, rate, test.expectedRate, pitch, test.expectedPitch);
            return 1;
        }
    }
    std::printf("%zu turning and %zu pitch cases passed\n",
                sizeof cases / sizeof cases[0], sizeof pitchCases / sizeof pitchCases[0]);
    return 0;
}
