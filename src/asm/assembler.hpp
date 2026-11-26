#pragma once

#include <string>

#include "../memory.hpp"
#include "token.hpp"

bool assemble(const Token* head, Memory& mem, std::string& error, bool listing);
