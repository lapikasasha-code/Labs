#include "alu.hpp"

static Byte set_zn(Byte r, Flags& f) {
    f.z = r == 0;
    f.n = (r & 0x80) != 0;
    return r;
}

Byte alu_add(Byte a, Byte b, Flags& f) {
    unsigned sum = (unsigned)a + (unsigned)b;
    f.c = sum > 0xFF;
    return set_zn((Byte)sum, f);
}

Byte alu_sub(Byte a, Byte b, Flags& f) {
    f.c = a < b;
    return set_zn((Byte)(a - b), f);
}

Byte alu_and(Byte a, Byte b, Flags& f) {
    f.c = false;
    return set_zn((Byte)(a & b), f);
}

Byte alu_or(Byte a, Byte b, Flags& f) {
    f.c = false;
    return set_zn((Byte)(a | b), f);
}

Byte alu_xor(Byte a, Byte b, Flags& f) {
    f.c = false;
    return set_zn((Byte)(a ^ b), f);
}

Byte alu_not(Byte a, Flags& f) {
    f.c = false;
    return set_zn((Byte)~a, f);
}

Byte alu_shl(Byte a, Flags& f) {
    f.c = (a & 0x80) != 0;
    return set_zn((Byte)(a << 1), f);
}

Byte alu_shr(Byte a, Flags& f) {
    f.c = (a & 0x01) != 0;
    return set_zn((Byte)(a >> 1), f);
}

Byte alu_inc(Byte a, Flags& f) {
    return set_zn((Byte)(a + 1), f);
}

Byte alu_dec(Byte a, Flags& f) {
    return set_zn((Byte)(a - 1), f);
}
