#include "cpu.hpp"

#include <iomanip>
#include <iostream>

#include "alu.hpp"

const Byte GROUP_CONTROL = 0x0;
const Byte GROUP_ALU = 0x1;

struct Decoded {
    Byte group;
    Byte index;
};

static Decoded decode(Byte op) {
    Decoded d;
    d.group = (Byte)((op >> 4) & 0x0F);
    d.index = (Byte)(op & 0x0F);
    return d;
}

static void exec_control(CPU& cpu, Byte op) {
    switch (op) {
    case OP_HALT:
        cpu.halted = true;
        cpu.pc += 1;
        break;
    case OP_NOP:
        cpu.pc += 1;
        break;
    default:
        cpu.pc += 1;
        break;
    }
}

static void exec_alu(CPU& cpu, Byte op) {
    switch (op) {
    case OP_ADD: cpu.a = alu_add(cpu.a, cpu.b, cpu.f); break;
    case OP_SUB: cpu.a = alu_sub(cpu.a, cpu.b, cpu.f); break;
    case OP_AND: cpu.a = alu_and(cpu.a, cpu.b, cpu.f); break;
    case OP_OR:  cpu.a = alu_or(cpu.a, cpu.b, cpu.f); break;
    case OP_XOR: cpu.a = alu_xor(cpu.a, cpu.b, cpu.f); break;
    case OP_NOT: cpu.a = alu_not(cpu.a, cpu.f); break;
    case OP_SHL: cpu.a = alu_shl(cpu.a, cpu.f); break;
    case OP_SHR: cpu.a = alu_shr(cpu.a, cpu.f); break;
    case OP_INC: cpu.a = alu_inc(cpu.a, cpu.f); break;
    case OP_DEC: cpu.a = alu_dec(cpu.a, cpu.f); break;
    default: break;
    }
    cpu.pc += 1;
}

void step(CPU& cpu) {
    if (cpu.halted) {
        std::cout << "halted\n";
        return;
    }
    Byte op = mem_get(*cpu.mem, cpu.pc);
    Decoded d = decode(op);

    switch (d.group) {
    case GROUP_CONTROL:
        exec_control(cpu, op);
        break;
    case GROUP_ALU:
        exec_alu(cpu, op);
        break;
    default:
        cpu.pc += 1;
        break;
    }
}

void dump_regs(const CPU& cpu) {
    std::cout << "PC=" << std::hex << std::setfill('0') << std::setw(4) << cpu.pc
              << std::dec << std::setfill(' ') << "  A=" << (int)cpu.a
              << "  B=" << (int)cpu.b << "  Z=" << cpu.f.z << " N=" << cpu.f.n
              << " C=" << cpu.f.c << (cpu.halted ? "  halted" : "") << '\n';
}
