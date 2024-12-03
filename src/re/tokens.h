#pragma once

namespace re {
	constexpr int MAX_BACKREFS = 256;

    enum tokens : unsigned int {
        TOK_END = 0xE00,
        TOK_CHAR = 0xE01,
        TOK_CTRL_CHAR = 0xE02,
        TOK_ESCAPE = 0xE03,
        TOK_REGISTER = 0xE04,
        TOK_BACKREF = 0xE05,
        TOK_EXT_REGISTER = 0xE06
    };


    constexpr int SYNTAX_ERROR = -1;
    constexpr int BACKREFERENCE_OVERFLOW = -2;
    constexpr int EXPRESSION_TOO_LONG = -3;
    constexpr int ILLEGAL_BACKREFERENCE = -4;
    constexpr int ILLEGAL_CLOSURE = -5;
    constexpr int ILLEGAL_DELIMITER = -6;
    constexpr int ILLEGAL_OPERATOR = -7;
    constexpr int ILLEGAL_NUMBER = -8;
    constexpr int MISMATCHED_BRACES = -9;
    constexpr int MISMATCHED_BRACKETS = -10;
    constexpr int MISMATCHED_PARENTHESIS = -11;
}