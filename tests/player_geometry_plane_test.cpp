#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_LevelGeometry.h"
#include "st/ST_LevelPlacement.h"
#include "st/ST_SpatialCandidate.h"
#include "st/ST_PlayerState.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

static int unexpectedCalls;
void memset_recomp(uint8_t* rdram, recomp_context* ctx) {
    std::memset(GET_PTR(ctx->r4), ctx->r5, ctx->r6);
    ctx->r2 = ctx->r4;
}
void ST_QueryActiveCollisionRecords(uint8_t*, recomp_context* ctx) { ctx->r2 = 0xFFFFFFFF; }
void ST_QueryActiveCollisionRecordsWithOffsets(uint8_t*, recomp_context* ctx) { ctx->r2 = 0xFFFFFFFF; }
void ST_QueryIndexedCollisionRecords(uint8_t*, recomp_context* ctx) { ctx->r2 = 0xFFFFFFFF; }
void ST_QueryPlayerCollision(uint8_t*, recomp_context* ctx) { ctx->r2 = 0xFFFFFFFF; }
void ST_EnterPlayerZeroHpState(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_HandleDownwardCollisionFeedback(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_RequestMovementSound(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_SetCollisionReference(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void VectorNormalS(uint8_t*, recomp_context*) { ++unexpectedCalls; }

int main() {
    // One synthetic placement uses the initial stage's horizontal plane.
    constexpr uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    constexpr uint32_t base = 0x800F0000;
    constexpr uint32_t model = base + 12;
    constexpr uint32_t placement = ST_LEVEL_PLACEMENT_TABLE_ADDRESS + 3 * 12;
    constexpr uint32_t vertices = base + 12 + 0x100;
    constexpr uint32_t normal = base + 12 + 0x140;
    constexpr uint32_t record = base + 12 + 0x148;
    constexpr uint32_t matrices = ST_LEVEL_QUARTER_TURN_MATRICES_ADDRESS;
    MEM_W(0, ST_SPATIAL_FILTER_DESCRIPTOR_POINTER_ADDRESS) = base;
    MEM_W(ST_GEOMETRY_MODEL_VERTEX_OFFSET, model) = 0x100;
    MEM_W(ST_GEOMETRY_MODEL_VERTEX_COUNT, model) = 4;
    MEM_W(ST_GEOMETRY_MODEL_VECTOR_OFFSET, model) = 0x140;
    MEM_W(ST_GEOMETRY_MODEL_RECORD_OFFSET, model) = 0x148;
    MEM_H(ST_GEOMETRY_MODEL_FOUR_VERTEX_COUNT, model) = 1;
    const int16_t vertex[4][3] = {
        {-1024, -1024, -2048}, {-1024, -1024, 2048},
        {1024, -1024, -2048}, {1024, -1024, 2048}
    };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j) MEM_H(i * 8 + j * 2, vertices) = vertex[i][j];
    MEM_H(2, normal) = -4096;
    MEM_H(ST_GEOMETRY_RECORD_FLAGS, record) = 1;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_0_OFFSET, record) = 0;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_1_OFFSET, record) = 8;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_2_OFFSET, record) = 16;
    MEM_H(ST_GEOMETRY_RECORD_VERTEX_3_OFFSET, record) = 24;
    MEM_B(ST_LEVEL_PLACEMENT_ORIENTATION, placement) = 1;
    MEM_B(ST_LEVEL_PLACEMENT_X_CELL, placement) = 64;
    MEM_B(ST_LEVEL_PLACEMENT_Y_CELL, placement) = 66;
    MEM_B(ST_LEVEL_PLACEMENT_Z_CELL, placement) = 63;
    MEM_B(ST_LEVEL_PLACEMENT_Y_SUBCELL, placement) = 128;
    MEM_H(4, matrices + 32) = -4096;
    MEM_H(8, matrices + 32) = 4096;
    MEM_H(12, matrices + 32) = 4096;
    constexpr uint32_t x = ST_SPATIAL_CANDIDATE_X_BITSETS_ADDRESS;
    MEM_W(64 * 64, x) = 1u << 3;
    MEM_W(ST_SPATIAL_CANDIDATE_Y_BITSETS_OFFSET + 65 * 64, x) = 1u << 3;
    MEM_W(ST_SPATIAL_CANDIDATE_Z_BITSETS_OFFSET + 63 * 64, x) = 1u << 3;
    MEM_W(ST_COLLISION_QUERY_POSITION_X, query) = 0x40000;
    MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = 0x419C0;
    MEM_W(ST_COLLISION_QUERY_POSITION_Z, query) = 0x3F000;
    MEM_W(ST_COLLISION_QUERY_INPUT_FLAGS, query) = ST_COLLISION_QUERY_PLAYER_MOVEMENT_CHECKS;
    MEM_H(ST_COLLISION_QUERY_HORIZONTAL_PARAMETER, query) = 0x100;
    MEM_H(ST_COLLISION_QUERY_VERTICAL_PARAMETER, query) = 0x640;
    MEM_H(ST_COLLISION_QUERY_DIRECTION, query) = ST_COLLISION_QUERY_DIRECTION_FROM_DISPLACEMENT;
    static uint8_t initial[sizeof rdram];
    std::memcpy(initial, rdram, sizeof rdram);
    struct Case { int dy, proposed; };
    const Case cases[] = {
        {0, 576}, {256, 320}, {576, 0}, {-256, 832}, {832, -256}, {833, 576}
    };
    for (const auto& test : cases) {
        std::memcpy(rdram, initial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_H(ST_COLLISION_QUERY_DISPLACEMENT_Y, query) = test.dy;
        recomp_context ctx{};
        ctx.r4 = query;
        ctx.r29 = 0x801EF000;
        ST_QueryLevelGeometry(rdram, &ctx);
        if (ctx.r2 != ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD ||
            MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 1 ||
            S32(MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query)) != test.proposed ||
            S32(MEM_W(ST_COLLISION_QUERY_CURRENT_Y_RESULT, query)) != 576 ||
            MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, query) != 0) {
            std::fprintf(stderr, "plane geometry query failed for displacement %d\n", test.dy);
            return 1;
        }
    }
    // Exercise successive movement calls with no matching object records.
    std::memcpy(rdram, initial, sizeof rdram);
    std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
    constexpr uint32_t player = 0x80198F28;
    MEM_W(ST_PLAYER_POSITION_X, player) = 0x40000;
    MEM_W(ST_PLAYER_POSITION_Y, player) = 0x419C0;
    MEM_W(ST_PLAYER_POSITION_Z, player) = 0x3F000;
    MEM_W(ST_PLAYER_MOVE_LIMIT, player) = 192;
    MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = 1;
    recomp_context ctx{};
    ctx.r29 = 0x801EF000;
    ctx.r28 = 0x80080000;
    struct MovementCase { int y, vertical, proposed, flags; };
    const MovementCase movementCases[] = {
        {0x419C0, -16, 576, 1}, {0x419B0, -64, 592, 1},
        {0x41970, -112, 656, 1}, {0x41900, -160, 768, 1},
        {0x41860, -208, 928, 1}, {0x41790, -256, 1136, 1},
        {0x41690, -208, 1392, 1}, {0x415C0, 0, 1600, 0},
        {0x415C0, 0, 1600, 0}
    };
    for (const auto& test : movementCases) {
        ST_PlayerMoveWithCollision(rdram, &ctx);
        if (MEM_W(ST_PLAYER_POSITION_Y, player) != test.y ||
            MEM_HS(ST_PLAYER_VERTICAL_MOVE, player) != test.vertical ||
            S32(MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query)) != test.proposed ||
            MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, query) != test.flags ||
            MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 1 || unexpectedCalls) {
            std::fprintf(stderr, "plane movement failed at Y 0x%x\n", test.y);
            return 1;
        }
    }
    // Replace the plane with the stage's positive-X vertical record.
    std::memcpy(rdram, initial, sizeof rdram);
    const int16_t wall[4][3] = {
        {1024, -1024, -2048}, {1024, -1024, 2048},
        {1024, -512, -2048}, {1024, -512, 2048}
    };
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j) MEM_H(i * 8 + j * 2, vertices) = wall[i][j];
    MEM_H(0, normal) = 4096;
    MEM_H(2, normal) = 0;
    MEM_B(1, record) = 4;
    MEM_H(ST_GEOMETRY_RECORD_FLAGS, record) = 0xA;
    // The axis-aligned query uses the sine table's quarter-turn entry.
    MEM_H(0x800, 0x80081710) = 4096;
    static uint8_t wallInitial[sizeof rdram];
    std::memcpy(wallInitial, rdram, sizeof rdram);
    struct WallCase { int y, flag, resultOffset, normalOffset, fallbackOffset, fallbackDistance; };
    const WallCase wallCases[] = {
        {0x41480, ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD,
            ST_COLLISION_QUERY_SECOND_NORMAL_RESULT, ST_COLLISION_QUERY_SECOND_NORMAL_Z,
            ST_COLLISION_GEOMETRY_SCRATCH_SECOND_NORMAL_FALLBACK_DISTANCE, 256},
        {0x416C0, ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD,
            ST_COLLISION_QUERY_FIRST_NORMAL_RESULT, ST_COLLISION_QUERY_FIRST_NORMAL_Z,
            ST_COLLISION_GEOMETRY_SCRATCH_FIRST_NORMAL_FALLBACK_DISTANCE, 256},
        {0x41C00, ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD,
            ST_COLLISION_QUERY_FIRST_NORMAL_RESULT, ST_COLLISION_QUERY_FIRST_NORMAL_Z,
            ST_COLLISION_GEOMETRY_SCRATCH_FIRST_NORMAL_FALLBACK_DISTANCE,
            ST_COLLISION_GEOMETRY_NO_RESULT}
    };
    for (const auto& test : wallCases) {
        std::memcpy(rdram, wallInitial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = test.y;
        MEM_W(ST_COLLISION_QUERY_POSITION_Z, query) = 0x3F500;
        MEM_H(ST_COLLISION_QUERY_DISPLACEMENT_Z, query) = -48;
        recomp_context queryCtx{};
        queryCtx.r4 = query;
        queryCtx.r29 = 0x801EF000;
        ST_QueryLevelGeometry(rdram, &queryCtx);
        if (queryCtx.r2 != test.flag || MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 1 ||
            S32(MEM_W(test.resultOffset, query)) != 256 ||
            MEM_HS(test.normalOffset, query) != 4096 ||
            MEM_W(test.fallbackOffset, 0x1F800000) != test.fallbackDistance) {
            std::fprintf(stderr, "wall geometry query failed at Y 0x%x\n", test.y);
            return 1;
        }

        std::memcpy(rdram, wallInitial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_W(ST_PLAYER_POSITION_X, player) = 0x40000;
        MEM_W(ST_PLAYER_POSITION_Y, player) = test.y;
        MEM_W(ST_PLAYER_POSITION_Z, player) = 0x3F500;
        MEM_H(ST_PLAYER_MOVE_Z, player) = -48;
        MEM_B(ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED, player) = 1;
        recomp_context moveCtx{};
        moveCtx.r29 = 0x801EF000;
        moveCtx.r28 = 0x80080000;
        ST_PlayerMoveWithCollision(rdram, &moveCtx);
        if (MEM_W(ST_PLAYER_POSITION_X, player) != 0x40000 ||
            MEM_W(ST_PLAYER_POSITION_Y, player) != test.y ||
            MEM_W(ST_PLAYER_POSITION_Z, player) != 0x3F501 ||
            MEM_HS(ST_PLAYER_COLLISION_DISPLACEMENT_Z, player) != 1 ||
            MEM_HS(ST_PLAYER_VERTICAL_MOVE, player) != 16 ||
            MEM_W(ST_COLLISION_QUERY_RESULT_FLAGS, query) != 0 || unexpectedCalls) {
            std::fprintf(stderr, "wall movement failed at Y 0x%x\n", test.y);
            return 1;
        }
    }
    struct DirectionalCase { int y, z, displacement; };
    const DirectionalCase directionalCases[] = {
        {0x416C0, 0x3F4D0, -48},
        {0x41C00, 0x3F501, 1}
    };
    for (const auto& test : directionalCases) {
        std::memcpy(rdram, wallInitial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_W(ST_PLAYER_POSITION_X, player) = 0x40000;
        MEM_W(ST_PLAYER_POSITION_Y, player) = test.y;
        MEM_W(ST_PLAYER_POSITION_Z, player) = 0x3F500;
        recomp_context directionalCtx{};
        directionalCtx.r29 = 0x801EF000;
        directionalCtx.r28 = 0x80080000;
        directionalCtx.r4 = 0xC00;
        directionalCtx.r5 = 48;
        ST_ApplyDirectionalMovementWithCollision(rdram, &directionalCtx);
        if (MEM_W(ST_PLAYER_POSITION_X, player) != 0x40000 ||
            MEM_W(ST_PLAYER_POSITION_Y, player) != test.y ||
            MEM_W(ST_PLAYER_POSITION_Z, player) != test.z ||
            MEM_HS(ST_PLAYER_COLLISION_DISPLACEMENT_Z, player) != test.displacement ||
            unexpectedCalls) {
            std::fprintf(stderr, "directional wall movement failed at Y 0x%x\n", test.y);
            return 1;
        }
    }
    // Use the first three wall vertices as a triangle with the same winding.
    for (const auto& test : wallCases) {
        std::memcpy(rdram, wallInitial, sizeof rdram);
        std::memset(g_psxScratchpad, 0, sizeof g_psxScratchpad);
        MEM_H(ST_GEOMETRY_MODEL_THREE_VERTEX_COUNT, model) = 1;
        MEM_H(ST_GEOMETRY_MODEL_FOUR_VERTEX_COUNT, model) = 0;
        MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = test.y;
        MEM_W(ST_COLLISION_QUERY_POSITION_Z, query) = 0x3F500;
        MEM_H(ST_COLLISION_QUERY_DISPLACEMENT_Z, query) = -48;
        recomp_context triangleCtx{};
        triangleCtx.r4 = query;
        triangleCtx.r29 = 0x801EF000;
        ST_QueryLevelGeometry(rdram, &triangleCtx);
        if (triangleCtx.r2 != test.flag || MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 1 ||
            S32(MEM_W(test.resultOffset, query)) != 256 ||
            MEM_HS(test.normalOffset, query) != 4096 ||
            MEM_W(test.fallbackOffset, 0x1F800000) != test.fallbackDistance) {
            std::fprintf(stderr, "triangle geometry query failed at Y 0x%x\n", test.y);
            return 1;
        }
    }
    std::puts("plane and wall geometry queries and movement updates passed");
}
