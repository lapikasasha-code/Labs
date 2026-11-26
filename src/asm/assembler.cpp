#include "assembler.hpp"

#include <iomanip>
#include <iostream>
#include <vector>

#include "../cpu.hpp"

namespace {

enum class Kind { RegA, RegB, RegH, Imm, Mem, MemH };

struct Operand {
    Kind kind;
    const Token* value;
};

struct Form {
    const char* mnemonic;
    std::vector<Kind> operands;
    Op op;
};

const std::vector<Form> FORMS = {
    {"HALT", {}, Op::Halt},
    {"NOP", {}, Op::Nop},
    {"OUT", {}, Op::Out},
    {"OUTN", {}, Op::Outn},
    {"OUTS", {Kind::Imm}, Op::Outs},
    {"ADD", {Kind::RegA, Kind::RegB}, Op::Add},
    {"SUB", {Kind::RegA, Kind::RegB}, Op::Sub},
    {"AND", {Kind::RegA, Kind::RegB}, Op::And},
    {"OR", {Kind::RegA, Kind::RegB}, Op::Or},
    {"XOR", {Kind::RegA, Kind::RegB}, Op::Xor},
    {"NOT", {Kind::RegA}, Op::Not},
    {"SHL", {Kind::RegA}, Op::Shl},
    {"SHR", {Kind::RegA}, Op::Shr},
    {"INC", {Kind::RegA}, Op::Inc},
    {"DEC", {Kind::RegA}, Op::Dec},
    {"CMP", {Kind::RegA, Kind::RegB}, Op::Cmp},
    {"MUL", {Kind::RegA, Kind::RegB}, Op::Mul},
    {"LOADI", {Kind::RegA, Kind::Imm}, Op::LoadiA},
    {"LOADI", {Kind::RegB, Kind::Imm}, Op::LoadiB},
    {"LOAD", {Kind::RegA, Kind::Mem}, Op::LoadA},
    {"LOAD", {Kind::RegB, Kind::Mem}, Op::LoadB},
    {"STORE", {Kind::Mem, Kind::RegA}, Op::StoreA},
    {"STORE", {Kind::Mem, Kind::RegB}, Op::StoreB},
    {"MOV", {Kind::RegA, Kind::RegB}, Op::MovAB},
    {"MOV", {Kind::RegB, Kind::RegA}, Op::MovBA},
    {"LOADH", {Kind::RegH, Kind::Imm}, Op::LoadH},
    {"LOAD", {Kind::RegA, Kind::MemH}, Op::LoadAH},
    {"STORE", {Kind::MemH, Kind::RegA}, Op::StoreHA},
    {"INCH", {}, Op::IncH},
    {"DECH", {}, Op::DecH},
    {"HLOW", {}, Op::HLow},
    {"JMP", {Kind::Imm}, Op::Jmp},
    {"JZ", {Kind::Imm}, Op::Jz},
    {"JNZ", {Kind::Imm}, Op::Jnz},
    {"CLS", {}, Op::Cls},
    {"PLOT", {}, Op::Plot},
    {"SHOW", {}, Op::Show},
    {"PUSH", {Kind::RegA}, Op::PushA},
    {"POP", {Kind::RegA}, Op::PopA},
    {"PUSH", {Kind::RegB}, Op::PushB},
    {"POP", {Kind::RegB}, Op::PopB},
    {"CALL", {Kind::Imm}, Op::Call},
    {"RET", {}, Op::Ret},
};

struct Label {
    std::string name;
    std::uint16_t addr;
};

struct Line {
    int number;
    const Token* label;
    const Token* mnemonic;
    std::vector<Operand> operands;
};

struct Assembler {
    std::vector<Label> labels;
    std::string error;
};

bool fail(Assembler& as, int line, const std::string& msg) {
    if (as.error.empty()) as.error = "line " + std::to_string(line) + ": " + msg;
    return false;
}

bool parse_value(const std::string& text, long& out) {
    try {
        if (text.size() > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
            out = std::stol(text.substr(2), nullptr, 2);
        } else {
            out = std::stol(text, nullptr, 0);
        }
        return true;
    } catch (...) {
        return false;
    }
}

const Label* find_label(const Assembler& as, const std::string& name) {
    for (const Label& l : as.labels) {
        if (l.name == name) return &l;
    }
    return nullptr;
}

bool is_register(const std::string& s) {
    return s == "A" || s == "B" || s == "H";
}

bool parse_operand(Assembler& as, const Token*& t, int line, Operand& out) {
    if (t->kind == Token::Kind::Ident) {
        if (t->text == "A") out = {Kind::RegA, t};
        else if (t->text == "B") out = {Kind::RegB, t};
        else if (t->text == "H") out = {Kind::RegH, t};
        else out = {Kind::Imm, t};
        t = t->next;
        return true;
    }
    if (t->kind == Token::Kind::Number) {
        out = {Kind::Imm, t};
        t = t->next;
        return true;
    }
    if (t->kind == Token::Kind::LBracket) {
        const Token* inner = t->next;
        if (inner->kind != Token::Kind::Number && inner->kind != Token::Kind::Ident) {
            return fail(as, line, "expected address inside [ ]");
        }
        if (inner->next->kind != Token::Kind::RBracket) return fail(as, line, "missing ']'");
        out = {inner->text == "H" ? Kind::MemH : Kind::Mem, inner};
        t = inner->next->next;
        return true;
    }
    return fail(as, line, "unexpected '" + t->text + "'");
}

bool split_lines(Assembler& as, const Token* head, std::vector<Line>& lines) {
    const Token* t = head;
    while (t->kind != Token::Kind::Eof) {
        Line line{t->line, nullptr, nullptr, {}};
        if (t->kind == Token::Kind::Ident && t->next->kind == Token::Kind::Colon) {
            line.label = t;
            t = t->next->next;
        }
        if (t->kind == Token::Kind::Ident) {
            line.mnemonic = t;
            t = t->next;
            while (t->kind != Token::Kind::Newline && t->kind != Token::Kind::Eof) {
                Operand op{};
                if (!parse_operand(as, t, line.number, op)) return false;
                line.operands.push_back(op);
                if (t->kind == Token::Kind::Comma) {
                    t = t->next;
                    if (t->kind == Token::Kind::Newline || t->kind == Token::Kind::Eof) {
                        return fail(as, line.number, "operand expected after ','");
                    }
                } else if (t->kind != Token::Kind::Newline && t->kind != Token::Kind::Eof) {
                    return fail(as, line.number, "missing ',' before '" + t->text + "'");
                }
            }
        } else if (t->kind != Token::Kind::Newline && t->kind != Token::Kind::Eof) {
            return fail(as, line.number, "expected instruction, got '" + t->text + "'");
        }
        if (line.label || line.mnemonic) lines.push_back(line);
        if (t->kind == Token::Kind::Newline) t = t->next;
    }
    return true;
}

const Form* match(const Line& line) {
    for (const Form& f : FORMS) {
        if (line.mnemonic->text != f.mnemonic || f.operands.size() != line.operands.size()) {
            continue;
        }
        bool ok = true;
        for (std::size_t i = 0; i < f.operands.size(); ++i) {
            if (f.operands[i] != line.operands[i].kind) ok = false;
        }
        if (ok) return &f;
    }
    return nullptr;
}

bool known_mnemonic(const std::string& m) {
    if (m == "ORG" || m == "DB") return true;
    for (const Form& f : FORMS) {
        if (m == f.mnemonic) return true;
    }
    return false;
}

bool value_of(Assembler& as, const Operand& op, int line, long& out) {
    if (op.value->kind == Token::Kind::Number) {
        if (!parse_value(op.value->text, out) || out < 0 || out > 0xFFFF) {
            return fail(as, line, "number out of range: " + op.value->text);
        }
        return true;
    }
    const Label* l = find_label(as, op.value->text);
    if (!l) return fail(as, line, "unknown label '" + op.value->text + "'");
    out = l->addr;
    return true;
}

bool line_size(Assembler& as, const Line& line, std::uint16_t& size) {
    const std::string& m = line.mnemonic->text;
    if (m == "DB") {
        size = (std::uint16_t)line.operands.size();
        return true;
    }
    const Form* f = match(line);
    if (!f) {
        if (!known_mnemonic(m)) return fail(as, line.number, "unknown mnemonic '" + m + "'");
        return fail(as, line.number, "wrong operands for " + m);
    }
    size = instruction_size(f->op);
    return true;
}

void print_listing_line(std::uint16_t addr, const std::vector<Byte>& bytes, const Line& line) {
    std::cout << std::hex << std::setfill('0') << std::setw(4) << addr << "  ";
    std::string hex;
    for (std::size_t i = 0; i < bytes.size() && i < 4; ++i) {
        const char* digits = "0123456789abcdef";
        hex += digits[bytes[i] >> 4];
        hex += digits[bytes[i] & 0x0F];
        hex += ' ';
    }
    if (bytes.size() > 4) hex += "... ";
    std::cout << std::left << std::setfill(' ') << std::setw(16) << hex << std::right << std::dec;
    if (line.label) std::cout << line.label->text << ":";
    if (line.mnemonic) {
        std::cout << (line.label ? " " : "") << line.mnemonic->text;
        for (std::size_t i = 0; i < line.operands.size(); ++i) {
            const Operand& op = line.operands[i];
            std::cout << (i == 0 ? " " : ", ");
            if (op.kind == Kind::Mem || op.kind == Kind::MemH) {
                std::cout << '[' << op.value->text << ']';
            } else {
                std::cout << op.value->text;
            }
        }
    }
    std::cout << '\n';
}

}  // namespace

