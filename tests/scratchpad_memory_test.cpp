#include "recomp.h"
#include <cstdio>

uint8_t rdram[2 * 1024 * 1024];
void writeScratchpadFromAnotherFile();

int main() {
    writeScratchpadFromAnotherFile();
    if (MEM_W(0x1C8, 0x1F800000) != 0x12345678 ||
        MEM_W(0x1C8, 0xBF800000) != 0x12345678) {
        std::fputs("scratchpad storage is not shared\n", stderr);
        return 1;
    }
    std::puts("scratchpad is shared across translation units and aliases");
    return 0;
}
