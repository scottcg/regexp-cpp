#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <cassert>

namespace nfa {
    enum class op_codes {
        start_match,
        match_char,
        match_char_range,
        not_match_char_range,
        match_string,

        group_start,
        group_end,
        match_backref,

        //transition_goto,
        choice,
        //loop,
        //loop_at_least_once,
        loop_count, // {min,max}, {min,} default of -1 is infinity, ok to start at 0
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
                   + ", next_states=" + std::to_string(next_states.size()) + ", arguments=" + std::to_string(
                       arguments.size());
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

        std::unordered_map<int, int> loop_counter; // Tracks iterations for `loop_count` instructions
        std::unordered_map<int, std::string> back_references; // Track back references to matched strings

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
                    const auto matches = (context.input_index < input.size() && input[context.input_index] >= start &&
                                          input
                                          [context.input_index] <= end);

                    if (matches) {
                        context.input_index++; // Consume character
                        context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                    } else {
                        if (instr.next_states.size() == 1) {
                            break;
                        }
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    }
                } else if (instr.opcode == op_codes::not_match_char_range) {
                    assert(instr.arguments.size() == 2);
                    const auto start = instr.arguments[0].front();
                    const auto end = instr.arguments[1].front();
                    const auto matches = !(context.input_index < input.size() && input[context.input_index] >= start &&
                                           input[context.input_index] <= end);

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
                        if (instr.next_states.size() == 1) {
                            break;
                        }
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
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
                } else if (instr.opcode == op_codes::loop_count) {
                    assert(instr.arguments.size() == 2);
                    const int min = std::stoi(instr.arguments[0]) - 1;
                    int max = std::stoi(instr.arguments[1]) - 1;
                    if (max < -1) max = std::numeric_limits<int>::max();

                    int &current_loop_count = loop_counter[context.instruction_index];

                    if (current_loop_count < max && context.input_index < input.size()) {
                        current_loop_count++;
                        state_stack.push({state_to_index.at(instr.next_states[1]), context.input_index});
                        context.instruction_index = state_to_index.at(instr.next_states[0]);
                    } else if (current_loop_count >= min) {
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    } else {
                        break;
                    }
                } else if (instr.opcode == op_codes::choice) {
                    // Push all alternatives except the first onto the stack to try later
                    for (size_t i = 1; i < instr.next_states.size(); ++i) {
                        state_stack.push({state_to_index.at(instr.next_states[i]), context.input_index});
                    }
                    // Continue with the first alternative
                    context.instruction_index = state_to_index.at(instr.next_states[0]);
                } else if (instr.opcode == op_codes::group_start) {
                    const int id = std::stoi(instr.arguments[0]);
                    // save the input location to a map
                    back_references[id] = "";
                    break;
                } else if (instr.opcode == op_codes::group_end) {
                    const int id = std::stoi(instr.arguments[0]);
                    back_references[id] = "test";
                    // check length from the map and get difference and put the test in backref map
                    // going to need backref id.. parameters i guess
                    break;
                } else if (instr.opcode == op_codes::match_backref) {
                    const int id = std::stoi(instr.arguments[0]);
                    auto value = back_references[id];
                    // compare value to input buffer
                    // grab backref value and do a string compare (check lengths of the string)
                    break;
                } else if (instr.opcode == op_codes::end_match) {
                    if (context.input_index == input.size()) return true; // Successfully matched entire input
                    break;
                } else if (instr.opcode == op_codes::end_no_match) {
                    return false;
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
}

using namespace nfa;

TEST(test_engine, MathWuthBackreference) {
    // ([a-z]+) \1
    const std::vector<nfa_instruction> nfa2 {
        {"start", op_codes::start_match, {"group_start"}, {}},
        {"group_start", op_codes::group_start, {"match_chars"}, {"1"}},
        {"match_chars", op_codes::match_char_range, {"match_sequence", "end_match"}, {"a", "z"}},
        {"match_sequence", op_codes::loop_count, {"match_range", "group_end"}, {"1", "-1"}},
        {"group_end", op_codes::group_end, {"match_space"}, {"1"}},
        {"match_space", op_codes::match_char, {"match_backref", "end_match"}, {" "}},
        {"match_backref", op_codes::match_backref, {"end_match"}, {"1"}},
        {"end_match", op_codes::end_match, {}, {}},
    };

    // ([a-z]+ )
    const std::vector<nfa_instruction> nfa {
            {"start", op_codes::start_match, {"match_chars"}, {}},
            {"match_chars", op_codes::match_char_range, {"match_sequence", "end_match"}, {"a", "z"}},
            {"match_sequence", op_codes::loop_count, {"match_chars", "match_space"}, {"1", "-1"}},
            {"match_space", op_codes::match_char, {"end_match", "end_no_match"}, {" "}},
            {"end_match", op_codes::end_match, {}, {}},
            {"end_no_match", op_codes::end_no_match, {}, {}}
        };

    EXPECT_TRUE(execute_nfa(nfa, "thethe ")); // 2 matches
    EXPECT_FALSE(execute_nfa(nfa, "the dog the")); // Less than 2 matches
}

TEST(test_engine, MatchDogCatWithIntervals) {
    // (dog|cat){2,4}
    std::vector<nfa_instruction> nfa {
        {"start", op_codes::start_match, {"group_start"}, {}},
        {"group_start", op_codes::choice, {"match_dog", "match_cat"}, {}},
        {"match_dog", op_codes::match_string, {"group_end"}, {"dog"}},
        {"match_cat", op_codes::match_string, {"group_end"}, {"cat"}},
        {"group_end", op_codes::loop_count, {"group_start", "end_match"}, {"2", "4"}},
        {"end_match", op_codes::end_match, {}, {}}
    };

    EXPECT_TRUE(execute_nfa(nfa, "dogdog")); // 2 matches
    EXPECT_TRUE(execute_nfa(nfa, "dogcatdog")); // 3 matches
    EXPECT_TRUE(execute_nfa(nfa, "catcatdogcat")); // 4 matches
    EXPECT_FALSE(execute_nfa(nfa, "dog")); // Less than 2 matches
    EXPECT_FALSE(execute_nfa(nfa, "dogcatdogcatdog")); // More than 4 matches
}

TEST(test_engine, MatchAlternate) {
    // a|d
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"choice"}, {}},
        {"choice", op_codes::choice, {"match_a", "match_d"}, {}},
        {"match_a", op_codes::match_char, {"end_match", "match_d"}, {"a"}},
        {"match_d", op_codes::match_char, {"end_match", "unmatched"}, {"d"}},
        {"unmatched", op_codes::unmatched, {}, {}},
        {"end_match", op_codes::end_match, {}, {}}
    };

    EXPECT_TRUE(execute_nfa(nfa, "a"));
    EXPECT_TRUE(execute_nfa(nfa, "d"));
    EXPECT_FALSE(execute_nfa(nfa, "dd"));
}

