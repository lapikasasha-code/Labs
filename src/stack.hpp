#pragma once

#include <cstdint>

#include "memory.hpp"

const std::uint16_t STACK_LO = 0xF00;
const std::uint16_t STACK_HI = 0xFFF;

struct Stack {
    std::uint16_t sp = STACK_HI;
};

bool push(Stack& s, Memory& mem, Byte value);
bool pop(Stack& s, const Memory& mem, Byte& out);
bool empty(const Stack& s);
bool full(const Stack& s);
std::uint16_t stack_pointer(const Stack& s);
void print_stack(const Stack& s, const Memory& mem);
