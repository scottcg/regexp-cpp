#include <concepts>
#include <locale>
#include <cstring>
#include <cwchar>
#include <string>
#include <tuple>
#include <iostream>

#include "tokens.h"

namespace re {
    enum opcodes {
        OP_END, // end of compiled operators.
        OP_NOOP, // no op; does nothing.
        OP_BACKUP, // backup the current source character.
        OP_FORWARD, // forward to the next source character.

        OP_BEGIN_OF_LINE, // match to beginning of line.
        OP_END_OF_LINE, // match to end of line.

        OP_STRING, // match a string; string chars follow (+null)
        OP_BIN_CHAR, // match a "binary" character (follows); usually constants.
        OP_NOT_BIN_CHAR, // not (OP_BIN_CHAR).
        OP_ANY_CHAR, // matches any single character (not a newline).
        OP_CHAR, // match character (char follows), caseless if requested.
        OP_NOT_CHAR, // does not match char (char follows), caseless if requested.
        OP_RANGE_CHAR, // match a range of chars (two chars follow), caseless if requested.
        OP_NOT_RANGE_CHAR, // not(OP_RANGE_CHAR).

        OP_BACKREF_BEGIN, // a backref starts (followed by a backref number).
        OP_BACKREF_END, // ends a backref address (followed by a backref number).
        OP_BACKREF, // match a duplicate of backref contents (backref follows).
        OP_BACKREF_FAIL, // check prior backref after failed alternate ex. ((a)|(b))

        OP_EXT_BEGIN, // like a backref, that is "grouping ()", not in backref list (n follows)
        OP_EXT_END, // closes an extension (n follows)
        OP_EXT, // match a duplicate of extension contexts (n follows)
        OP_NOT_EXT, // not (OP_EXT), (n follows)

        OP_GOTO, // followed by 2 bytes (lsb, msb) of displacement.
        OP_PUSH_FAILURE, // jump to address on failure.
        OP_PUSH_FAILURE2, // i'm completely out of control
        OP_POP_FAILURE, // pop the last failure off the stack
        OP_POP_FAILURE_GOTO, // combination of OP_GOTO and OP_POP_FAILURE
        OP_FAKE_FAILURE_GOTO, // push a dummy failure point and jump.

        OP_CLOSURE, // push a min,max pair for {} counted matching.
        OP_CLOSURE_INC, // add a completed match and loop if more to match.
        OP_TEST_CLOSURE, // test min,max pair and jump or fail.

        OP_BEGIN_OF_BUFFER, // match at beginning of buffer.
        OP_END_OF_BUFFER, // match at end of buffer.

        OP_BEGIN_OF_WORD, // match at the beginning of a word.
        OP_END_OF_WORD, // match at the end of a word.

        OP_DIGIT, // match using isdigit (one byte follow != 0 for complement)
        OP_SPACE, // match using isspace (one byte follow != 0 for complement)
        OP_WORD, // match using isalnum (one byte follow != 0 for complement)
        OP_WORD_BOUNDARY, // match a word boundary (one byte follow != 0 for complement)

        OP_CASELESS, // turn on caseless compares
        OP_NO_CASELESS, // turn off caseless compares
        OP_LCASELESS, // turn on lower caseless compares
        OP_NO_LCASELESS, // turn off lower caseless compares

        OP_ASSERT, // assert following expression at current point.
        OP_SPLIT,
        OP_JUMP,
        OP_ANY,
        OP_FAIL,
        OP_REPEAT_START,
        OP_REPEAT_CHECK,
        OP_MATCH,
        OP_LOOP_START,
        OP_LOOP_END,
        OP_GROUP_START,
        OP_GROUP_END,
        OP_RECURSE,
    };

    template<class T>
    struct char_traits : std::char_traits<T> {
        typedef T char_type;
        typedef typename std::char_traits<T>::int_type int_type;
        typedef std::basic_string<T> string_type;
    };

