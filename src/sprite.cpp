#include "sprite.hpp"

#include "screen.hpp"

static void move_axis(std::uint8_t& pos, std::int8_t& vel, int limit) {
    int next = pos + vel;
    if (next < 0 || next >= limit) {
        vel = (std::int8_t)-vel;
        next = pos + vel;
    }
    pos = (std::uint8_t)next;
}

void tick(Sprite sprites[], int count, Memory& mem) {
    clear(mem);
    for (int i = 0; i < count; ++i) {
        Sprite& s = sprites[i];
        if (!s.alive) continue;
        move_axis(s.x, s.vx, SCREEN_W);
        move_axis(s.y, s.vy, SCREEN_H);
        plot(mem, s.x, s.y);
    }
    show(mem);
}
