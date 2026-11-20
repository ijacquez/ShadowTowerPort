#include "recomp.h"

void writeScratchpadFromAnotherFile() {
    MEM_W(0x1C8, 0x1F800000) = 0x12345678;
}