    template<>
    struct char_traits<char> : std::char_traits<char> {
        using traits_type = std::char_traits<char>;
        typedef std::basic_string<char> string_type;

        static size_t length(const char_type *x) { return traits_type::length(x); }

        static int isalpha(const int_type c) { return std::isalpha(c); }
        static int isalnum(const int_type c) { return std::isalnum(c); }
        static int isspace(const int_type c) { return std::isspace(c); }
        static int isdigit(const int_type c) { return std::isdigit(c); }
        static int islower(const int_type c) { return std::islower(c); }
        static int toupper(const int_type c) { return std::toupper(c); }
        static int tolower(const int_type c) { return std::tolower(c); }
    };

    struct instruction {
        int op; // Opcode (e.g., OP_CHAR, OP_RANGE_CHAR, etc.)
        int arg1; // First argument (e.g., character, range start, jump target, etc.)
        int arg2; // Second argument (e.g., range end, repetition max, etc.)

        instruction(int opcode) : op(opcode), arg1(0), arg2(0) {
        }

        instruction(int opcode, int argument1) : op(opcode), arg1(argument1), arg2(0) {
        }

        instruction(int opcode, int argument1, int argument2)
            : op(opcode), arg1(argument1), arg2(argument2) {
        }
    };

    template<class traitsType>
    class expression_engine {
    public:
        typedef typename traitsType::char_type char_type;
        typedef typename traitsType::int_type int_type;

