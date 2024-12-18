#pragma once

namespace re {
	constexpr int MAX_BACKREFS = 256;

    enum tokens : int {
        TOK_END = 0xE00,
        TOK_CHAR = 0xE01,
        TOK_CTRL_CHAR = 0xE02,
        TOK_ESCAPE = 0xE03,
        TOK_REGISTER = 0xE04,
        TOK_BACKREF = 0xE05,
        TOK_EXT_REGISTER = 0xE06
    };

    enum opcodes {
		OP_END,				// end of compiled operators.
		OP_NOOP,			// no op; does nothing.
		OP_BACKUP,			// backup the current source character.
		OP_FORWARD,			// forward to the next source character.

		OP_BEGIN_OF_LINE,	// match to beginning of line.
		OP_END_OF_LINE,		// match to end of line.
		
		OP_STRING,			// match a string; string chars follow (+null)
		OP_BIN_CHAR,		// match a "binary" character (follows); usually constants.
		OP_NOT_BIN_CHAR,	// not (OP_BIN_CHAR).
		OP_ANY_CHAR,		// matches any single character (not a newline).
		OP_CHAR,			// match character (char follows), caseless if requested.
		OP_NOT_CHAR,		// does not match char (char follows), caseless if requested.
		OP_RANGE_CHAR,		// match a range of chars (two chars follow), caseless if requested.
		OP_NOT_RANGE_CHAR,	// not(OP_RANGE_CHAR).

		OP_BACKREF_BEGIN,	// a backref starts (followed by a backref number).
		OP_BACKREF_END,		// ends a backref address (followed by a backref number).
		OP_BACKREF,			// match a duplicate of backref contents (backref follows).
		OP_BACKREF_FAIL,	// check prior backref after failed alternate ex. ((a)|(b))

		OP_EXT_BEGIN,		// like a backref, that is "grouping ()", not in backref list (n follows)
		OP_EXT_END,			// closes an extension (n follows)
		OP_EXT,				// match a duplicate of extension contexts (n follows)
		OP_NOT_EXT,			// not (OP_EXT), (n follows)

		OP_GOTO,			// followed by 2 bytes (lsb, msb) of displacement.
		OP_PUSH_FAILURE,	// jump to address on failure.
		OP_PUSH_FAILURE2,	// i'm completely out of control
		OP_POP_FAILURE,		// pop the last failure off the stack
		OP_POP_FAILURE_GOTO,// combination of OP_GOTO and OP_POP_FAILURE
		OP_FAKE_FAILURE_GOTO,// push a dummy failure point and jump.
		
		OP_CLOSURE,			// push a min,max pair for {} counted matching.
		OP_CLOSURE_INC,		// add a completed match and loop if more to match.
		OP_TEST_CLOSURE,	// test min,max pair and jump or fail.
		
		OP_BEGIN_OF_BUFFER,	// match at beginning of buffer.
		OP_END_OF_BUFFER,	// match at end of buffer.

		OP_BEGIN_OF_WORD,	// match at the beginning of a word.
		OP_END_OF_WORD,		// match at the end of a word.

		OP_DIGIT,			// match using isdigit (one byte follow != 0 for complement)
		OP_SPACE,			// match using isspace (one byte follow != 0 for complement)
		OP_WORD,			// match using isalnum (one byte follow != 0 for complement)
		OP_WORD_BOUNDARY,	// match a word boundary (one byte follow != 0 for complement)

		OP_CASELESS,		// turn on caseless compares
		OP_NO_CASELESS,		// turn off caseless compares
		OP_LCASELESS,		// turn on lower caseless compares
		OP_NO_LCASELESS,	// turn off lower caseless compares
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

	std::string opcode_to_string(int opcode) {
    switch (opcode) {
        case OP_END: return "OP_END";
        case OP_NOOP: return "OP_NOOP";
        case OP_BACKUP: return "OP_BACKUP";
        case OP_FORWARD: return "OP_FORWARD";
        case OP_BEGIN_OF_LINE: return "OP_BEGIN_OF_LINE";
        case OP_END_OF_LINE: return "OP_END_OF_LINE";
        case OP_STRING: return "OP_STRING";
        case OP_BIN_CHAR: return "OP_BIN_CHAR";
        case OP_NOT_BIN_CHAR: return "OP_NOT_BIN_CHAR";
        case OP_ANY_CHAR: return "OP_ANY_CHAR";
        case OP_CHAR: return "OP_CHAR";
        case OP_NOT_CHAR: return "OP_NOT_CHAR";
        case OP_RANGE_CHAR: return "OP_RANGE_CHAR";
        case OP_NOT_RANGE_CHAR: return "OP_NOT_RANGE_CHAR";
        case OP_BACKREF_BEGIN: return "OP_BACKREF_BEGIN";
        case OP_BACKREF_END: return "OP_BACKREF_END";
        case OP_BACKREF: return "OP_BACKREF";
        case OP_BACKREF_FAIL: return "OP_BACKREF_FAIL";
        case OP_EXT_BEGIN: return "OP_EXT_BEGIN";
        case OP_EXT_END: return "OP_EXT_END";
        case OP_EXT: return "OP_EXT";
        case OP_NOT_EXT: return "OP_NOT_EXT";
        case OP_GOTO: return "OP_GOTO";
        case OP_PUSH_FAILURE: return "OP_PUSH_FAILURE";
        case OP_PUSH_FAILURE2: return "OP_PUSH_FAILURE2";
        case OP_POP_FAILURE: return "OP_POP_FAILURE";
        case OP_POP_FAILURE_GOTO: return "OP_POP_FAILURE_GOTO";
        case OP_FAKE_FAILURE_GOTO: return "OP_FAKE_FAILURE_GOTO";
        case OP_CLOSURE: return "OP_CLOSURE";
        case OP_CLOSURE_INC: return "OP_CLOSURE_INC";
        case OP_TEST_CLOSURE: return "OP_TEST_CLOSURE";
        case OP_BEGIN_OF_BUFFER: return "OP_BEGIN_OF_BUFFER";
        case OP_END_OF_BUFFER: return "OP_END_OF_BUFFER";
        case OP_BEGIN_OF_WORD: return "OP_BEGIN_OF_WORD";
        case OP_END_OF_WORD: return "OP_END_OF_WORD";
        case OP_DIGIT: return "OP_DIGIT";
        case OP_SPACE: return "OP_SPACE";
        case OP_WORD: return "OP_WORD";
        case OP_WORD_BOUNDARY: return "OP_WORD_BOUNDARY";
        case OP_CASELESS: return "OP_CASELESS";
        case OP_NO_CASELESS: return "OP_NO_CASELESS";
        case OP_LCASELESS: return "OP_LCASELESS";
        case OP_NO_LCASELESS: return "OP_NO_LCASELESS";
        default: return "UNKNOWN_OPCODE";
    }
}
}