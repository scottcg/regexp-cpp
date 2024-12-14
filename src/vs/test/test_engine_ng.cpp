#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <gtest/gtest.h>

#define DEBUG // Uncomment this line for debugging

struct BacktrackState {
    int instr_ptr;
    int input_ptr;
};

struct Instruction {
    enum Type {
        MATCH, MATCH_ANY, MATCH_GROUP, PUSH_BACKTRACK, JUMP, FAIL, ACCEPT,
        BEGIN_GROUP, END_GROUP, ANCHOR_START, ANCHOR_END
    } type;

    std::string value; // For MATCH and MATCH_GROUP (string, set, or fixed match)
    int target; // Jump/Backtrack target
    int group_id; // Group ID for captures
};

bool execute(const std::vector<Instruction> &program, const std::string &input) {
    std::stack<BacktrackState> backtrack_stack;
    std::unordered_map<int, std::pair<int, int> > group_positions; // Captured group positions
    int instr_ptr = 0, input_ptr = 0;

#ifdef DEBUG
    std::cout << "Execution started.\n";
#endif

    while (true) {
        if (instr_ptr >= program.size()) return false;
        const auto &instr = program[instr_ptr];

#ifdef DEBUG
        std::cout << "Instruction: " << instr_ptr << ", Type: " << instr.type
                << ", Input position: " << input_ptr << "\n";
#endif

        switch (instr.type) {
            case Instruction::ANCHOR_START:
                if (input_ptr != 0) goto fail;
                ++instr_ptr;
                break;

            case Instruction::ANCHOR_END:
                if (input_ptr != input.size()) goto fail;
                ++instr_ptr;
                break;

            case Instruction::MATCH:
                if (input_ptr + instr.value.size() <= input.size() &&
                    input.substr(input_ptr, instr.value.size()) == instr.value) {
                    input_ptr += instr.value.size();
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::MATCH_ANY:
                if (input_ptr < input.size()) {
                    ++input_ptr;
                    ++instr_ptr;
                } else goto fail;
                break;

            case Instruction::PUSH_BACKTRACK:
                backtrack_stack.push({instr.target, input_ptr});
                ++instr_ptr;
                break;

            case Instruction::JUMP:
                instr_ptr = instr.target;
                break;

            case Instruction::BEGIN_GROUP:
                group_positions[instr.group_id] = {input_ptr, -1};
                ++instr_ptr;
                break;

            case Instruction::END_GROUP:
                group_positions[instr.group_id].second = input_ptr;
                ++instr_ptr;
                break;

            case Instruction::MATCH_GROUP: {
                auto &pos = group_positions[instr.group_id];
                if (pos.second == -1) goto fail;
                std::string captured = input.substr(pos.first, pos.second - pos.first);
                if (input.compare(input_ptr, captured.size(), captured) == 0) {
                    input_ptr += captured.size();
                    ++instr_ptr;
                } else goto fail;
                break;
            }

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

TEST(RegexEngineTest, ConcatenationAndMatch) {
    std::string input = "hello";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "hello"},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));
}

TEST(RegexEngineTest, Alternation) {
    std::string input = "b";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", 3},
        {Instruction::MATCH, "a"},
        {Instruction::JUMP, "", 5},
        {Instruction::MATCH, "b"},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));
}

TEST(RegexEngineTest, Repetition) {
    std::string input = "aaa";
    std::vector<Instruction> program = {
        {Instruction::PUSH_BACKTRACK, "", 3},
        {Instruction::MATCH, "a"},
        {Instruction::JUMP, "", 0},
        {Instruction::ACCEPT}
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
