#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <cassert>

namespace enfa {
    enum class op_codes {
        start,
        transition,
        success,
        failure,

        match_char,
        match_char_range,
        not_match_char_range,
        match_string,
        match_any,
        match_repeat,

        group_start,
        group_end,
        match_backref,

        choice,
        loop_count, // {min,max}, {min,} default of -1 is infinity, ok to start at 0

        match_start_of_line, // New: Matches "^"
        match_end_of_line // New: Matches "$"
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
        std::unordered_map<int, int> loop_counter; // Tracks iterations for `loop_count`
    };

    bool match_nfa(const std::vector<nfa_instruction> &instructions,
                     const std::unordered_map<std::string, int> &state_to_index, const std::string &input) {
        std::stack<state_context> state_stack;
        state_stack.push({state_to_index.at("start"), 0});

        std::unordered_map<int, int> loop_counter; // Tracks iterations for `loop_count` instructions
        std::unordered_map<int, std::pair<size_t, size_t> > back_references; // Track back references to matched strings

        while (!state_stack.empty()) {
            state_context context = state_stack.top();
            state_stack.pop();

            while (context.instruction_index < instructions.size() && context.input_index <= input.size()) {
                const nfa_instruction &instr = instructions[context.instruction_index];

                // std::cout << "State: " << instr.state_id << ", Opcode: " << static_cast<int>(instr.opcode)<< ", Input Index: " << context.input_index<< ", Current Input Char: "<< (context.input_index < input.size() ? input[context.input_index] : ' ') << "\n";

                if (instr.opcode == op_codes::start) {
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
                } else if (instr.opcode == op_codes::match_repeat) {
                    assert(instr.arguments.size() == 3);
                    const std::string& subpattern_id = instr.arguments[0];
                    const int min = std::stoi(instr.arguments[1]);
                    int max = std::stoi(instr.arguments[2]);
                    if (max < 0) max = std::numeric_limits<int>::max(); // Treat -1 as infinity

                    int &current_repeat_count = loop_counter[context.instruction_index];

                    if (current_repeat_count < max) {
                        // Push the continuation state onto the stack to continue after the subpattern
                        state_stack.push({state_to_index.at(instr.next_states[1]), context.input_index});

                        // Start the subpattern again
                        context.instruction_index = state_to_index.at(subpattern_id);
                        current_repeat_count++;
                    } else if (current_repeat_count >= min) {
                        // If min repetitions are satisfied, proceed to the next state
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    } else {
                        // Failure case: Not enough repetitions
                        break;
                    }
                } else if (instr.opcode == op_codes::loop_count) {
                    assert(instr.arguments.size() == 2);
                    const int min = std::stoi(instr.arguments[0]) - 1;
                    int max = std::stoi(instr.arguments[1]) - 1;
                    if (max < -1) max = std::numeric_limits<int>::max();

                    int &current_loop_count = loop_counter[context.instruction_index];

                    if (current_loop_count < max && context.input_index < input.size()) {
                        // Non-greedy: Push the continuation state onto the stack first
                        state_stack.push({state_to_index.at(instr.next_states[1]), context.input_index});

                        // Continue with the loop state
                        current_loop_count++;
                        context.instruction_index = state_to_index.at(instr.next_states[0]);
                    } else if (current_loop_count >= min) {
                        // If min repetitions are satisfied, proceed to the next state
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    } else {
                        break; // Failure case: not enough repetitions
                    }
                } else if (instr.opcode == op_codes::match_any) {
                    if (context.input_index < input.size()) {
                        context.input_index++; // Consume any single character
                        context.instruction_index = state_to_index.at(instr.next_states[0]); // Move to next state
                    } else {
                        if (instr.next_states.size() == 1) {
                            break; // Failure case
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
                } else if (instr.opcode == op_codes::group_start) {
                    // Store the start index for this group
                    const int id = std::stoi(instr.arguments[0]);
                    back_references[id] = {context.input_index, context.input_index}; // Initialize range (start=start)
                    context.instruction_index = state_to_index.at(instr.next_states[0]);
                } else if (instr.opcode == op_codes::group_end) {
                    // Finalize the range: update the end index
                    const int id = std::stoi(instr.arguments[0]);
                    back_references[id].second = context.input_index; // End of the group match
                    context.instruction_index = state_to_index.at(instr.next_states[0]);
                } else if (instr.opcode == op_codes::match_backref) {
                    // Compare the current input with the captured group range
                    const int id = std::stoi(instr.arguments[0]);
                    const auto [start, end] = back_references[id]; // Get the range
                    size_t length = end - start;

                    if (context.input_index + length <= input.size() &&
                        input.substr(context.input_index, length) == input.substr(start, length)) {
                        context.input_index += length; // Consume matched backreference
                        context.instruction_index = state_to_index.at(instr.next_states[0]);
                    } else {
                        if (instr.next_states.size() == 1) {
                            break; // Failure case
                        }
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    }
                } else if (instr.opcode == op_codes::transition) {
                    context.instruction_index = state_to_index.at(instr.next_states[0]);
                    break;
                } else if (instr.opcode == op_codes::match_start_of_line) {
                    // Check if we are at the start of the input
                    if (context.input_index == 0) {
                        context.instruction_index = state_to_index.at(instr.next_states[0]);
                    } else {
                        if (instr.next_states.size() == 1) break; // Failure case
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    }
                } else if (instr.opcode == op_codes::match_end_of_line) {
                    // Check if we are at the end of the input
                    if (context.input_index == input.size()) {
                        context.instruction_index = state_to_index.at(instr.next_states[0]);
                    } else {
                        if (instr.next_states.size() == 1) break; // Failure case
                        context.instruction_index = state_to_index.at(instr.next_states[1]);
                    }
                } else if (instr.opcode == op_codes::failure) {
                    return false;
                } else if (instr.opcode == op_codes::success) {
                    return true;
                } else {
                    assert("unknown opcode");
                    break; // Unhandled opcode or error state
                }
            }
        }

        return false;
    }

    bool exec_nfa(const std::vector<nfa_instruction> &instructions, const std::string &input) {
        std::unordered_map<std::string, int> state_to_index;
        for (auto i = 0; i < instructions.size(); ++i) {
            state_to_index[instructions[i].state_id] = i;
        }
        return match_nfa(instructions, state_to_index, input);
    }
}

using namespace enfa;


TEST(test_engine_nx, BasicMatchString) {
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"match_range"}, {}},
        {"match_range", op_codes::match_string, {"success", "fail"}, {"text"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };
    ASSERT_TRUE(exec_nfa(nfa, "text"));
    ASSERT_FALSE(exec_nfa(nfa, "no-matched"));
}

TEST(test_engine_nx, BasicMatchAlternate) {
    // a|d
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"choice"}, {}},
        {"choice", op_codes::choice, {"match_a", "match_d"}, {}},
        {"match_a", op_codes::match_char, {"success", "match_d"}, {"a"}},
        {"match_d", op_codes::match_char, {"success", "fail"}, {"d"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}}
    };

    EXPECT_TRUE(exec_nfa(nfa, "a"));
    EXPECT_TRUE(exec_nfa(nfa, "d"));
}

