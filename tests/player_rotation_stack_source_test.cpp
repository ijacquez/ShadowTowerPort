#include "recomp.h"
#include "st/ST_PlayerAttributes.h"
#include "st/ST_PlayerControls.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
void sub_80042620(uint8_t*, recomp_context*) {}
void sub_8003F670(uint8_t*, recomp_context*) {}
void sub_8003F910(uint8_t*, recomp_context*) {}
void sub_8003F4EC(uint8_t*, recomp_context*) {}

int main() {
    constexpr uint32_t player = 0x80198F28;
    constexpr uint32_t controls = 0x801CD180;
    constexpr uint32_t stack = 0x80180000;
    const uint32_t slots[] = {
        0x80198F60, 0x80198F64, 0x80198F66, 0x80198F70,
        0x80198F72, 0x80198F74, 0x80198F7A, 0x80198F80, 0, 0
    };
    for (unsigned action = 2; action <= 3; ++action) {
        std::memset(rdram, 0, sizeof(rdram));
        for (int i = 0; i < 18; ++i)
            MEM_H(ST_PLAYER_RECORD_SLOTS + i * 2, player) = 0xFFFF;
        for (int i = 0; i < 10; ++i)
            MEM_W(i * 4, ST_LOW_RECORD_VALUE_SLOT_POINTERS) = slots[i];
        MEM_W(ST_PLAYER_ACTION_STATE_PAIR, player) = 0xFFFFFFFF;
        MEM_B(ST_PLAYER_FIRST_ACTION_REPEAT, player) = 0xFF;
        MEM_B(ST_PLAYER_SECOND_ACTION_REPEAT, player) = 0xFF;
        MEM_H(action == 2 ? ST_PLAYER_ACTION_FIRST_RECORD_SLOT :
                            ST_PLAYER_ACTION_SECOND_RECORD_SLOT, player) = 0;
        MEM_B(ST_CONTROL_ACTION_SELECTION, controls) = action;
        MEM_H(-0x54, stack) = 0x1234;
        recomp_context ctx{};
        ctx.r29 = stack;
        ST_ProcessPlayerActionControls(rdram, &ctx);
        ST_PlayerUpdateTurning(rdram, &ctx);
        if (MEM_HU(-0x54, stack) != 0x8F64 || ctx.r29 != stack) {
            std::fprintf(stderr, "rotation stack source mismatch for action %u\n", action);
            return 1;
        }
    }
    std::puts("2 action paths preserve the copied rotation input halfword");
    return 0;
}
