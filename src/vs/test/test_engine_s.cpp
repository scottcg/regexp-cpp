#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <memory>
#include <gtest/gtest.h>

enum StateType {
    NORMAL,          // Regular transition
    BEGIN_GROUP,     // Start of a group
    END_GROUP,       // End of a group
    BACK_REFERENCE   // Back-reference to a group
};

struct State {
    int id;
    bool is_accept = false;
    int group_id = -1; // Group ID for BEGIN_GROUP and END_GROUP
    std::vector<std::pair<char, std::shared_ptr<State>>> transitions;
    StateType type = NORMAL;
};

struct NFA {
    std::shared_ptr<State> start;
    std::shared_ptr<State> accept;
};

// Global state counter for unique IDs
int state_counter = 0;

// Utility to create a new state
std::shared_ptr<State> create_state(const bool is_accept = false) {
    return std::make_shared<State>(State{state_counter++, is_accept, -1, {}});
}

// Helper: Add a transition between two states
void add_transition(const std::shared_ptr<State>& from, const std::shared_ptr<State>& to, char symbol) {
    from->transitions.emplace_back(symbol, to);
}

// Build NFA for a single literal character
NFA build_literal(const char c) {
    const auto start = create_state();
    const auto accept = create_state(true);
    add_transition(start, accept, c);
    return {start, accept};
}

// Concatenate two NFAs
NFA concatenate(const NFA& first, const NFA& second) {
    add_transition(first.accept, second.start, '\0'); // ε-transition
    first.accept->is_accept = false;
    return {first.start, second.accept};
}

// Alternation: a|b
NFA alternation(const NFA& first, const NFA& second) {
    const auto start = create_state();
    const auto accept = create_state(true);

    add_transition(start, first.start, '\0'); // ε-transition
    add_transition(start, second.start, '\0'); // ε-transition

    add_transition(first.accept, accept, '\0');
    add_transition(second.accept, accept, '\0');

    first.accept->is_accept = false;
    second.accept->is_accept = false;

    return {start, accept};
}

// Repetition: a*
NFA repetition(const NFA& nfa, bool one_or_more = false) {
    const auto start = create_state();
    const auto accept = create_state(true);

    // If one_or_more is true, require at least one transition
    if (one_or_more) {
        add_transition(start, nfa.start, '\0'); // ε-transition to NFA
        add_transition(nfa.accept, nfa.start, '\0'); // Loop back to start
        add_transition(nfa.accept, accept, '\0');    // Transition to accept
    } else {
        // Zero or more (standard *)
        add_transition(start, nfa.start, '\0'); // ε-transition to NFA
        add_transition(start, accept, '\0');    // ε-transition to accept
        add_transition(nfa.accept, nfa.start, '\0'); // Loop back to start
        add_transition(nfa.accept, accept, '\0');    // Transition to accept
    }

    nfa.accept->is_accept = false;
    return {start, accept};
}

NFA begin_group(const int group_id) {
    const auto start = create_state();
    start->type = BEGIN_GROUP;
    start->group_id = group_id;

    const auto accept = create_state();
    add_transition(start, accept, '\0'); // ε-transition
    return {start, accept};
}

NFA end_group(const int group_id) {
    const auto start = create_state();
    start->type = END_GROUP;
    start->group_id = group_id;

    const auto accept = create_state();
    add_transition(start, accept, '\0'); // ε-transition
    return {start, accept};
}

// Compute the epsilon closure of a set of states
void epsilon_closure(std::unordered_set<std::shared_ptr<State>>& states) {
    std::queue<std::shared_ptr<State>> to_process;

    for (const auto& s : states) to_process.push(s);

    while (!to_process.empty()) {
        const auto current = to_process.front();
        to_process.pop();

        for (const auto& [symbol, next] : current->transitions) {
            if (symbol == '\0' && states.insert(next).second) { // ε-transition
                to_process.push(next);
            }
        }
    }
}

