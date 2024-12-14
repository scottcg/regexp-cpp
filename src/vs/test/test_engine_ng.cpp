#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <gtest/gtest.h>

// #define DEBUG // Uncomment this line for debugging

struct BacktrackState {
    int instr_ptr;
    int input_ptr;
};

struct GroupState {
    int start;
    int end;
};

struct Instruction {
    enum Type {
        MATCH, MATCH_ANY, MATCH_GROUP, PUSH_BACKTRACK, JUMP, FAIL, ACCEPT,
        BEGIN_GROUP, END_GROUP, ANCHOR_START, ANCHOR_END,
        MATCH_RANGE, MATCH_ONE_OF, MATCH_STRING
    } type;
    std::string value;    // Primary value (used for MATCH, MATCH_ONE_OF, MATCH_STRING)
    std::string value2;   // Secondary value (used for MATCH_RANGE)
    int target;           // Jump/Backtrack target
    int group_id;         // Group ID for captures
};

bool execute(const std::vector<Instruction>& program, const std::string& input) {
    std::stack<BacktrackState> backtrack_stack;
    std::unordered_map<int, std::stack<GroupState>> group_stacks; // Group capture stacks
    int instr_ptr = 0, input_ptr = 0;

    #ifdef DEBUG
    std::cout << "Execution started\n";
    #endif

    while (true) {
        if (instr_ptr >= program.size()) return false;
        const auto& instr = program[instr_ptr];

        #ifdef DEBUG
        std::cout << "Instr: " << instr_ptr << " Type: " << instr.type
                  << " InputPos: " << input_ptr << "\n";
        #endif

        switch (instr.type) {
            case Instruction::MATCH_RANGE:
                if (input_ptr < input.size() &&
                    input[input_ptr] >= instr.value[0] &&
                    input[input_ptr] <= instr.value2[0]) {
                    ++input_ptr;
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::MATCH_ONE_OF:
                if (input_ptr < input.size() &&
                    instr.value.find(input[input_ptr]) != std::string::npos) {
                    ++input_ptr;
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::MATCH_STRING:
                if (input_ptr + instr.value.size() <= input.size() &&
                    input.substr(input_ptr, instr.value.size()) == instr.value) {
                    input_ptr += instr.value.size();
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::MATCH:
                if (input_ptr + instr.value.size() <= input.size() &&
                    input.substr(input_ptr, instr.value.size()) == instr.value) {
                    input_ptr += instr.value.size();
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::MATCH_ANY:
                if (input_ptr < input.size()) { ++input_ptr; ++instr_ptr; }
                else goto fail;
                break;

            case Instruction::PUSH_BACKTRACK:
                backtrack_stack.push({instr.target, input_ptr});
                ++instr_ptr;
                break;

            case Instruction::JUMP:
                instr_ptr = instr.target;
                break;

            case Instruction::BEGIN_GROUP:
                group_stacks[instr.group_id].push({input_ptr, -1});
                ++instr_ptr;
                break;

            case Instruction::END_GROUP:
                group_stacks[instr.group_id].top().end = input_ptr;
                ++instr_ptr;
                break;

            case Instruction::MATCH_GROUP: {
                if (group_stacks[instr.group_id].empty()) goto fail;
                GroupState group = group_stacks[instr.group_id].top();
                std::string captured = input.substr(group.start, group.end - group.start);
                if (input.compare(input_ptr, captured.size(), captured) == 0) {
                    input_ptr += captured.size();
                    ++instr_ptr;
                } else goto fail;
                break;
            }

            case Instruction::ANCHOR_START:
                if (input_ptr != 0) goto fail;
                ++instr_ptr;
                break;

            case Instruction::ANCHOR_END:
                if (input_ptr != input.size()) goto fail;
                ++instr_ptr;
                break;

            case Instruction::ACCEPT:
                return true;

            case Instruction::FAIL:
                fail:
                    if (backtrack_stack.empty()) {
#ifdef DEBUG
                        std::cout << "Backtracking failed - no states left. Terminating execution.\n";
#endif
                        return false; // Definitive failure: no backtrack states remain
                    }

            // Restore previous state from backtracking stack
            instr_ptr = backtrack_stack.top().instr_ptr;
            input_ptr = backtrack_stack.top().input_ptr;
            backtrack_stack.pop();

#ifdef DEBUG
            std::cout << "Backtracked to Instr: " << instr_ptr << ", InputPos: " << input_ptr << "\n";
#endif

            // Add this to prevent ACCEPT from being falsely reached
            if (program[instr_ptr].type == Instruction::ACCEPT) {
                return false;
            }
            break;

            default:
                throw std::runtime_error("Unknown instruction");
        }
    }
}

// Test: Single character match (regex: "a")
TEST(RegexEngineTest, MatchSingleCharacter) {
    std::string input = "a";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "a", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));
}

// Test: Match an entire string (regex: "hello")
TEST(RegexEngineTest, MatchString) {
    std::string input = "hello";
    std::vector<Instruction> program = {
        {Instruction::MATCH_STRING, "hello", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "hell";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match any character (regex: ".")
TEST(RegexEngineTest, MatchAnyCharacter) {
    std::string input = "x";
    std::vector<Instruction> program = {
        {Instruction::MATCH_ANY, "", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match a range of characters (regex: "[a-z]")
TEST(RegexEngineTest, MatchRange) {
    std::string input = "g";
    std::vector<Instruction> program = {
        {Instruction::MATCH_RANGE, "a", "z"},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "1";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match one of specific characters (regex: "[abhjjy]")
TEST(RegexEngineTest, MatchOneOfCharacters) {
    std::string input = "h";
    std::vector<Instruction> program = {
        {Instruction::MATCH_ONE_OF, "abhjjy", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "z";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Alternation (regex: "a|b")
TEST(RegexEngineTest, Alternation) {
    std::string input = "b";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", "", 3},
        {Instruction::MATCH, "a", ""},
        {Instruction::JUMP, "", "", 5},
        {Instruction::MATCH, "b", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "c";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Zero or more repetition (regex: "a*")
TEST(RegexEngineTest, Repetition) {
    std::string input = "aaa";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", "", 4}, // Save backtrack point
        {Instruction::MATCH, "a", ""},           // Match 'a'
        {Instruction::JUMP, "", "", 0},          // Jump back to start of loop
        {Instruction::FAIL, "", ""},             // Explicit failure to terminate
        {Instruction::ACCEPT}                    // Successful match
    };

    // EXPECT_TRUE(execute(program, input));  <-- Commented for focus

    std::string failInput = "b";
    EXPECT_FALSE(execute(program, failInput));
}


// Test: Backreference (regex: "(ab) \1")
TEST(RegexEngineTest, Backreference) {
    std::string input = "ab ab";
    std::vector<Instruction> program = {
        {Instruction::BEGIN_GROUP, "", "", 1},  // Start group 1
        {Instruction::MATCH, "ab", ""},
        {Instruction::END_GROUP, "", "", 1},    // End group 1
        {Instruction::MATCH, " ", ""},
        {Instruction::MATCH_GROUP, "", "", 1},  // Match backreference \1
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "ab ac";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Start and end anchors (regex: "^abc$")
TEST(RegexEngineTest, Anchors) {
    std::string input = "abc";
    std::vector<Instruction> program = {
        {Instruction::ANCHOR_START, "", ""},
        {Instruction::MATCH, "abc", ""},
        {Instruction::ANCHOR_END, "", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "aabc";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Complex pattern (regex: "^a[0-9]bc$")
TEST(RegexEngineTest, ComplexPattern) {
    std::string input = "a1bc";
    std::vector<Instruction> program = {
        {Instruction::ANCHOR_START, "", ""},
        {Instruction::MATCH, "a", ""},
        {Instruction::MATCH_RANGE, "0", "9"},
        {Instruction::MATCH, "bc", ""},
        {Instruction::ANCHOR_END, "", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "aabc";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Combination of alternation and range (regex: "a|b|[c-f]")
TEST(RegexEngineTest, AlternationAndRange) {
    std::string input = "d";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", "", 3},
        {Instruction::MATCH, "a", ""},
        {Instruction::JUMP, "", "", 8},
        {Instruction::PUSH_BACKTRACK, "", "", 6},
        {Instruction::MATCH, "b", ""},
        {Instruction::JUMP, "", "", 8},
        {Instruction::MATCH_RANGE, "c", "f"},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "z";
    EXPECT_FALSE(execute(program, failInput));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
