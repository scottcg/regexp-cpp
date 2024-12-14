#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <gtest/gtest.h>

//#define DEBUG // Uncomment this line for debugging

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
                if (backtrack_stack.empty()) return false;
                instr_ptr = backtrack_stack.top().instr_ptr;
                input_ptr = backtrack_stack.top().input_ptr;
                backtrack_stack.pop();
                break;

            default:
                throw std::runtime_error("Unknown instruction");
        }
    }
}

// Test case 1: Basic concatenation and matching
TEST(RegexEngineTest, ConcatenationAndMatch) {
    std::string input = "hello";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "h"},     // Match 'h'
        {Instruction::MATCH, "e"},     // Match 'e'
        {Instruction::MATCH, "l"},     // Match 'l'
        {Instruction::MATCH, "l"},     // Match 'l'
        {Instruction::MATCH, "o"},     // Match 'o'
        {Instruction::ACCEPT}          // Successful match
    };
    EXPECT_TRUE(execute(program, input));
}

// Test case 2: Alternation (a|b)
TEST(RegexEngineTest, Alternation) {
    std::string input = "b";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", "", 3},  // Save state; try 'a'
        {Instruction::MATCH, "a"},             // Match 'a'
        {Instruction::JUMP, "", "", 5},            // Jump to ACCEPT
        {Instruction::MATCH, "b"},             // Match 'b'
        {Instruction::ACCEPT}                  // Successful match
    };
    EXPECT_TRUE(execute(program, input));
}

// Test case 3: Repetition (a*)
TEST(RegexEngineTest, Repetition) {
    std::string input = "aaa";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", "", 3},  // Save state
        {Instruction::MATCH, "a"},             // Match 'a'
        {Instruction::JUMP, "", "", 0},            // Repeat
        {Instruction::ACCEPT}                  // Successful match
    };
    EXPECT_TRUE(execute(program, input));
}


// Test case 5: Wildcard and character classes
TEST(RegexEngineTest, WildcardAndCharacterClass) {
    std::string input = "xza";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "x"},             // Match 'x'
        {Instruction::MATCH_ANY},              // Match any character
        {Instruction::MATCH, "a"},             // Match 'a'
        {Instruction::ACCEPT}                  // Successful match
    };
    EXPECT_TRUE(execute(program, input));
}

// Test for backreferences with groups
TEST(RegexEngineTest, Backreference) {
    std::string input = "ab ab";
    std::vector<Instruction> program = {
        {Instruction::BEGIN_GROUP, "", 0, 1},  // Start group 1
        {Instruction::MATCH, "ab"},            // Match 'ab'
        {Instruction::END_GROUP, "", 0, 1},    // End group 1
        {Instruction::MATCH, " "},             // Match a space
        {Instruction::MATCH_GROUP, "", 0, 1},  // Match backreference \1
        {Instruction::ACCEPT, "", 0}           // Success
    };
    EXPECT_TRUE(execute(program, input));
}

TEST(RegexEngineTest, Anchors) {
    std::string input = "abc";
    std::vector<Instruction> program = {
        {Instruction::ANCHOR_START},
        {Instruction::MATCH, "abc"},
        {Instruction::ANCHOR_END },
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string input_fail = "aabc";
    EXPECT_FALSE(execute(program, input_fail));
}

TEST(RegexEngineTest, WildcardAndClass) {
    std::string input = "xza";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "x"},
        {Instruction::MATCH_ANY},
        {Instruction::MATCH, "a"},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
