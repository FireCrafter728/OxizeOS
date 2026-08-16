// SPDX-License-Identifier: GPL-3.0-or-later

#include <tokenizer.hpp>

bool Tokenizer::Tokenize(const std::string& line, std::vector<Token>& out)
{
	if(line.empty()) return true;

	// Token separators(exclusive): ' ', '\n', '\r', '\t', '\f', '\v'
	// Token separators(inclusive): '=', '(', ')', ','
	// Anything inside double quotes(exclusive) is passed as a single token.
	// If a # is found, stop parsing, as it means that everything from it to the end of the line has to be ignored

	bool inQuotes = false;
	std::string current = "";
	Token token;

	for(size_t i = 0; i < line.length(); i++)
	{
		char c = line[i];

		switch(c)
		{
			case '#': {
				if(inQuotes) {
					current += c;
					continue;
				}
				if(!current.empty()) {
					token.value = current;
					if(IsNumber(current)) token.type = TokenTypes::Number;
					else token.type = TokenTypes::String;
					out.push_back(token);
				}

				return true;
			}
			case '"':
			{
				if(inQuotes) {
					token.value = current;
					token.type = TokenTypes::String;
					out.push_back(token);
					current.clear();
					inQuotes = false;
					continue;
				} else {
					if(!current.empty()) {
						fprintf(stderr, "Invalid or unfinished token %s before \"\n", current.c_str());
						return false;
					}
					inQuotes = true;
					continue;
				}
			}
			case ' ':
			case '\n':
			case '\r':
			case '\t':
			case '\f':
			case '\v':
			{
				if(inQuotes) {
					current += c;
					continue;
				}
				if(!current.empty()) {
					token.value = current;
					if(IsNumber(current)) token.type = TokenTypes::Number;
					else token.type = TokenTypes::String;
					out.push_back(token);
					current.clear();
					continue;
				}
				continue;
			}
			case '=':
			case '(':
			case ')':
			case ',':
			{
				if(inQuotes) {
					current += c;
					continue;
				}
				if(!current.empty()) {
					token.value = current;
					if(IsNumber(current)) token.type = TokenTypes::Number;
					else token.type = TokenTypes::String;
					out.push_back(token);
					current.clear();
				}
				token.type = TokenTypes::Separator;
				token.value = std::string(1, c);
				out.push_back(token);
				current.clear();
				continue;
			}
			default: {
				current += c;
				break;
			}
		}
	}

	if(inQuotes) {
		fprintf(stderr, "Unfinished quoted string %s\n", current.c_str());
		return false;
	}

	if(!current.empty()) {
		token.value = current;
		if(IsNumber(current)) token.type = TokenTypes::Number;
		else token.type = TokenTypes::String;
		out.push_back(token);
	}

	return true;
}

bool Tokenizer::IsNumber(const std::string& str)
{
	if(str.empty()) return false;

	const char dec[] = "0123456789";
	const char hex[] = "0123456789ABCDEFabcdef";
	const char oct[] = "01234567";

	enum NumTypes {
		Decimal,
		Hexadecimal,
		Octal
	};

	NumTypes numType;

	if(str[0] == '0') {
		if(str.length() < 2) return true; // Value is 0, accepted
		if(str[1] == 'x') {
			if(str.length() < 3) return false;
			numType = Hexadecimal;
		}
		else numType = Octal;
	} else {
		if(str.length() < 1) return false;
		numType = Decimal;
	}
	
	for(size_t i = numType == Hexadecimal ? 2 : (numType == Decimal ? 0 : 1); i < str.length(); i++)
	{
		const char* allow = numType ==  Hexadecimal ? hex : (numType == Decimal ? dec : oct);
		if(strchr(allow, str[i]) == nullptr) return false; // Non-number character
	}

	return true;
}