// Execute NFA on input string
bool execute_nfa(const NFA& nfa, const std::string& input) {
    struct Frame {
        std::shared_ptr<State> state;
        int input_index;
        std::unordered_map<int, std::string> group_captures; // Group captures
        std::unordered_map<int, int> group_start_positions;  // Group start positions
    };

    std::queue<Frame> state_queue;
    state_queue.push({nfa.start, 0, {}, {}}); // Initial frame

    while (!state_queue.empty()) {
        Frame frame = state_queue.front();
        state_queue.pop();

        const auto state = frame.state;
        const int index = frame.input_index;

        // Accept state: check if input is consumed
        if (state->is_accept && index == input.size()) {
            for (const auto& [group_id, capture] : frame.group_captures) {
                std::cout << "Group " << group_id << ": " << capture << "\n";
            }
            return true;
        }

        // Handle group start
        if (state->type == BEGIN_GROUP) {
            frame.group_start_positions[state->group_id] = index; // Record group start position
        }

        // Handle group end
        if (state->type == END_GROUP) {
            auto start_it = frame.group_start_positions.find(state->group_id);
            if (start_it != frame.group_start_positions.end()) {
                int start = start_it->second;
                frame.group_captures[state->group_id] = input.substr(start, index - start);
            }
        }

        // Process transitions
        for (const auto& [symbol, next] : state->transitions) {
            if (symbol == '\0') { // ε-transition
                state_queue.push({
                    next,
                    index,
                    frame.group_captures,      // Copy group captures
                    frame.group_start_positions // Copy group start positions
                });
            } else if (index < input.size() && symbol == input[index]) {
                state_queue.push({
                    next,
                    index + 1,
                    frame.group_captures,      // Copy group captures
                    frame.group_start_positions // Copy group start positions
                });
            }
        }
    }

    return false; // No match
}

// Google Test cases
TEST(NFATest, LiteralMatch) {
    const NFA nfa = build_literal('a');

    EXPECT_TRUE(execute_nfa(nfa, "a"));
    EXPECT_FALSE(execute_nfa(nfa, "b"));
}

TEST(NFATest, Concatenation) {
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA ab = concatenate(a, b);

    EXPECT_TRUE(execute_nfa(ab, "ab"));
    EXPECT_FALSE(execute_nfa(ab, "a"));
    EXPECT_FALSE(execute_nfa(ab, "b"));
}

TEST(NFATest, Alternation) {
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA a_or_b = alternation(a, b);

    EXPECT_TRUE(execute_nfa(a_or_b, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b, "b"));
    EXPECT_FALSE(execute_nfa(a_or_b, "c"));
}

TEST(NFATest, Repetition) {
    const NFA a = build_literal('a');
    const NFA a_star = repetition(a);

    EXPECT_TRUE(execute_nfa(a_star, ""));
    EXPECT_TRUE(execute_nfa(a_star, "a"));
    EXPECT_TRUE(execute_nfa(a_star, "aaa"));
    EXPECT_FALSE(execute_nfa(a_star, "b"));
}

TEST(NFATest, ComplexConcatenation) {
    // Regex: "abc"
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA c = build_literal('c');
    const NFA abc = concatenate(concatenate(a, b), c);

    EXPECT_TRUE(execute_nfa(abc, "abc"));
    EXPECT_FALSE(execute_nfa(abc, "ab"));
    EXPECT_FALSE(execute_nfa(abc, "abcd"));
    EXPECT_FALSE(execute_nfa(abc, ""));
}

TEST(NFATest, NestedAlternation) {
    // Regex: "a|(b|c)"
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA c = build_literal('c');
    const NFA b_or_c = alternation(b, c);
    const NFA a_or_b_or_c = alternation(a, b_or_c);

    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "b"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "c"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, "d"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, ""));
}

TEST(NFATest, ComplexRepetition) {
    // Regex: "a*(b|c)"
    const NFA a = build_literal('a');
    const NFA a_star = repetition(a);
    const NFA b = build_literal('b');
    const NFA c = build_literal('c');
    const NFA b_or_c = alternation(b, c);
    const NFA pattern = concatenate(a_star, b_or_c);

    EXPECT_TRUE(execute_nfa(pattern, "b"));
    EXPECT_TRUE(execute_nfa(pattern, "c"));
    EXPECT_TRUE(execute_nfa(pattern, "ab"));
    EXPECT_TRUE(execute_nfa(pattern, "aac"));
    EXPECT_TRUE(execute_nfa(pattern, "aaaaab"));
    EXPECT_FALSE(execute_nfa(pattern, "d"));
    EXPECT_FALSE(execute_nfa(pattern, "a"));
    EXPECT_FALSE(execute_nfa(pattern, "aabd"));
}

