#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <cassert>

enum class op_codes {
    start_match,
    match_char,
    match_char_range,
    not_match_char_range,
    match_string,
    transition_goto,
    choice,
    loop,
    loop_at_least_once,
    loop_count,
    matched,
    unmatched,
    end_match,
    end_no_match
};

struct nfa_instruction {
    std::string state_id;
    op_codes opcode;
    std::vector<std::string> next_states; // Possible states to transition to often: 0=success, 1=failure
    std::vector<std::string> arguments; // Optional arguments, typically character ranges or characters

    std::string to_string() const {
        return "state_id=" + state_id + ", opcode=" + std::to_string(static_cast<int>(opcode))
        + ", next_states=" + std::to_string(next_states.size()) + ", arguments=" + std::to_string(arguments.size());
    }
};

struct state_context {
    int instruction_index;
    size_t input_index;
};

bool execute_nfa(const std::vector<nfa_instruction> &instructions,
                 const std::unordered_map<std::string, int> &state_to_index, const std::string &input) {
    std::stack<state_context> state_stack;
    state_stack.push({state_to_index.at("start"), 0});

    while (!state_stack.empty()) {
        state_context context = state_stack.top();
        state_stack.pop();

        while (context.instruction_index < instructions.size() && context.input_index <= input.size()) {
            const nfa_instruction &instr = instructions[context.instruction_index];

            if (instr.opcode == op_codes::start_match) {
                if (context.input_index != 0) break; // Ensure starting at beginning
                context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
            } else if (instr.opcode == op_codes::match_char_range) {
                assert(instr.arguments.size() == 2);
                const auto start = instr.arguments[0].front();
                const auto end = instr.arguments[1].front();
                const auto matches = (context.input_index < input.size() && input[context.input_index] >= start && input[context.input_index] <= end);

                if (matches) {
                    context.input_index++; // Consume character
                    context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                } else {
                    context.instruction_index = state_to_index.at(instr.next_states[1]);
                    //break; // Failed to match, break to try another path if possible
                }
            } else if (instr.opcode == op_codes::not_match_char_range) {
                assert(instr.arguments.size() == 2);
                const auto start = instr.arguments[0].front();
                const auto end = instr.arguments[1].front();
                const auto matches = !(context.input_index < input.size() && input[context.input_index] >= start && input[context.input_index] <= end);

                if (matches) {
                    context.input_index++; // Consume character
                    context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                } else {
                    if (instr.next_states.size() == 1) {
                        break;
                    }
                    context.instruction_index = state_to_index.at(instr.next_states[1]);
                }
            } else if (instr.opcode == op_codes::match_char) {
                const auto match_char = instr.arguments[0].front();
                if (context.input_index < input.size() && (match_char == input[context.input_index])) {
                    context.input_index++; // Consume character
                    context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                } else {
                    break; // Failed to match, break to try another path if possible
                }
            } else if (instr.opcode == op_codes::match_string) {
                const auto match_string = instr.arguments[0];
                if (context.input_index + match_string.size() <= input.size() &&
                    match_string == input.substr(context.input_index, match_string.size())) {
                    context.input_index += match_string.size(); // Consume characters
                    context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                } else {
                    if (instr.next_states.size() == 1) {
                        break;
                    }
                    context.instruction_index = state_to_index.at(instr.next_states[1]);
                }
            } else if (instr.opcode == op_codes::choice) {
                // Push all alternatives except the first onto the stack to try later
                for (size_t i = 1; i < instr.next_states.size(); ++i) {
                    state_stack.push({state_to_index.at(instr.next_states[i]), context.input_index});
                }
                // Continue with the first alternative
                context.instruction_index = state_to_index.at(instr.next_states[0]);
            } else if (instr.opcode == op_codes::end_match) {
                if (context.input_index == input.size()) return true; // Successfully matched entire input
                break;
            } else {
                break; // Unhandled opcode or error state
            }
        }
    }

    return false;
}

bool execute_nfa(const std::vector<nfa_instruction> &instructions, const std::string &input) {
    std::unordered_map<std::string, int> state_to_index;
    for (auto i = 0; i < instructions.size(); ++i) {
        state_to_index[instructions[i].state_id] = i;
    }
    return execute_nfa(instructions, state_to_index, input);
}


TEST(test_engine, MatchAlternate) {
    // a|d
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"choice"}, {}},
        {"choice", op_codes::choice, {"match_a", "match_d"}, {}},
        {"match_a", op_codes::match_char, {"matched", "match_d"}, {"a"}},
        {"match_d", op_codes::match_char, {"matched", "unmatched"}, {"d"}},
        {"matched", op_codes::matched, {}, {}},
        {"unmatched", op_codes::unmatched, {}, {}}
    };
}

