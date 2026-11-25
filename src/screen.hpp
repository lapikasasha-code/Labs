#pragma once

#include "memory.hpp"

const std::size_t VRAM_LO = 0xA00;
const std::size_t VRAM_SIZE = 256;
const int SCREEN_W = 64;
const int SCREEN_H = 32;

void show(const Memory& mem);
bool plot(Memory& mem, int x, int y);
void clear(Memory& mem);