bool assemble(const Token* head, Memory& mem, std::string& error, bool listing) {
    Assembler as;
    std::vector<Line> lines;
    if (!split_lines(as, head, lines)) {
        error = as.error;
        return false;
    }

    // Pass 1: addresses of labels.
    long pc = 0;
    for (const Line& line : lines) {
        if (line.label) {
            if (is_register(line.label->text)) {
                fail(as, line.number, "'" + line.label->text + "' is a register, not a label");
                break;
            }
            if (find_label(as, line.label->text)) {
                fail(as, line.number, "label '" + line.label->text + "' defined twice");
                break;
            }
            as.labels.push_back({line.label->text, (std::uint16_t)pc});
        }
        if (!line.mnemonic) continue;
        if (line.mnemonic->text == "ORG") {
            long addr = 0;
            if (line.operands.size() != 1 || line.operands[0].value->kind != Token::Kind::Number ||
                !parse_value(line.operands[0].value->text, addr) || addr < 0 ||
                addr >= (long)MEM_SIZE) {
                fail(as, line.number, "ORG needs one address 0..4095");
                break;
            }
            pc = addr;
            continue;
        }
        std::uint16_t size = 0;
        if (!line_size(as, line, size)) break;
        pc += size;
        if (pc > (long)MEM_SIZE) {
            fail(as, line.number, "program does not fit in memory");
            break;
        }
    }
    if (!as.error.empty()) {
        error = as.error;
        return false;
    }

    // Pass 2: bytes.
    pc = 0;
    for (const Line& line : lines) {
        if (!line.mnemonic) {
            if (listing) print_listing_line((std::uint16_t)pc, {}, line);
            continue;
        }
        const std::string& m = line.mnemonic->text;
        std::vector<Byte> bytes;
        if (m == "ORG") {
            long addr = 0;
            parse_value(line.operands[0].value->text, addr);
            pc = addr;
            continue;
        }
        if (m == "DB") {
            for (const Operand& op : line.operands) {
                long v = 0;
                if (op.kind != Kind::Imm || !value_of(as, op, line.number, v)) {
                    if (as.error.empty()) fail(as, line.number, "DB takes numbers");
                    break;
                }
                if (v > 255) {
                    fail(as, line.number, "DB value does not fit in a byte: " + op.value->text);
                    break;
                }
                bytes.push_back((Byte)v);
            }
        } else {
            const Form* f = match(line);
            bytes.push_back((Byte)f->op);
            for (const Operand& op : line.operands) {
                if (op.kind != Kind::Imm && op.kind != Kind::Mem) continue;
                long v = 0;
                if (!value_of(as, op, line.number, v)) break;
                if (instruction_size(f->op) == 2) {
                    if (v > 255) {
                        fail(as, line.number, "value does not fit in a byte: " + op.value->text);
                        break;
                    }
                    bytes.push_back((Byte)v);
                } else {
                    bytes.push_back((Byte)(v & 0xFF));
                    bytes.push_back((Byte)((v >> 8) & 0xFF));
                }
            }
            if (as.error.empty() && bytes.size() != instruction_size(f->op)) {
                fail(as, line.number, "internal size mismatch for " + m);
            }
        }
        if (!as.error.empty()) break;
        if (listing) print_listing_line((std::uint16_t)pc, bytes, line);
        for (Byte b : bytes) {
            mem_set(mem, (std::size_t)pc, b);
            ++pc;
        }
    }
    if (!as.error.empty()) {
        error = as.error;
        return false;
    }
    return true;
}
