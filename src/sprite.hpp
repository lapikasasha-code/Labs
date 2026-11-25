#pragma once

#include <cstdint>

#include "memory.hpp"

struct Sprite {
    std::uint8_t x, y;
    std::int8_t vx, vy;
    bool alive;
};

const int SPRITE_COUNT = 8;

void tick(Sprite sprites[], int count, Memory& mem);