        bool exec(const std::vector<instruction> &program, const std::string &input) {
            unsigned int pc = 0; // Program counter
            unsigned int sp = 0; // Input pointer
            std::stack<std::pair<int, int> > failure_stack; // For backtracking
            std::stack<std::pair<int, int> > repeat_stack; // For repetition counters
            std::unordered_map<int, std::string> groups; // Captured groups

            while (true) {
                if (pc >= program.size()) {
                    throw std::runtime_error("Program counter out of bounds");
                }
                const auto &instr = program[pc];

                switch (instr.op) {
                    case OP_END:
                        return true; // End of program, successful match

                    case OP_MATCH:
                        return true; // Successful match

                    case OP_FAIL:
                        goto fail; // Explicit failure state

                    case OP_CHAR:
                        if (sp < input.size() && input[sp] == instr.arg1) {
                            sp++;
                            pc++;
                        } else {
                            goto fail;
                        }
                        break;

                    case OP_RANGE_CHAR:
                        if (sp < input.size() && input[sp] >= instr.arg1 && input[sp] <= instr.arg2) {
                            sp++;
                            pc++;
                        } else {
                            goto fail;
                        }
                        break;

                    case OP_ANY:
                        if (sp < input.size()) {
                            sp++;
                            pc++;
                        } else {
                            goto fail;
                        }
                        break;

                    case OP_JUMP:
                        pc = instr.arg1; // Unconditional jump
                        break;

                    case OP_SPLIT:
                        // Push alternate branch onto failure stack
                        failure_stack.push({instr.arg2, sp});
                        pc = instr.arg1; // Take primary branch
                        break;

                    case OP_PUSH_FAILURE:
                        // Save current state for backtracking
                        failure_stack.push({pc + 1, sp});
                        pc = instr.arg1; // Jump to target
                        break;

                    case OP_POP_FAILURE:
                        if (failure_stack.empty()) return false; // No more backtracking
                        pc = failure_stack.top().first;
                        sp = failure_stack.top().second;
                        failure_stack.pop();
                        break;

                    case OP_REPEAT_START:
                        // Initialize repetition counter with min/max bounds
                        repeat_stack.push({0, instr.arg2});
                        pc++;
                        break;

                    case OP_REPEAT_CHECK: {
                        auto &[counter, max] = repeat_stack.top();
                        counter++;
                        if (counter < instr.arg1) {
                            // Minimum repetitions not yet reached
                            pc = instr.arg2; // Loop back
                        } else if (max != -1 && counter >= max) {
                            // Maximum repetitions reached
                            repeat_stack.pop();
                            pc++;
                        } else {
                            failure_stack.push({pc + 1, sp});
                            pc = instr.arg2; // Continue repetition
                        }
                        break;
                    }

                    case OP_LOOP_START:
                        // Push the current program counter to mark the loop start
                        failure_stack.push({pc + 1, sp});
                        pc++;
                        break;

                    case OP_LOOP_END:
                        // Loop back to the loop start
                        if (failure_stack.empty()) {
                            throw std::runtime_error("Loop end without matching start");
                        }
                        pc = failure_stack.top().first;
                        break;

                    case OP_ASSERT:
                        // Handle start-of-line (^) and end-of-line ($) assertions
                        if ((instr.arg1 == '^' && sp != 0) || (instr.arg1 == '$' && sp != input.size())) {
                            goto fail;
                        }
                        pc++;
                        break;

                    case OP_GROUP_START:
                        // Save group start position
                        failure_stack.push({pc + 1, sp});
                        pc++;
                        break;

                    case OP_GROUP_END: {
                        if (failure_stack.empty()) throw std::runtime_error("Group end without matching start");
                        auto [prev_pc, prev_sp] = failure_stack.top();
                        failure_stack.pop();
                        groups[instr.arg1] = input.substr(prev_sp, sp - prev_sp); // Capture group
                        pc++;
                        break;
                    }

                    default:
                        throw std::runtime_error("Unknown opcode");
                }
                continue;

            fail:
                if (failure_stack.empty()) return false; // No more failure points
                pc = failure_stack.top().first; // Restore program counter
                sp = failure_stack.top().second; // Restore input pointer
                failure_stack.pop();
            }
        }
    };

void dump_instructions(const std::vector<instruction>& program, const std::string& name) {
    std::cout << "std::vector<instruction> " << name << " = {\n";
    for (const auto& instr : program) {
        std::cout << "    {";
        switch (instr.op) {
            case OP_END: std::cout << "OP_END"; break;
            case OP_NOOP: std::cout << "OP_NOOP"; break;
            case OP_BACKUP: std::cout << "OP_BACKUP"; break;
            case OP_FORWARD: std::cout << "OP_FORWARD"; break;
            case OP_BEGIN_OF_LINE: std::cout << "OP_BEGIN_OF_LINE"; break;
            case OP_END_OF_LINE: std::cout << "OP_END_OF_LINE"; break;
            case OP_STRING: std::cout << "OP_STRING"; break;
            case OP_BIN_CHAR: std::cout << "OP_BIN_CHAR"; break;
            case OP_NOT_BIN_CHAR: std::cout << "OP_NOT_BIN_CHAR"; break;
            case OP_ANY_CHAR: std::cout << "OP_ANY_CHAR"; break;
            case OP_CHAR: std::cout << "OP_CHAR"; break;
            case OP_NOT_CHAR: std::cout << "OP_NOT_CHAR"; break;
            case OP_RANGE_CHAR: std::cout << "OP_RANGE_CHAR"; break;
            case OP_NOT_RANGE_CHAR: std::cout << "OP_NOT_RANGE_CHAR"; break;
            case OP_BACKREF_BEGIN: std::cout << "OP_BACKREF_BEGIN"; break;
            case OP_BACKREF_END: std::cout << "OP_BACKREF_END"; break;
            case OP_BACKREF: std::cout << "OP_BACKREF"; break;
            case OP_BACKREF_FAIL: std::cout << "OP_BACKREF_FAIL"; break;
            case OP_EXT_BEGIN: std::cout << "OP_EXT_BEGIN"; break;
            case OP_EXT_END: std::cout << "OP_EXT_END"; break;
            case OP_EXT: std::cout << "OP_EXT"; break;
            case OP_NOT_EXT: std::cout << "OP_NOT_EXT"; break;
            case OP_GOTO: std::cout << "OP_GOTO"; break;
            case OP_PUSH_FAILURE: std::cout << "OP_PUSH_FAILURE"; break;
            case OP_PUSH_FAILURE2: std::cout << "OP_PUSH_FAILURE2"; break;
            case OP_POP_FAILURE: std::cout << "OP_POP_FAILURE"; break;
            case OP_POP_FAILURE_GOTO: std::cout << "OP_POP_FAILURE_GOTO"; break;
            case OP_FAKE_FAILURE_GOTO: std::cout << "OP_FAKE_FAILURE_GOTO"; break;
            case OP_CLOSURE: std::cout << "OP_CLOSURE"; break;
            case OP_CLOSURE_INC: std::cout << "OP_CLOSURE_INC"; break;
            case OP_TEST_CLOSURE: std::cout << "OP_TEST_CLOSURE"; break;
            case OP_BEGIN_OF_BUFFER: std::cout << "OP_BEGIN_OF_BUFFER"; break;
            case OP_END_OF_BUFFER: std::cout << "OP_END_OF_BUFFER"; break;
            case OP_BEGIN_OF_WORD: std::cout << "OP_BEGIN_OF_WORD"; break;
            case OP_END_OF_WORD: std::cout << "OP_END_OF_WORD"; break;
            case OP_DIGIT: std::cout << "OP_DIGIT"; break;
            case OP_SPACE: std::cout << "OP_SPACE"; break;
            case OP_WORD: std::cout << "OP_WORD"; break;
            case OP_WORD_BOUNDARY: std::cout << "OP_WORD_BOUNDARY"; break;
            case OP_CASELESS: std::cout << "OP_CASELESS"; break;
            case OP_NO_CASELESS: std::cout << "OP_NO_CASELESS"; break;
            case OP_LCASELESS: std::cout << "OP_LCASELESS"; break;
            case OP_NO_LCASELESS: std::cout << "OP_NO_LCASELESS"; break;
            case OP_ASSERT: std::cout << "OP_ASSERT"; break;
            case OP_SPLIT: std::cout << "OP_SPLIT"; break;
            case OP_JUMP: std::cout << "OP_JUMP"; break;
            case OP_ANY: std::cout << "OP_ANY"; break;
            case OP_FAIL: std::cout << "OP_FAIL"; break;
            case OP_REPEAT_START: std::cout << "OP_REPEAT_START"; break;
            case OP_REPEAT_CHECK: std::cout << "OP_REPEAT_CHECK"; break;
            case OP_MATCH: std::cout << "OP_MATCH"; break;
            case OP_LOOP_START: std::cout << "OP_LOOP_START"; break;
            case OP_LOOP_END: std::cout << "OP_LOOP_END"; break;
            case OP_GROUP_START: std::cout << "OP_GROUP_START"; break;
            case OP_GROUP_END: std::cout << "OP_GROUP_END"; break;
            default: std::cout << instr.op; break;
        }
        if (instr.arg1 != 0 || instr.arg2 != 0) {
            std::cout << ", " << instr.arg1;
        }
        if (instr.arg2 != 0) {
            std::cout << ", " << instr.arg2;
        }
        std::cout << "},\n";
    }
    std::cout << "};\n";
}
};



