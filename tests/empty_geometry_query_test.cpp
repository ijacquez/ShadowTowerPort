#include "recomp.h"
#include "st/ST_CollisionQuery.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];
static int unexpectedCalls;
void ST_SelectSpatialCandidates(uint8_t*, recomp_context*) {}
void ST_DecodeCandidateOrigins(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_TransformCandidateVertices(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_TestThreeVertexCollisionRecords(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ST_TestFourVertexCollisionRecords(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void SquareRoot0(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void ratan2(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void rcos(uint8_t*, recomp_context*) { ++unexpectedCalls; }
void rsin(uint8_t*, recomp_context*) { ++unexpectedCalls; }

int main() {
    constexpr uint32_t query = ST_COLLISION_QUERY_BASE_ADDRESS;
    MEM_H(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) = 12;
    MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, query) = 0x38;
    MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query) = 1;
    MEM_W(ST_COLLISION_QUERY_CURRENT_Y_RESULT, query) = 2;
    recomp_context ctx{};
    ctx.r4 = query;
    ctx.r29 = 0x801EF000;
    ST_QueryLevelGeometry(rdram, &ctx);
    if (ctx.r2 != 0 || MEM_HU(ST_COLLISION_QUERY_CANDIDATE_COUNT, query) != 0 ||
        MEM_W(ST_COLLISION_QUERY_PROPOSED_Y_RESULT, query) != 0x80000 ||
        MEM_W(ST_COLLISION_QUERY_CURRENT_Y_RESULT, query) != 0x80000 ||
        MEM_W(ST_COLLISION_QUERY_RECORD_FLAGS, query) != 0x38 || unexpectedCalls) {
        std::fputs("empty geometry query did not preserve record flags\n", stderr);
        return 1;
    }
    std::puts("empty geometry query preserves record flags");
}
