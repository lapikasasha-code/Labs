#include "cpu.hpp"

#include <iomanip>
#include <iostream>

#include "alu.hpp"

const Byte GROUP_CONTROL = 0x0;
const Byte GROUP_ALU = 0x1;
const Byte GROUP_DATA = 0x2;

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

static std::uint16_t instruction_size(Byte op) {
    switch (op) {
    case OP_LOADI_A:
    case OP_LOADI_B:
        return 2;
    case OP_LOAD_A:
    case OP_LOAD_B:
    case OP_STORE_A:
    case OP_STORE_B:
    case OP_LOADH:
        return 3;
    default:
        return 1;
    }
}

static void exec_control(CPU& cpu, Byte op) {
    switch (op) {
    case OP_HALT: cpu.halted = true; break;
    case OP_NOP: break;
    case OP_OUT: std::cout << (char)cpu.a; break;
    case OP_OUTN: std::cout << (int)cpu.a << ' '; break;
    default: break;
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
}

static void exec_data(CPU& cpu, Byte op) {
    Memory& mem = *cpu.mem;
    Byte imm = mem_get(mem, (std::size_t)cpu.pc + 1);
    std::uint16_t addr = get16(mem, (std::size_t)cpu.pc + 1);

    switch (op) {
    case OP_LOADI_A: cpu.a = imm; break;
    case OP_LOADI_B: cpu.b = imm; break;
    case OP_LOAD_A: cpu.a = mem_get(mem, addr); break;
    case OP_LOAD_B: cpu.b = mem_get(mem, addr); break;
    case OP_STORE_A: mem_set(mem, addr, cpu.a); break;
    case OP_STORE_B: mem_set(mem, addr, cpu.b); break;
    case OP_MOV_AB: cpu.a = cpu.b; break;
    case OP_MOV_BA: cpu.b = cpu.a; break;
    case OP_LOADH: cpu.h = addr; break;
    case OP_LOAD_A_H: cpu.a = mem_get(mem, cpu.h); break;
    case OP_STORE_H_A: mem_set(mem, cpu.h, cpu.a); break;
    case OP_INCH: cpu.h = (std::uint16_t)(cpu.h + 1); break;
    case OP_DECH: cpu.h = (std::uint16_t)(cpu.h - 1); break;
    default: break;
    }
}

void step(CPU& cpu) {
    if (cpu.halted) {
        std::cout << "halted\n";
        return;
    }
    if ((std::size_t)cpu.pc >= MEM_SIZE) {
        std::cout << "error: PC=" << cpu.pc << " is outside memory\n";
        cpu.halted = true;
        return;
    }
    Byte op = mem_get(*cpu.mem, cpu.pc);
    std::uint16_t size = instruction_size(op);
    if ((std::size_t)cpu.pc + size > MEM_SIZE) {
        std::cout << "error: instruction at PC=" << cpu.pc << " runs past the end of memory\n";
        cpu.halted = true;
        return;
    }

    Decoded d = decode(op);
    switch (d.group) {
    case GROUP_CONTROL: exec_control(cpu, op); break;
    case GROUP_ALU: exec_alu(cpu, op); break;
    case GROUP_DATA: exec_data(cpu, op); break;
    default: break;
    }
    cpu.pc = (std::uint16_t)(cpu.pc + size);
}

void run(CPU& cpu) {
    while (!cpu.halted) step(cpu);
    std::cout << '\n';
}

void dump_regs(const CPU& cpu) {
    std::cout << std::hex << std::setfill('0') << "PC=" << std::setw(4) << cpu.pc
              << "  H=" << std::setw(4) << cpu.h << std::dec << std::setfill(' ')
              << "  A=" << (int)cpu.a << "  B=" << (int)cpu.b << "  Z=" << cpu.f.z
              << " N=" << cpu.f.n << " C=" << cpu.f.c << (cpu.halted ? "  halted" : "")
              << '\n';
}