TEST(test_engine_nx, BasicMatchRange) {
    // [s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"match_range"}, {}},
        {"match_range", op_codes::match_char_range, {"success", "fail"}, {"s", "w"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };
    ASSERT_TRUE(exec_nfa(nfa, "s"));
    ASSERT_FALSE(exec_nfa(nfa, "a"));
}

TEST(test_engine_nx, BasicMatchRangeNot) {
    // [^s-w]
    const std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"match_range"}, {}},
        {"match_range", op_codes::not_match_char_range, {"success", "fail"}, {"s", "w"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };
    ASSERT_FALSE(exec_nfa(nfa, "s"));
    ASSERT_TRUE(exec_nfa(nfa, "a"));
}

TEST(test_engine_nx, CountMatchRangeCount) {
    // [a-z]+
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"group_start"}, {}},
        {"group_start", op_codes::match_char_range, {"group_end", "fail"}, {"a", "z"}},
        {"group_end", op_codes::loop_count, {"group_start", "success"}, {"1", "20"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };
    ASSERT_TRUE(exec_nfa(nfa, "s"));
    ASSERT_TRUE(exec_nfa(nfa, "ss"));
    ASSERT_TRUE(exec_nfa(nfa, "sss"));
    ASSERT_FALSE(exec_nfa(nfa, "9"));
}


