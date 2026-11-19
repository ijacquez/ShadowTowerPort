// Include the implementation so the pure accumulator helper can be tested
// without exposing it as a public engine API.
#include "../scr/st/ST_PlayerUpdateMovement.c"
#include <cstdio>

uint8_t rdram[2 * 1024 * 1024];

struct AxisCase {
    const char* name;
    int32_t value;
    int32_t limit;
    int direction;
    int32_t expected;
};

int main() {
    const AxisCase cases[] = {
        {"forward from rest", 0, 192, 1, 48},
        {"reverse from rest", 0, 192, -1, -48},
        {"positive clamp", 180, 192, 1, 192},
        {"negative clamp", -180, 192, -1, -192},
        {"release positive", 192, 192, 0, 168},
        {"release negative", -192, 192, 0, -168},
        {"release near positive zero", 10, 192, 0, 0},
        {"release near negative zero", -10, 192, 0, 0},
        {"reverse direction", 48, 192, -1, 0},
        {"neutral deceleration", 100, 192, 0, 76},
        {"integer shifts", 0, 7, 1, 1},
        {"zero limit", 0, 0, 1, 0},
    };
    for (const auto &test : cases) {
        int32_t actual = S32(ST_UpdateMovementAxis((uint32_t)test.value, test.limit, test.direction));
        if (actual != test.expected) {
            std::fprintf(stderr, "%s: got %d, expected %d\n", test.name, actual, test.expected);
            return 1;
        }
    }
    std::printf("%zu movement-axis cases passed\n", sizeof cases / sizeof cases[0]);
    return 0;
}
