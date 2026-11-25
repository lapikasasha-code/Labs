#include "screen.hpp"

#include <iostream>

void show(const Memory& mem) {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            Byte cell = mem.data[0xA00 + y * 8 + (x / 8)];
            bool lit  = (cell >> (7 - (x % 8))) & 1;
            std::cout << (lit ? '#' : '.');
        }
        std::cout << '\n';
    }
}

bool plot(Memory& mem, int x, int y) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return false;
    std::size_t addr = VRAM_LO + (std::size_t)(y * 8 + x / 8);
    mem.data[addr] = (Byte)(mem.data[addr] | (1u << (7 - x % 8)));
    return true;
}

void clear(Memory& mem) {
    for (std::size_t i = 0; i < VRAM_SIZE; ++i) mem.data[VRAM_LO + i] = 0;
}
