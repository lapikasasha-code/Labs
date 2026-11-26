#include "stack.hpp"

#include <iomanip>
#include <iostream>

bool empty(const Stack& s) {
    return s.sp >= STACK_HI;
}

bool full(const Stack& s) {
    return s.sp < STACK_LO;
}

bool push(Stack& s, Memory& mem, Byte value) {
    if (full(s)) return false;
    mem_set(mem, s.sp, value);
    s.sp = (std::uint16_t)(s.sp - 1);
    return true;
}

bool pop(Stack& s, const Memory& mem, Byte& out) {
    if (empty(s)) return false;
    s.sp = (std::uint16_t)(s.sp + 1);
    out = mem_get(mem, s.sp);
    return true;
}

std::uint16_t stack_pointer(const Stack& s) {
    return s.sp;
}

void print_stack(const Stack& s, const Memory& mem) {
    std::cout << std::hex << std::setfill('0') << "SP=" << std::setw(4) << s.sp;
    if (empty(s)) {
        std::cout << "  (empty)";
    } else {
        std::cout << "  top:";
        int shown = 0;
        for (std::uint16_t a = (std::uint16_t)(s.sp + 1); a <= STACK_HI && shown < 8; ++a, ++shown) {
            std::cout << ' ' << std::setw(4) << a << '=' << std::setw(2) << (int)mem_get(mem, a);
        }
    }
    std::cout << std::dec << std::setfill(' ') << '\n';
}
