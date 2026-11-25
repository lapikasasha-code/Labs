// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <iostream>
#include <sstream>
#include <string>

#include "alu.hpp"
#include "cpu.hpp"
#include "dump.hpp"
#include "memory.hpp"

// Turn a word into a number. Accepts decimal (65) and hex (0x41).
// Returns false if the word is not a number at all.
static bool parse_number(const std::string& word, long& out) {
    try {
        std::size_t used = 0;
        // base 0 means: look at the prefix. "0x41" is hex, "65" is decimal.
        out = std::stol(word, &used, 0);
        return used == word.size();  // reject things like "12abc"
    } catch (...) {
        return false;
    }
}

static bool run_alu(const std::string& name, Byte a, Byte b, Byte& out, Flags& f) {
    if (name == "add") out = alu_add(a, b, f);
    else if (name == "sub") out = alu_sub(a, b, f);
    else if (name == "and") out = alu_and(a, b, f);
    else if (name == "or") out = alu_or(a, b, f);
    else if (name == "xor") out = alu_xor(a, b, f);
    else if (name == "not") out = alu_not(a, f);
    else if (name == "shl") out = alu_shl(a, f);
    else if (name == "shr") out = alu_shr(a, f);
    else if (name == "inc") out = alu_inc(a, f);
    else if (name == "dec") out = alu_dec(a, f);
    else return false;
    return true;
}

static void print_help() {
    std::cout << "commands:\n"
              << "  inc <addr>        increase value by one byte\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  get <addr>        show one byte four ways\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  set16 <addr> <val> write a 16-bit value, low byte first\n"
              << "  run               step until HALT\n"
              << "  reg <a|b> <val>   put a value into register A or B\n"
              << "  regs              print PC H A B Z N C\n"
              << "  step              execute one instruction at PC\n"
              << "  alu <op> <a> [b]  try one ALU operation (add sub and or xor not shl shr inc dec)\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main() {
    Memory mem;  // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu;
    cpu.mem = &mem;

    std::cout << "ember 0.1 - 4096 bytes of memory you can see. Type `help`.\n";

    std::string line;
    while (true) {
        std::cout << "ember> ";

        // getline reads one whole line. It returns false at end of input
        // (Ctrl-D), which is how the loop ends if you never type `quit`.
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        // Split the line into words: "set 0 65" -> cmd="set", args "0" and "65".
        std::istringstream words(line);
        std::string cmd;
        words >> cmd;

        if (cmd.empty()) {
            continue;  // the user just pressed Enter
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "dump") {
            dump(mem);
        } else if (cmd == "get") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: get <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                show_byte(mem_get(mem, (std::size_t)addr));
            }
        } else if (cmd == "set") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 255) {
                // A cell holds ONE byte. 256 does not fit. Lab 1, theory 3.
                std::cout << "a byte is 0..255, got " << value << '\n';
            } else if (!mem_set(mem, (std::size_t)addr, (Byte)value)) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1
                          << '\n';
            }
        } else if (cmd == "inc") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr) || addr < 0) {
                std::cout << "usage: inc <addr>\n";
            } else {
                Byte b = mem_get(mem, (std::size_t)addr);
                mem_set(mem, (std::size_t)addr, (Byte)(b + 1));
            }
        } else if (cmd == "reg") {
            std::string r, v;
            long value = 0;
            if (!(words >> r) || !(words >> v) || !parse_number(v, value) || value < 0 ||
                value > 255) {
                std::cout << "usage: reg <a|b> <0..255>\n";
            } else if (r == "a") {
                cpu.a = (Byte)value;
            } else if (r == "b") {
                cpu.b = (Byte)value;
            } else {
                std::cout << "unknown register: " << r << '\n';
            }
        } else if (cmd == "regs") {
            dump_regs(cpu);
        } else if (cmd == "step") {
            step(cpu);
        } else if (cmd == "run") {
            run(cpu);
        } else if (cmd == "set16") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set16 <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 0xFFFF) {
                std::cout << "a 16-bit value is 0..65535, got " << value << '\n';
            } else if (!set16(mem, (std::size_t)addr, (std::uint16_t)value)) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 2
                          << '\n';
            }
        } else if (cmd == "alu") {
            std::string name, x, y;
            long a = 0, b = 0;
            Byte result = 0;
            Flags f;
            words >> name >> x;
            if (words >> y && !parse_number(y, b)) b = -1;
            if (!parse_number(x, a) || a < 0 || a > 255 || b < 0 || b > 255 ||
                !run_alu(name, (Byte)a, (Byte)b, result, f)) {
                std::cout << "usage: alu <add|sub|and|or|xor|not|shl|shr|inc|dec> <a> [b]\n";
            } else {
                std::cout << "result=" << (int)result << "  Z=" << f.z << " N=" << f.n
                          << " C=" << f.c << '\n';
            }
        } else {
            std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0;  // 0 means "success" to the operating system
}
