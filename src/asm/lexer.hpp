#pragma once

#include <string>

#include "token.hpp"

Token* lex(const std::string& src, std::string& error);
