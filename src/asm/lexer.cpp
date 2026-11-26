#include "lexer.hpp"

#include <cctype>

struct Lexer {
    const std::string& src;
    std::size_t i = 0;
    int line = 1;
    Token* head = nullptr;
    Token* tail = nullptr;
    std::string error;
};

static void emit(Lexer& lx, Token::Kind kind, const std::string& text) {
    Token* t = new Token{kind, text, lx.line, nullptr};
    if (!lx.head) {
        lx.head = lx.tail = t;
    } else {
        lx.tail->next = t;
        lx.tail = t;
    }
}

static bool fail(Lexer& lx, const std::string& msg) {
    lx.error = "line " + std::to_string(lx.line) + ": " + msg;
    return false;
}

void destroy(Token* head) {
    while (head) {
        Token* next = head->next;
        delete head;
        head = next;
    }
}

const char* kind_name(Token::Kind kind) {
    switch (kind) {
    case Token::Kind::Ident: return "Ident";
    case Token::Kind::Number: return "Number";
    case Token::Kind::Comma: return "Comma";
    case Token::Kind::Colon: return "Colon";
    case Token::Kind::LBracket: return "LBracket";
    case Token::Kind::RBracket: return "RBracket";
    case Token::Kind::Newline: return "Newline";
    case Token::Kind::Eof: return "Eof";
    }
    return "?";
}

static bool is_hex(char c) {
    return std::isxdigit((unsigned char)c) != 0;
}

static bool is_bin(char c) {
    return c == '0' || c == '1';
}

static bool is_dec(char c) {
    return std::isdigit((unsigned char)c) != 0;
}

static bool number(Lexer& lx) {
    const std::string& s = lx.src;
    std::size_t start = lx.i;
    bool (*digit)(char) = is_dec;

    if (s[lx.i] == '0' && lx.i + 1 < s.size() && (s[lx.i + 1] == 'x' || s[lx.i + 1] == 'X')) {
        digit = is_hex;
        lx.i += 2;
    } else if (s[lx.i] == '0' && lx.i + 1 < s.size() && (s[lx.i + 1] == 'b' || s[lx.i + 1] == 'B')) {
        digit = is_bin;
        lx.i += 2;
    }

    std::size_t digits = lx.i;
    while (lx.i < s.size() && digit(s[lx.i])) ++lx.i;

    bool empty = lx.i == digits;
    bool glued = lx.i < s.size() && (std::isalnum((unsigned char)s[lx.i]) || s[lx.i] == '_');
    if (empty || glued) {
        while (lx.i < s.size() && (std::isalnum((unsigned char)s[lx.i]) || s[lx.i] == '_')) ++lx.i;
        return fail(lx, "bad number '" + s.substr(start, lx.i - start) + "'");
    }
    emit(lx, Token::Kind::Number, s.substr(start, lx.i - start));
    return true;
}

Token* lex(const std::string& src, std::string& error) {
    Lexer lx{src, 0, 1, nullptr, nullptr, ""};
    while (lx.i < src.size()) {
        char c = src[lx.i];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++lx.i;
        } else if (c == ';') {
            while (lx.i < src.size() && src[lx.i] != '\n') ++lx.i;
        } else if (c == '\n') {
            emit(lx, Token::Kind::Newline, "\\n");
            ++lx.line;
            ++lx.i;
        } else if (std::isalpha((unsigned char)c) || c == '_') {
            std::size_t start = lx.i;
            while (lx.i < src.size() &&
                   (std::isalnum((unsigned char)src[lx.i]) || src[lx.i] == '_')) {
                ++lx.i;
            }
            emit(lx, Token::Kind::Ident, src.substr(start, lx.i - start));
        } else if (std::isdigit((unsigned char)c)) {
            if (!number(lx)) break;
        } else if (c == ',') { emit(lx, Token::Kind::Comma, ",");    ++lx.i; }
        else if (c == ':')   { emit(lx, Token::Kind::Colon, ":");    ++lx.i; }
        else if (c == '[')   { emit(lx, Token::Kind::LBracket, "["); ++lx.i; }
        else if (c == ']')   { emit(lx, Token::Kind::RBracket, "]"); ++lx.i; }
        else {
            fail(lx, std::string("unexpected character '") + c + "'");
            break;
        }
    }
    if (!lx.error.empty()) {
        destroy(lx.head);
        error = lx.error;
        return nullptr;
    }
    emit(lx, Token::Kind::Eof, "");
    return lx.head;
}
