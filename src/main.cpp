// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "alu.hpp"
#include "asm/assembler.hpp"
#include "asm/lexer.hpp"
#include "cpu.hpp"
#include "dump.hpp"
#include "memory.hpp"
#include "screen.hpp"
#include "sprite.hpp"

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

static bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

static bool load_program(const std::string& path, Memory& mem, bool listing) {
    std::string src, error;
    if (!read_file(path, src)) {
        std::cout << "cannot open " << path << '\n';
        return false;
    }
    Token* tokens = lex(src, error);
    if (!tokens) {
        std::cout << path << ": " << error << '\n';
        return false;
    }
    bool ok = assemble(tokens, mem, error, listing);
    destroy(tokens);
    if (!ok) std::cout << path << ": " << error << '\n';
    return ok;
}

static void print_tokens(const std::string& path) {
    std::string src, error;
    if (!read_file(path, src)) {
        std::cout << "cannot open " << path << '\n';
        return;
    }
    Token* tokens = lex(src, error);
    if (!tokens) {
        std::cout << path << ": " << error << '\n';
        return;
    }
    for (const Token* t = tokens; t; t = t->next) {
        std::cout << t->line << "  " << kind_name(t->kind);
        if (!t->text.empty()) std::cout << "  " << t->text;
        std::cout << '\n';
    }
    destroy(tokens);
}