TEST(test_engine_nx, MatchRangeWithKleeneStar) {
    // [a-z]*
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"group_start"}, {}},
        {"group_start", op_codes::loop_count, {"match_range", "success"}, {"0", "-1"}},
        {"match_range", op_codes::match_char_range, {"group_start", "fail"}, {"a", "z"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    // Test cases for Kleene Star behavior
    ASSERT_TRUE(exec_nfa(nfa, "")); // Zero matches
    ASSERT_TRUE(exec_nfa(nfa, "a")); // Single match
    ASSERT_TRUE(exec_nfa(nfa, "abc")); // Multiple matches
    ASSERT_TRUE(exec_nfa(nfa, "z")); // Edge of range
    ASSERT_FALSE(exec_nfa(nfa, "1")); // Non-matching input
    ASSERT_FALSE(exec_nfa(nfa, "a1b")); // Mixed valid/invalid input
}

TEST(test_engine_nx, MatchRangeWithKleeneStarAndChar) {
    // [a-z]*9
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"group_start"}, {}},
        {"group_start", op_codes::loop_count, {"match_range", "match_9"}, {"0", "-1"}},
        {"match_range", op_codes::match_char_range, {"group_start", "match_9"}, {"a", "z"}},
        {"match_9", op_codes::match_char, {"success", "fail"}, {"9"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    // Test cases for [a-z]*9
    ASSERT_TRUE(exec_nfa(nfa, "9")); // Zero matches followed by '9'
    ASSERT_TRUE(exec_nfa(nfa, "a9")); // Single match followed by '9'
    ASSERT_TRUE(exec_nfa(nfa, "abc9")); // Multiple matches followed by '9'
    ASSERT_TRUE(exec_nfa(nfa, "z9")); // Edge case: last letter followed by '9'
    ASSERT_FALSE(exec_nfa(nfa, "a")); // Missing '9'
    ASSERT_FALSE(exec_nfa(nfa, "a19")); // Invalid character in between
    ASSERT_FALSE(exec_nfa(nfa, "xyz")); // Missing '9'
    ASSERT_FALSE(exec_nfa(nfa, "19")); // Starts with invalid character
}

TEST(test_engine_nx, MatchWithBackreference) {
    // Regex: ([a-z]) \1
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"group_start"}, {}},

        // Group start: ([a-z])
        {"group_start", op_codes::group_start, {"match_range"}, {"1"}},
        {"match_range", op_codes::match_char_range, {"group_end", "fail"}, {"a", "z"}},
        {"group_end", op_codes::group_end, {"match_space"}, {"1"}},

        // Match space
        {"match_space", op_codes::match_char, {"match_backref", "fail"}, {" "}},

        // Match backreference: \1
        {"match_backref", op_codes::match_backref, {"success", "fail"}, {"1"}},

        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    // Test cases for ([a-z]) \1
    ASSERT_TRUE(exec_nfa(nfa, "a a")); // Same letter with space
    ASSERT_TRUE(exec_nfa(nfa, "b b")); // Same letter with space
    ASSERT_FALSE(exec_nfa(nfa, "a b")); // Different letters
    ASSERT_FALSE(exec_nfa(nfa, "a  a")); // Extra space
    ASSERT_FALSE(exec_nfa(nfa, "a1 a")); // Invalid character
}

TEST(test_engine_nx, MatchStartOfLine) {
    // Regex: ^a
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"start_anchor"}, {}},
        {"start_anchor", op_codes::match_start_of_line, {"match_a"}, {}},
        {"match_a", op_codes::match_char, {"success", "fail"}, {"a"}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    ASSERT_TRUE(exec_nfa(nfa, "a")); // Start of line matches 'a'
    ASSERT_FALSE(exec_nfa(nfa, "ba")); // 'a' is not at the start
}

TEST(test_engine_nx, MatchEndOfLine) {
    // Regex: a$
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"match_a"}, {}},
        {"match_a", op_codes::match_char, {"end_anchor", "fail"}, {"a"}},
        {"end_anchor", op_codes::match_end_of_line, {"success"}, {}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    ASSERT_TRUE(exec_nfa(nfa, "a")); // 'a' is at the end
    ASSERT_FALSE(exec_nfa(nfa, "ab")); // 'a' is not at the end
}

TEST(test_engine_nx, MatchStartAndEndOfLine) {
    // Regex: ^a$
    std::vector<nfa_instruction> nfa{
        {"start", op_codes::start, {"start_anchor"}, {}},
        {"start_anchor", op_codes::match_start_of_line, {"match_a"}, {}},
        {"match_a", op_codes::match_char, {"end_anchor", "fail"}, {"a"}},
        {"end_anchor", op_codes::match_end_of_line, {"success"}, {}},
        {"fail", op_codes::failure, {}, {}},
        {"success", op_codes::success, {}, {}},
    };

    ASSERT_TRUE(exec_nfa(nfa, "a")); // Matches 'a' exactly
    ASSERT_FALSE(exec_nfa(nfa, "ab")); // Extra character at the end
    ASSERT_FALSE(exec_nfa(nfa, "ba")); // Extra character at the start
    ASSERT_FALSE(exec_nfa(nfa, "")); // Empty string
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