using namespace re;

int main() {
    using traits_type = char_traits<char>;
    expression_engine<traits_type> engine;

    // https://colauttilab.github.io/PythonCrashCourse/2_regex.html#1_overview

    // Compiled Opcodes for \w{1,3}
    std::vector<instruction> program1 = {
        {OP_REPEAT_START, 1, 3}, // Start repetition, min = 1, max = 3
        {OP_PUSH_FAILURE, 13}, // Push failure state for backtracking
        {OP_RANGE_CHAR, 'a', 'z'}, // Match 'a-z'
        {OP_JUMP, 6}, // Skip to repeat check
        {OP_RANGE_CHAR, 'A', 'Z'}, // Match 'A-Z'
        {OP_JUMP, 6}, // Skip to repeat check
        {OP_RANGE_CHAR, '0', '9'}, // Match '0-9'
        {OP_JUMP, 6}, // Skip to repeat check
        {OP_CHAR, '_'}, // Match '_'
        {OP_REPEAT_CHECK}, // Check repetition bounds
        {OP_POP_FAILURE}, // Exit loop if no match
        {OP_FAIL}, // Failure state
        {OP_MATCH}, // Successful match
        {OP_END}, // End of program
    };
    auto r = engine.exec(program1, "the"); // true
    std::cout << "result1 = " << r << std::endl;

    // Compiled Opcodes for [a-zA-Z0-9_]+ (equivalent to \w+)
    std::vector<instruction> program2 = {
        {OP_LOOP_START}, // Start of the loop
        {OP_PUSH_FAILURE, 13}, // Push failure point (exit loop)
        {OP_RANGE_CHAR, 'a', 'z'}, // Match 'a-z'
        {OP_JUMP, 0}, // Loop back to check next character
        {OP_RANGE_CHAR, 'A', 'Z'}, // Match 'A-Z'
        {OP_JUMP, 0}, // Loop back to check next character
        {OP_RANGE_CHAR, '0', '9'}, // Match '0-9'
        {OP_JUMP, 0}, // Loop back to check next character
        {OP_CHAR, '_'}, // Match '_'
        {OP_JUMP, 0}, // Loop back to check next character
        {OP_POP_FAILURE}, // Exit loop if no match
        {OP_LOOP_END}, // End of the loop
        {OP_MATCH}, // Successful match
        {OP_END}, // End of program
    };
    r = engine.exec(program2, "abc123_def"); // true
    std::cout << "result2 = " << r << std::endl;

    // Compiled Opcodes for  (a|b)*c+d?e{2,3}
    std::vector<instruction> program3 = {
        // (a|b)*
        {OP_LOOP_START}, // Start of loop
        {OP_SPLIT, 3, 6}, // Fork: try 'a' or 'b'
        {OP_CHAR, 'a'}, // Match 'a'
        {OP_JUMP, 0}, // Loop back
        {OP_CHAR, 'b'}, // Match 'b'
        {OP_JUMP, 0}, // Loop back
        {OP_POP_FAILURE}, // Exit loop if no match

        // c+
        {OP_REPEAT_START, 1, -1}, // Start repetition (min 1, max unlimited)
        {OP_CHAR, 'c'}, // Match 'c'
        {OP_REPEAT_CHECK}, // Check repetition bounds

        // d?
        {OP_SPLIT, 12, 14}, // Fork: match 'd' or skip
        {OP_CHAR, 'd'}, // Match 'd'

        // e{2,3}
        {OP_REPEAT_START, 2, 3}, // Start repetition (min 2, max 3)
        {OP_CHAR, 'e'}, // Match 'e'
        {OP_REPEAT_CHECK}, // Check repetition bounds

        // End of program
        {OP_MATCH}, // Successful match
        {OP_END} // End of program
    };
    r = engine.exec(program2, "acee"); // true
    std::cout << "result3 = " << r << std::endl;

    // Compiled Opcodes for  ^(abc|def)$
    std::vector<instruction> program4 = {
        // ^
        {OP_ASSERT, '^'}, // Assert start of input

        // abc|def
        {OP_SPLIT, 3, 8}, // Fork: try "abc" or "def"
        {OP_CHAR, 'a'}, // Match 'a'
        {OP_CHAR, 'b'}, // Match 'b'
        {OP_CHAR, 'c'}, // Match 'c'
        {OP_JUMP, 12}, // Skip "def" branch
        {OP_CHAR, 'd'}, // Match 'd'
        {OP_CHAR, 'e'}, // Match 'e'
        {OP_CHAR, 'f'}, // Match 'f'

        // $
        {OP_ASSERT, '$'}, // Assert end of input

        // End of program
        {OP_MATCH}, // Successful match
        {OP_END} // End of program
    };
    r = engine.exec(program2, "abc"); // true
    std::cout << "result4 = " << r << std::endl;

    r = engine.exec(program2, "def"); // true
    std::cout << "result4 = " << r << std::endl;

    // Compiled Opcodes for ".*(\w\w+).*"
    std::vector<instruction> program5 = {
        // .*
        {OP_LOOP_START}, // Start loop for '.*'
        {OP_PUSH_FAILURE, 6}, // Push failure point
        {OP_ANY}, // Match any character
        {OP_JUMP, 0}, // Loop back
        {OP_POP_FAILURE}, // Exit loop if no match

        // (\w\w+)
        {OP_GROUP_START, 1}, // Start capture group 1
        {OP_RANGE_CHAR, 'a', 'z'}, // Match first word character
        {OP_RANGE_CHAR, 'a', 'z'}, // Match second word character
        {OP_REPEAT_START, 1, -1}, // Match additional word characters (min 1, max unlimited)
        {OP_RANGE_CHAR, 'a', 'z'}, // Match repeated word characters
        {OP_REPEAT_CHECK}, // Check repetition bounds
        {OP_GROUP_END, 1}, // End capture group 1

        // .*
        {OP_LOOP_START}, // Start loop for trailing '.*'
        {OP_PUSH_FAILURE, 17}, // Push failure point
        {OP_ANY}, // Match any character
        {OP_JUMP, 14}, // Loop back
        {OP_POP_FAILURE}, // Exit loop if no match

        // End of program
        {OP_MATCH}, // Successful match
        {OP_END} // End of program
    };
    r = engine.exec(program5, "abc123def"); // true
    std::cout << "result5 = " << r << std::endl;

    // Compiled Opcodes for "[which]"
    std::vector<instruction> program6 = {
        {OP_SPLIT, 2, 4}, // Try 'w' or move to 'h'
        {OP_CHAR, 'w'}, // Match 'w'
        {OP_MATCH}, // Successful match
        {OP_SPLIT, 5, 7}, // Try 'h' or move to 'i'
        {OP_CHAR, 'h'}, // Match 'h'
        {OP_MATCH}, // Successful match
        {OP_SPLIT, 8, 10}, // Try 'i' or move to 'c'
        {OP_CHAR, 'i'}, // Match 'i'
        {OP_MATCH}, // Successful match
        {OP_SPLIT, 11, 13}, // Try 'c' or move to 'h'
        {OP_CHAR, 'c'}, // Match 'c'
        {OP_MATCH}, // Successful match
        {OP_CHAR, 'h'}, // Match 'h'
        {OP_MATCH}, // Successful match
        {OP_END} // End of program
    };
    r = engine.exec(program6, "c"); // true
    std::cout << "result6 = " << r << std::endl;


    /*
    *text = "(a(b)c)d"
pattern = r"\(([^()]*|(?R))*+\)"  # Matches nested parentheses
matches = re.findall(pattern, text)
print(matches)  # Output: ['(a(b)c)', '(b)']
     */
    std::vector<instruction> nested_parentheses_program = {
        {OP_CHAR, '('},                        // Match '('
        {OP_LOOP_START},                       // Start loop for nested content
            {OP_SPLIT, 3, 5},                  // Try `[^()]*` or `(?R)`
            {OP_NOT_RANGE_CHAR, '(', ')'},     // Match `[^()]`
            {OP_JUMP, 1},                      // Loop back for more characters
            {OP_RECURSE},                      // Recursive call for nested parentheses
            {OP_JUMP, 1},                      // Loop back for more characters
        {OP_POP_FAILURE},                      // Exit loop if no match
        {OP_CHAR, ')'},                        // Match ')'
        {OP_MATCH},                            // Successful match
        {OP_END}                               // End of program
    };
}