static void print_help() {
    std::cout << "commands:\n"
              << "  inc <addr>        increase value by one byte\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  get <addr>        show one byte four ways\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  set16 <addr> <val> write a 16-bit value, low byte first\n"
              << "  run               step until HALT (at most 100000 steps)\n"
              << "  find <lo> <hi> <byte>  first address in [lo, hi) holding byte\n"
              << "  show              draw the screen (64x32 at 0xA00)\n"
              << "  cls               clear the screen\n"
              << "  plot <x> <y>      light one pixel\n"
              << "  frame             draw a frame around the screen\n"
              << "  sprite <i> <x> <y> <vx> <vy>  set sprite i (0..7)\n"
              << "  tick              move live sprites one step and show\n"
              << "  layout            sizeof and field offsets of CPU, Flags, Sprite\n"
              << "  reg <a|b> <val>   put a value into register A or B\n"
              << "  regs              print PC SP H A B Z N C\n"
              << "  stack             print SP and the top bytes of the stack\n"
              << "  step              execute one instruction at PC\n"
              << "  alu <op> <a> [b]  try one ALU operation (add sub and or xor not shl shr inc dec)\n"
              << "  lex <file.asm>    print the tokens of a file, one per line\n"
              << "  asm <file.asm>    assemble a file into memory at 0x000 and print the listing\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main(int argc, char** argv) {
    Memory mem;  // 4096 bytes, on the stack, zeroed by the {} in memory.hpp
    CPU cpu;
    cpu.mem = &mem;
    Sprite sprites[SPRITE_COUNT]{};

    if (argc > 1) {
        if (!load_program(argv[1], mem, false)) return 1;
        run(cpu);
        return 0;
    }

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
        } else if (cmd == "stack") {
            print_stack(cpu.stack, mem);
        } else if (cmd == "step") {
            step(cpu);
        } else if (cmd == "run") {
            run(cpu);
        } else if (cmd == "show") {
            show(mem);
        } else if (cmd == "cls") {
            clear(mem);
        } else if (cmd == "plot") {
            std::string xs, ys;
            long x = 0, y = 0;
            if (!(words >> xs) || !(words >> ys) || !parse_number(xs, x) || !parse_number(ys, y)) {
                std::cout << "usage: plot <x> <y>\n";
            } else if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H ||
                       !plot(mem, (int)x, (int)y)) {
                std::cout << "pixel must be inside 0..63 x 0..31\n";
            }
        } else if (cmd == "sprite") {
            std::string w[5];
            long v[5] = {0, 0, 0, 0, 0};
            bool ok = true;
            for (int k = 0; k < 5; ++k) ok = ok && (words >> w[k]) && parse_number(w[k], v[k]);
            if (!ok) {
                std::cout << "usage: sprite <i> <x> <y> <vx> <vy>\n";
            } else if (v[0] < 0 || v[0] >= SPRITE_COUNT || v[1] < 0 || v[1] >= SCREEN_W ||
                       v[2] < 0 || v[2] >= SCREEN_H || v[3] < -8 || v[3] > 8 || v[4] < -8 ||
                       v[4] > 8) {
                std::cout << "need i 0..7, x 0..63, y 0..31, vx and vy -8..8\n";
            } else {
                sprites[v[0]] = Sprite{(std::uint8_t)v[1], (std::uint8_t)v[2], (std::int8_t)v[3],
                                       (std::int8_t)v[4], true};
            }
        } else if (cmd == "tick") {
            tick(sprites, SPRITE_COUNT, mem);
        } else if (cmd == "layout") {
            std::cout << "sizeof(Flags)  = " << sizeof(Flags) << "  z@" << offsetof(Flags, z)
                      << " n@" << offsetof(Flags, n) << " c@" << offsetof(Flags, c) << '\n';
            std::cout << "sizeof(CPU)    = " << sizeof(CPU) << "  mem@" << offsetof(CPU, mem)
                      << " pc@" << offsetof(CPU, pc) << " a@" << offsetof(CPU, a) << " b@"
                      << offsetof(CPU, b) << " h@" << offsetof(CPU, h) << " f@"
                      << offsetof(CPU, f) << " stack@" << offsetof(CPU, stack) << " halted@" << offsetof(CPU, halted) << '\n';
            std::cout << "sizeof(Sprite) = " << sizeof(Sprite) << "  x@" << offsetof(Sprite, x)
                      << " y@" << offsetof(Sprite, y) << " vx@" << offsetof(Sprite, vx)
                      << " vy@" << offsetof(Sprite, vy) << " alive@" << offsetof(Sprite, alive)
                      << '\n';
        } else if (cmd == "lex") {
            std::string path;
            if (!(words >> path)) {
                std::cout << "usage: lex <file.asm>\n";
            } else {
                print_tokens(path);
            }
        } else if (cmd == "asm") {
            std::string path;
            if (!(words >> path)) {
                std::cout << "usage: asm <file.asm>\n";
            } else if (load_program(path, mem, true)) {
                cpu = CPU{};
                cpu.mem = &mem;
            }
        } else if (cmd == "frame") {
            for (int x = 0; x < SCREEN_W; ++x) {
                plot(mem, x, 0);
                plot(mem, x, SCREEN_H - 1);
            }
            for (int y = 0; y < SCREEN_H; ++y) {
                plot(mem, 0, y);
                plot(mem, SCREEN_W - 1, y);
            }
        } else if (cmd == "find") {
            std::string l, h, v;
            long lo = 0, hi = 0, value = 0;
            if (!(words >> l) || !(words >> h) || !(words >> v) || !parse_number(l, lo) ||
                !parse_number(h, hi) || !parse_number(v, value)) {
                std::cout << "usage: find <lo> <hi> <byte>\n";
            } else if (lo < 0 || hi < lo || hi > (long)MEM_SIZE) {
                std::cout << "range must satisfy 0 <= lo <= hi <= " << MEM_SIZE << '\n';
            } else if (value < 0 || value > 255) {
                std::cout << "a byte is 0..255, got " << value << '\n';
            } else {
                std::uint16_t at = find(mem, (std::uint16_t)lo, (std::uint16_t)hi, (Byte)value);
                if (at == hi) {
                    std::cout << "not found in [" << lo << ", " << hi << ")\n";
                } else {
                    std::cout << "found at " << at << " (0x" << std::hex << at << std::dec
                              << ")\n";
                }
            }
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
