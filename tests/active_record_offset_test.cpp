#include "recomp.h"
#include "st/ST_ActiveCollisionRecord.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static bool sampleFails;
static bool argumentsMatch;
static int16_t rotation[3];

void ST_SampleResourceVector(uint8_t*, recomp_context* ctx) {
    argumentsMatch = ctx->r4 == 0x83 && ctx->r5 == 7 &&
        static_cast<int32_t>(ctx->r6) == -20 && ctx->r7 == 0;
    const uint32_t output = MEM_W(0x10, ctx->r29);
    MEM_H(0, output) = 100;
    MEM_H(2, output) = -101;
    MEM_H(4, output) = 300;
    ctx->r2 = sampleFails ? 1 : 0;
}

void ST_ApplyCompositeRotationToVector(uint8_t*, recomp_context* ctx) {
    for (int i = 0; i < 3; ++i) {
        rotation[i] = MEM_HS(i * 2, ctx->r4);
        MEM_W(i * 4, ctx->r6) = MEM_HS(i * 2, ctx->r5);
    }
}

int main() {
    constexpr uint32_t record = 0x801BA138, output = 0x80110000;
    for (int failure = 0; failure < 2; ++failure) {
        std::memset(rdram, 0, sizeof rdram);
        sampleFails = failure != 0;
        MEM_B(ST_ACTIVE_COLLISION_RECORD_RESOURCE_SLOT, record) = 3;
        MEM_B(ST_ACTIVE_COLLISION_RECORD_RESOURCE_VECTOR_SELECTOR, record) = 7;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_UPDATE_ACCUMULATOR, record) = -20;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT, record) = 5;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT, record) = 5;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_SCALE_X, record) = 0x800;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_SCALE_Y, record) = 0x800;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_SCALE_Z, record) = 0x1000;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_ROTATION_X, record) = 3;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_ROTATION_Y, record) = 0xFC00;
        MEM_H(ST_ACTIVE_COLLISION_RECORD_ROTATION_Z, record) = 5;
        recomp_context ctx{};
        ctx.r4 = record;
        ctx.r6 = output;
        ctx.r29 = 0x80120000;
        ST_ComputeActiveCollisionRecordOffset(rdram, &ctx);
        const int32_t expectedX = failure ? 0 : 50;
        const int32_t expectedY = failure ? -3 : -51;
        const int32_t expectedZ = failure ? -5 : 300;
        if (!argumentsMatch || ctx.r2 != 1 || rotation[0] != 3 ||
            rotation[1] != 0x400 || rotation[2] != 5 ||
            S32(MEM_W(0, output)) != expectedX || S32(MEM_W(4, output)) != expectedY ||
            S32(MEM_W(8, output)) != expectedZ) {
            std::fprintf(stderr, "active record offset case %d failed\n", failure);
            return 1;
        }
    }
    std::puts("2 active record offset cases passed");
}