TEST(test_engine, MatchRange) {
    // [s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::match_char_range, {"matched", "matched"}, {"s", "w"}},
        {"matched", op_codes::end_match, {}, {}},
    };
    ASSERT_TRUE(execute_nfa(nfa, "s"));
    ASSERT_FALSE(execute_nfa(nfa, "a"));
}

TEST(test_engine, MatchRangeNot) {
    // [^s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::not_match_char_range, {"matched", "matched"}, {"s", "w"}},
        {"matched", op_codes::end_match, {}, {}},
    };
    ASSERT_FALSE(execute_nfa(nfa, "s"));
    ASSERT_TRUE(execute_nfa(nfa, "a"));
}


TEST(test_engine, MatchRangeCount) {
    // [a-z]+
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::match_char_range, {"match_count", "unmatched"}, {"a", "z"}},
        {"match_count", op_codes::loop_count, {"match_range_c"}, {"0", "-1"}},
        {"match_range_c", op_codes::match_char_range, {"match_count", "matched"}, {"a", "z"}},
        {"matched", op_codes::matched, {}, {}},
        {"unmatached", op_codes::unmatched, {}, {}}
    };
}

TEST(test_engine, MatchRangeCount2) {
    // [a-z]*
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_count", op_codes::loop_count, {"match_range_c"}, {"0", "-1"}},
        {"match_range_c", op_codes::match_char_range, {"match_count", "matched"}, {"a", "z"}},
        {"matched", op_codes::matched, {}, {}},
        {"unmatached", op_codes::unmatched, {}, {}}
    };
}

TEST(test_engine, MatchString) {
    const std::vector<nfa_instruction> nfa{
            {"start", op_codes::start_match, {"match_range"}, {}},
            {"match_range", op_codes::match_string, {"matched"}, {"text"}},
            {"matched", op_codes::end_match, {}, {}},
        };
    ASSERT_TRUE(execute_nfa(nfa, "text"));
    ASSERT_FALSE(execute_nfa(nfa, "no-matched"));
}

TEST(test_engine, CompileAndMatch) {
    // ^(abc|[s-z]+)$
    const std::vector<nfa_instruction> nfa = {
        {"start", op_codes::start_match, {"choice"}, {}},
        // group
        {"choice", op_codes::choice, {"match_a", "match_s"}, {}},
        {"match_a", op_codes::match_char, {"match_b"}, {"a"}},
        {"match_b", op_codes::match_char, {"match_c"}, {"b"}},
        {"match_c", op_codes::match_char, {"end_abc"}, {"c"}},
        {"end_abc", op_codes::end_match, {}, {}},
        {"match_s", op_codes::match_char, {"loop_s_to_z"}, {"s"}},
        {"loop_s_to_z", op_codes::match_char, {"end_s_to_z"}, {"z"}},
        // end group
        {"end_s_to_z", op_codes::end_match, {}, {}}
    };

    EXPECT_TRUE(execute_nfa(nfa, "abc"));
    EXPECT_TRUE(execute_nfa(nfa, "sz"));
}

TEST(test_engine, CompileAndMatchRange) {
    // abc|[s-z]+
    const std::vector<nfa_instruction> nfa = {
        {"start", op_codes::start_match, {"choice"}, {}},
        {"choice", op_codes::choice, {"match_a", "match_s"}, {}},
        {"match_a", op_codes::match_char, {"match_b"}, {"a"}},
        {"match_b", op_codes::match_char, {"match_c"}, {"b"}},
        {"match_c", op_codes::match_char, {"end_abc"}, {"c"}},
        {"end_abc", op_codes::end_match, {}, {}},
        {"match_s", op_codes::match_char_range, {"next_s_to_z"}, {"s", "z"}},
        {"next_s_to_z", op_codes::match_char_range, {"next_s_to_z", "end_s_to_z"}, {"s", "z"}},
        {"end_s_to_z", op_codes::end_match, {}, {}}
    };

    const std::string input = "abc";
    const bool match = execute_nfa(nfa, input);
    EXPECT_TRUE(match);
}

TEST(test_engine, CompileAndMatchRange2nd) {
    // ab|[s-z]+
    const std::vector<nfa_instruction> nfa = {
        {"start", op_codes::start_match, {"choice"}, {}},
        {"choice", op_codes::choice, {"match_a", "match_s"}, {}},
        {"match_a", op_codes::match_char, {"match_b"}, {"a"}},
        {"match_b", op_codes::match_char, {"end_match"}, {"b"}},
        {"match_s", op_codes::match_char_range, {"next_s_to_z", "end_match"}, {"s", "z"}},
        {"next_s_to_z", op_codes::match_char_range, {"next_s_to_z", "end_match"}, {"s", "z"}},
        {"end_match", op_codes::end_match, {}, {}}
    };

    const std::string input = "ss";
    const bool match = execute_nfa(nfa, input);
    EXPECT_TRUE(match);
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
