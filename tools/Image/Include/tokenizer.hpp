// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <string>

enum class TokenTypes
{
    String,
    Number,
    Separator
};

struct Token
{
    std::string value;
    TokenTypes type;
};

class Tokenizer
{
public:
    bool Tokenize(const std::string& line, std::vector<Token>& out);
private:
    bool IsNumber(const std::string& str);
};