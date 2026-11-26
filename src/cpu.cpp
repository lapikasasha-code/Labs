#include "cpu.hpp"

#include <iomanip>
#include <iostream>

#include "alu.hpp"
#include "screen.hpp"

const Byte GROUP_CONTROL = 0x0;
const Byte GROUP_ALU = 0x1;
const Byte GROUP_DATA = 0x2;
const Byte GROUP_JUMP = 0x3;
const Byte GROUP_SCREEN = 0x4;
const Byte GROUP_STACK = 0x5;

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

std::uint16_t instruction_size(Op op) {
    switch (op) {
    case Op::LoadiA:
    case Op::LoadiB:
        return 2;
    case Op::LoadA:
    case Op::LoadB:
    case Op::StoreA:
    case Op::StoreB:
    case Op::LoadH:
    case Op::Outs:
    case Op::Jmp:
    case Op::Jz:
    case Op::Jnz:
    case Op::Call:
        return 3;
    default:
        return 1;
    }
}

static void outs(const Memory& mem, std::size_t addr) {
    for (std::size_t i = 0; i < OUTS_LIMIT && addr + i < MEM_SIZE; ++i) {
        Byte c = mem_get(mem, addr + i);
        if (c == 0) break;
        std::cout << (char)c;
    }
}

static bool exec_control(CPU& cpu, Op op) {
    switch (op) {
    case Op::Halt: cpu.halted = true; return true;
    case Op::Nop: return true;
    case Op::Out: std::cout << (char)cpu.a; return true;
    case Op::Outn: std::cout << (int)cpu.a << ' '; return true;
    case Op::Outs: outs(*cpu.mem, get16(*cpu.mem, (std::size_t)cpu.pc + 1)); return true;
    default: return false;
    }
}

static bool exec_alu(CPU& cpu, Op op) {
    switch (op) {
    case Op::Add: cpu.a = alu_add(cpu.a, cpu.b, cpu.f); return true;
    case Op::Sub: cpu.a = alu_sub(cpu.a, cpu.b, cpu.f); return true;
    case Op::And: cpu.a = alu_and(cpu.a, cpu.b, cpu.f); return true;
    case Op::Or:  cpu.a = alu_or(cpu.a, cpu.b, cpu.f); return true;
    case Op::Xor: cpu.a = alu_xor(cpu.a, cpu.b, cpu.f); return true;
    case Op::Not: cpu.a = alu_not(cpu.a, cpu.f); return true;
    case Op::Shl: cpu.a = alu_shl(cpu.a, cpu.f); return true;
    case Op::Shr: cpu.a = alu_shr(cpu.a, cpu.f); return true;
    case Op::Inc: cpu.a = alu_inc(cpu.a, cpu.f); return true;
    case Op::Dec: cpu.a = alu_dec(cpu.a, cpu.f); return true;
    case Op::Cmp: alu_sub(cpu.a, cpu.b, cpu.f); return true;
    case Op::Mul: cpu.a = alu_mul(cpu.a, cpu.b, cpu.f); return true;
    default: return false;
    }
}

static bool exec_data(CPU& cpu, Op op) {
    Memory& mem = *cpu.mem;
    Byte imm = mem_get(mem, (std::size_t)cpu.pc + 1);
    std::uint16_t addr = get16(mem, (std::size_t)cpu.pc + 1);

    switch (op) {
    case Op::LoadiA: cpu.a = imm; return true;
    case Op::LoadiB: cpu.b = imm; return true;
    case Op::LoadA: cpu.a = mem_get(mem, addr); return true;
    case Op::LoadB: cpu.b = mem_get(mem, addr); return true;
    case Op::StoreA: mem_set(mem, addr, cpu.a); return true;
    case Op::StoreB: mem_set(mem, addr, cpu.b); return true;
    case Op::MovAB: cpu.a = cpu.b; return true;
    case Op::MovBA: cpu.b = cpu.a; return true;
    case Op::LoadH: cpu.h = addr; return true;
    case Op::LoadAH: cpu.a = mem_get(mem, cpu.h); return true;
    case Op::StoreHA: mem_set(mem, cpu.h, cpu.a); return true;
    case Op::IncH: cpu.h = (std::uint16_t)(cpu.h + 1); return true;
    case Op::DecH: cpu.h = (std::uint16_t)(cpu.h - 1); return true;
    case Op::HLow: cpu.a = (Byte)(cpu.h & 0xFF); return true;
    default: return false;
    }
}

static bool exec_jump(CPU& cpu, Op op, std::uint16_t& next) {
    std::uint16_t target = get16(*cpu.mem, (std::size_t)cpu.pc + 1);

    switch (op) {
    case Op::Jmp: next = target; return true;
    case Op::Jz: if (cpu.f.z) next = target; return true;
    case Op::Jnz: if (!cpu.f.z) next = target; return true;
    default: return false;
    }
}

