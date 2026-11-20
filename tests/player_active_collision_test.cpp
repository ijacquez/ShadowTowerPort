#include "recomp.h"
#include "st/ST_ActiveCollisionRecord.h"
#include "st/ST_CollisionQuery.h"
#include "st/ST_VectorNormalization.h"
#include <cstdio>
#include <cstring>

uint8_t rdram[2 * 1024 * 1024];

extern short SQRT[];
void ratan2(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void rcos(uint8_t*, recomp_context* ctx) { ctx->r2 = 0x1000; }
void sub_8004A1B4(uint8_t*, recomp_context* ctx) { ctx->r2 = 0; }
void sub_8004A148(uint8_t*, recomp_context* ctx) { ctx->r2 = 1; }

constexpr uint32_t query = 0x801B5948;
constexpr uint32_t records = 0x801BA138;
constexpr uint32_t classes = 0x801BD638;
constexpr uint32_t filterMask = 0x801C0239;

static void addRecord(unsigned index, int32_t x, int32_t y, int32_t z) {
    const uint32_t record = records + index * 212;
    MEM_B(ST_ACTIVE_COLLISION_RECORD_STATE, record) = 1;
    MEM_B(ST_ACTIVE_COLLISION_RECORD_CLASS_INDEX, record) = 0;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT, record) = 32;
    MEM_H(ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT, record) = 128;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, record) = x;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, record) = y;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Z, record) = z;
}

static int32_t runQuery() {
    recomp_context ctx{};
    ctx.r4 = query;
    ctx.r5 = 2;
    ctx.r29 = 0x801EF000;
    ST_QueryActiveCollisionRecords(rdram, &ctx);
    return S32(ctx.r2);
}

static void reset() {
    std::memset(rdram, 0, sizeof rdram);
    for (int i = 0; i < 192; ++i)
        MEM_H(i * 2, ST_SQUARE_ROOT_TABLE_ADDRESS) = (uint16_t)SQRT[i];
    MEM_W(ST_COLLISION_QUERY_POSITION_Y, query) = 1600;
    MEM_H(ST_COLLISION_QUERY_HORIZONTAL_PARAMETER, query) = 256;
    MEM_H(ST_COLLISION_QUERY_VERTICAL_PARAMETER, query) = 1600;
    MEM_H(0x16, classes) = 1;
}

int main() {
    reset();
    if (runQuery() != -1) return 1;
    addRecord(0, 0, 0, 0);
    if (runQuery() != 0) return 2;
    addRecord(1, 0, 100, 0);
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, records) = -1;
    if (runQuery() != -1) return 3; // The later valid record is not scanned.
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, records) = 0x2801;
    if (runQuery() != 1) return 4;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, records) = 0;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS, records) = 1;
    MEM_B(0, filterMask) = 1;
    if (runQuery() != 1) return 5;
    MEM_B(0, filterMask) = 0;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, records) = 1728;
    if (runQuery() != 0) return 6;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_Y, records) = 1729;
    if (runQuery() != -1) return 7;
    reset();
    addRecord(0, 800, 0, 0);
    if (runQuery() != 0) return 8;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, records) = 801;
    if (runQuery() != 0) return 9;
    MEM_W(ST_ACTIVE_COLLISION_RECORD_POSITION_X, records) = 802;
    if (runQuery() != -1) return 10;
    std::puts("10 active collision cases passed");
}