TEST(test_engine, MatchRange) {
    // [s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::match_char_range, {"end_match", "end_match"}, {"s", "w"}},
        {"end_match", op_codes::end_match, {}, {}},
    };
    ASSERT_TRUE(execute_nfa(nfa, "s"));
    ASSERT_FALSE(execute_nfa(nfa, "ss"));
    ASSERT_FALSE(execute_nfa(nfa, "a"));
    ASSERT_FALSE(execute_nfa(nfa, "aa"));
}

TEST(test_engine, MatchRangeNot) {
    // [^s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::not_match_char_range, {"end_match", "end_match"}, {"s", "w"}},
        {"end_match", op_codes::end_match, {}, {}},
    };
    ASSERT_FALSE(execute_nfa(nfa, "s"));
    ASSERT_TRUE(execute_nfa(nfa, "a"));
}

TEST(test_engine, MatchRangeCount) {
    // [a-z]+
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"group_start"}, {}},
        {"group_start", op_codes::match_char_range, {"group_end", "end_match"}, {"a", "z"}},
        {"group_end", op_codes::loop_count, {"group_start", "end_match"}, {"1", "20"}},
        {"unmatached", op_codes::unmatched, {}, {}},
        {"end_match", op_codes::end_match, {}, {}},
    };
    ASSERT_TRUE(execute_nfa(nfa, "s"));
    ASSERT_TRUE(execute_nfa(nfa, "ss"));
    ASSERT_TRUE(execute_nfa(nfa, "sss"));
    ASSERT_FALSE(execute_nfa(nfa, "9"));
}

TEST(test_engine, MatchRangeCount2) {
    // [a-z]*
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_count", op_codes::loop_count, {"match_range_c"}, {"0", "-1"}},
        {"match_range_c", op_codes::match_char_range, {"match_count", "end_match"}, {"a", "z"}},
        {"end_match", op_codes::matched, {}, {}},
        {"unmatached", op_codes::end_match, {}, {}}
    };
}

TEST(test_engine, MatchString) {
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start_match, {"match_range"}, {}},
        {"match_range", op_codes::match_string, {"end_match"}, {"text"}},
        {"end_match", op_codes::end_match, {}, {}},
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

    EXPECT_TRUE(execute_nfa(nfa, "abc"));
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

    EXPECT_TRUE(execute_nfa(nfa, "ab"));
    EXPECT_TRUE(execute_nfa(nfa, "s"));
    EXPECT_FALSE(execute_nfa(nfa, "T"));
    EXPECT_TRUE(execute_nfa(nfa, "ss"));
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