static bool exec_screen(CPU& cpu, Op op) {
    switch (op) {
    case Op::Cls: clear(*cpu.mem); return true;
    case Op::Plot: cpu.f.c = !plot(*cpu.mem, cpu.a, cpu.b); return true;
    case Op::Show: show(*cpu.mem); return true;
    default: return false;
    }
}

static void stack_error(CPU& cpu, const char* what, std::uint16_t& next) {
    std::cout << "error: stack " << what << " at PC=0x" << std::hex << std::setfill('0')
              << std::setw(4) << cpu.pc << ", SP=0x" << std::setw(4)
              << stack_pointer(cpu.stack) << std::dec << std::setfill(' ') << '\n';
    cpu.halted = true;
    next = cpu.pc;
}

static bool exec_stack(CPU& cpu, Op op, std::uint16_t& next) {
    Memory& mem = *cpu.mem;
    switch (op) {
    case Op::PushA:
        if (!push(cpu.stack, mem, cpu.a)) stack_error(cpu, "overflow", next);
        return true;
    case Op::PushB:
        if (!push(cpu.stack, mem, cpu.b)) stack_error(cpu, "overflow", next);
        return true;
    case Op::PopA:
        if (!pop(cpu.stack, mem, cpu.a)) stack_error(cpu, "underflow", next);
        return true;
    case Op::PopB:
        if (!pop(cpu.stack, mem, cpu.b)) stack_error(cpu, "underflow", next);
        return true;
    case Op::Call: {
        std::uint16_t ret = (std::uint16_t)(cpu.pc + 3);
        if (!push(cpu.stack, mem, (Byte)(ret & 0xFF)) ||
            !push(cpu.stack, mem, (Byte)(ret >> 8))) {
            stack_error(cpu, "overflow", next);
            return true;
        }
        next = get16(mem, (std::size_t)cpu.pc + 1);
        return true;
    }
    case Op::Ret: {
        Byte hi = 0, lo = 0;
        if (!pop(cpu.stack, mem, hi) || !pop(cpu.stack, mem, lo)) {
            stack_error(cpu, "underflow", next);
            return true;
        }
        next = (std::uint16_t)((hi << 8) | lo);
        return true;
    }
    default:
        return false;
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
    Byte byte = mem_get(*cpu.mem, cpu.pc);
    Op op = (Op)byte;
    std::uint16_t size = instruction_size(op);
    if ((std::size_t)cpu.pc + size > MEM_SIZE) {
        std::cout << "error: instruction at PC=" << cpu.pc << " runs past the end of memory\n";
        cpu.halted = true;
        return;
    }

    std::uint16_t next = (std::uint16_t)(cpu.pc + size);
    bool known = false;
    Decoded d = decode(byte);
    switch (d.group) {
    case GROUP_CONTROL: known = exec_control(cpu, op); break;
    case GROUP_ALU: known = exec_alu(cpu, op); break;
    case GROUP_DATA: known = exec_data(cpu, op); break;
    case GROUP_JUMP: known = exec_jump(cpu, op, next); break;
    case GROUP_SCREEN: known = exec_screen(cpu, op); break;
    case GROUP_STACK: known = exec_stack(cpu, op, next); break;
    default: known = false; break;
    }

    if (!known) {
        std::cout << "error: unknown opcode 0x" << std::hex << std::setfill('0') << std::setw(2)
                  << (int)byte << " at PC=0x" << std::setw(4) << cpu.pc << std::dec
                  << std::setfill(' ') << '\n';
        cpu.halted = true;
        return;
    }
    cpu.pc = next;
}

void run(CPU& cpu) {
    long steps = 0;
    while (!cpu.halted && steps < RUN_STEP_LIMIT) {
        step(cpu);
        ++steps;
    }
    std::cout << '\n';
    if (cpu.halted) {
        std::cout << "stopped after " << steps << " steps\n";
    } else {
        std::cout << "step limit " << RUN_STEP_LIMIT << " reached, machine paused at PC=0x"
                  << std::hex << std::setfill('0') << std::setw(4) << cpu.pc << std::dec
                  << std::setfill(' ') << '\n';
    }
}

void dump_regs(const CPU& cpu) {
    std::cout << std::hex << std::setfill('0') << "PC=" << std::setw(4) << cpu.pc
              << "  SP=" << std::setw(4) << stack_pointer(cpu.stack) << "  H=" << std::setw(4)
              << cpu.h << std::dec << std::setfill(' ')
              << "  A=" << (int)cpu.a << "  B=" << (int)cpu.b << "  Z=" << cpu.f.z
              << " N=" << cpu.f.n << " C=" << cpu.f.c << (cpu.halted ? "  halted" : "")
              << '\n';
}