TEST(NFATest, GroupCapture) {
    // Regex: (a|b)
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA group_start = begin_group(1);
    const NFA group_end = end_group(1);

    const NFA alt = alternation(a, b);
    const NFA group = concatenate(group_start, concatenate(alt, group_end));

    group.accept->is_accept = true;

    EXPECT_TRUE(execute_nfa(group, "a")); // Should capture group 1: "a"
    EXPECT_TRUE(execute_nfa(group, "b")); // Should capture group 1: "b"
    EXPECT_FALSE(execute_nfa(group, "c"));
}

TEST(NFATest, GroupCaptureAdvanced) {
    // Regex: (a|b)c*
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA c = build_literal('c');
    const NFA group_start = begin_group(1);
    const NFA group_end = end_group(1);

    const NFA a_or_b = alternation(a, b);
    const NFA group = concatenate(group_start, concatenate(a_or_b, group_end));
    const NFA c_star = repetition(c);
    const NFA pattern = concatenate(group, c_star);

    // Expected Behavior:
    // Input: "a", Captures group 1: "a"
    // Input: "b", Captures group 1: "b"
    // Input: "accc", Captures group 1: "a"
    // Input: "bcc", Captures group 1: "b"
    // Input: "c", Should fail (no group capture for 'a' or 'b')

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(pattern, "a"));
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: a") != std::string::npos);

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(pattern, "b"));
    output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: b") != std::string::npos);

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(pattern, "accc"));
    output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: a") != std::string::npos);

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(pattern, "bcc"));
    output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: b") != std::string::npos);

    EXPECT_FALSE(execute_nfa(pattern, "c"));
}

TEST(NFATest, NestedGroupCapture) {
    // Regex: (a(b))c
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA c = build_literal('c');

    const NFA group1_start = begin_group(1);
    const NFA group1_end = end_group(1);

    const NFA group2_start = begin_group(2);
    const NFA group2_end = end_group(2);

    // Build nested groups
    const NFA inner_group = concatenate(group2_start, concatenate(b, group2_end));
    const NFA outer_group = concatenate(group1_start, concatenate(a, concatenate(inner_group, group1_end)));
    const NFA pattern = concatenate(outer_group, c);

    // Expected Behavior:
    // Input: "abc", Captures:
    //   Group 1: "ab"
    //   Group 2: "b"

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(pattern, "abc"));
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.find("Group 1: ab") != std::string::npos);
    EXPECT_TRUE(output.find("Group 2: b") != std::string::npos);

    EXPECT_FALSE(execute_nfa(pattern, "ac")); // No "b" means group fails
}

TEST(NFATest, GroupCaptureWithRepetition) {
    // Regex: (a|b)+
    const NFA a = build_literal('a');
    const NFA b = build_literal('b');
    const NFA group_start = begin_group(1);
    const NFA group_end = end_group(1);

    const NFA a_or_b = alternation(a, b);
    const NFA group = concatenate(group_start, concatenate(a_or_b, group_end));
    const NFA repeated_group = repetition(group, true); // Use one_or_more = true

    // Expected Behavior:
    // Input: "a", Captures group 1: "a"
    // Input: "b", Captures group 1: "b"
    // Input: "ab", Captures group 1: "b" (latest repetition capture)
    // Input: "", Should fail (no match)

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(repeated_group, "a"));
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: a") != std::string::npos);

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(repeated_group, "b"));
    output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: b") != std::string::npos);

    testing::internal::CaptureStdout();
    EXPECT_TRUE(execute_nfa(repeated_group, "ab"));
    output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Group 1: b") != std::string::npos);

    EXPECT_FALSE(execute_nfa(repeated_group, ""));
}

TEST(NFATest, EmptyString) {
    // Regex: ""
    const NFA empty_nfa = build_literal('\0');
    empty_nfa.accept->is_accept = true;

    EXPECT_TRUE(execute_nfa(empty_nfa, ""));
    EXPECT_FALSE(execute_nfa(empty_nfa, "a"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}