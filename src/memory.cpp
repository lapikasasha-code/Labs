// memory.cpp — YOUR WORK (Lab 1, M3).
//
// Two functions, a few lines each. Both need exactly one `if`: an address is
// valid when it is smaller than MEM_SIZE. Addresses are counted from 0, so the
// last valid address is MEM_SIZE - 1, not MEM_SIZE. That off-by-one is the
// whole subject of Lab 3.
#include "memory.hpp"

Byte mem_get(const Memory& mem, std::size_t addr) {
    if (addr >= MEM_SIZE) return 0;
    return mem.data[addr];
}

bool mem_set(Memory& mem, std::size_t addr, Byte value) {
    if (addr >= MEM_SIZE) return false;
    mem.data[addr] = value;
    return true;
}
std::uint16_t get16(const Memory& mem, std::size_t addr) {
    Byte lo = mem_get(mem, addr);
    Byte hi = mem_get(mem, addr + 1);
    return (std::uint16_t)(lo | (hi << 8));
}

bool set16(Memory& mem, std::size_t addr, std::uint16_t value) {
    if (addr + 1 >= MEM_SIZE) return false;
    mem.data[addr] = (Byte)(value & 0xFF);
    mem.data[addr + 1] = (Byte)((value >> 8) & 0xFF);
    return true;
}

std::uint16_t find(const Memory& mem, std::uint16_t lo, std::uint16_t hi, Byte needle) {
    for (std::uint16_t i = lo; i < hi; ++i) {
        if (mem_get(mem, i) == needle) return i;
    }
    return hi;
}
