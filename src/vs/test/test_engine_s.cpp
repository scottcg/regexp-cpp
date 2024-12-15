#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
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
    std::vector<std::pair<char, State*>> transitions;
    StateType type = NORMAL;
};

struct NFA {
    State* start;
    State* accept;
};

// Global state counter for unique IDs
int state_counter = 0;

// Utility to create a new state
State* create_state(bool is_accept = false) {
    return new State{state_counter++, is_accept, -1, {}};
}

// Helper: Add a transition between two states
void add_transition(State* from, State* to, char symbol) {
    from->transitions.emplace_back(symbol, to);
}

// Build NFA for a single literal character
NFA build_literal(char c) {
    State* start = create_state();
    State* accept = create_state(true);
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
    State* start = create_state();
    State* accept = create_state(true);

    add_transition(start, first.start, '\0'); // ε-transition
    add_transition(start, second.start, '\0'); // ε-transition

    add_transition(first.accept, accept, '\0');
    add_transition(second.accept, accept, '\0');

    first.accept->is_accept = false;
    second.accept->is_accept = false;

    return {start, accept};
}

// Repetition: a*
NFA repetition(const NFA& nfa) {
    State* start = create_state();
    State* accept = create_state(true);

    add_transition(start, nfa.start, '\0'); // ε-transition to NFA
    add_transition(start, accept, '\0');   // ε-transition to accept
    add_transition(nfa.accept, nfa.start, '\0'); // Loop back to start
    add_transition(nfa.accept, accept, '\0');    // Transition to accept

    nfa.accept->is_accept = false;

    return {start, accept};
}

NFA begin_group(int group_id) {
    State* start = create_state();
    start->type = BEGIN_GROUP;
    start->group_id = group_id;

    State* accept = create_state();
    add_transition(start, accept, '\0'); // ε-transition
    return {start, accept};
}

NFA end_group(int group_id) {
    State* start = create_state();
    start->type = END_GROUP;
    start->group_id = group_id;

    State* accept = create_state();
    add_transition(start, accept, '\0'); // ε-transition
    return {start, accept};
}

// Compute the epsilon closure of a set of states
void epsilon_closure(std::unordered_set<State*>& states) {
    std::queue<State*> to_process;

    for (State* s : states) to_process.push(s);

    while (!to_process.empty()) {
        State* current = to_process.front();
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
        State* state;
        int input_index;
        std::unordered_map<int, std::string> group_captures; // Captured groups
    };

    std::queue<Frame> state_queue;
    state_queue.push({nfa.start, 0, {}});

    while (!state_queue.empty()) {
        Frame frame = state_queue.front();
        state_queue.pop();

        State* state = frame.state;
        int index = frame.input_index;

        // Accept state check
        if (state->is_accept && index == input.size()) {
            // Debug: Print captured groups
            for (const auto& [group_id, capture] : frame.group_captures) {
                std::cout << "Group " << group_id << ": " << capture << "\n";
            }
            return true;
        }

        // Process BEGIN_GROUP: Mark starting position
        if (state->type == BEGIN_GROUP) {
            frame.group_captures[state->group_id] = ""; // Clear group
            frame.group_captures[state->group_id] += input.substr(index);
        }

        // Process END_GROUP: Capture substring
        if (state->type == END_GROUP) {
            int start_pos = frame.group_captures[state->group_id].size();
            frame.group_captures[state->group_id] =
                input.substr(index - start_pos, start_pos);
        }

        // Process transitions
        for (const auto& [symbol, next] : state->transitions) {
            if (symbol == '\0') { // ε-transition
                state_queue.push({next, index, frame.group_captures});
            } else if (index < input.size() && symbol == input[index]) {
                state_queue.push({next, index + 1, frame.group_captures});
            }
        }
    }

    return false;
}

// Google Test cases
TEST(NFATest, LiteralMatch) {
    NFA nfa = build_literal('a');

    EXPECT_TRUE(execute_nfa(nfa, "a"));
    EXPECT_FALSE(execute_nfa(nfa, "b"));
}

TEST(NFATest, Concatenation) {
    NFA a = build_literal('a');
    NFA b = build_literal('b');
    NFA ab = concatenate(a, b);

    EXPECT_TRUE(execute_nfa(ab, "ab"));
    EXPECT_FALSE(execute_nfa(ab, "a"));
    EXPECT_FALSE(execute_nfa(ab, "b"));
}

TEST(NFATest, Alternation) {
    NFA a = build_literal('a');
    NFA b = build_literal('b');
    NFA a_or_b = alternation(a, b);

    EXPECT_TRUE(execute_nfa(a_or_b, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b, "b"));
    EXPECT_FALSE(execute_nfa(a_or_b, "c"));
}

TEST(NFATest, Repetition) {
    NFA a = build_literal('a');
    NFA a_star = repetition(a);

    EXPECT_TRUE(execute_nfa(a_star, ""));
    EXPECT_TRUE(execute_nfa(a_star, "a"));
    EXPECT_TRUE(execute_nfa(a_star, "aaa"));
    EXPECT_FALSE(execute_nfa(a_star, "b"));
}

TEST(NFATest, ComplexConcatenation) {
    // Regex: "abc"
    NFA a = build_literal('a');
    NFA b = build_literal('b');
    NFA c = build_literal('c');
    NFA abc = concatenate(concatenate(a, b), c);

    EXPECT_TRUE(execute_nfa(abc, "abc"));
    EXPECT_FALSE(execute_nfa(abc, "ab"));
    EXPECT_FALSE(execute_nfa(abc, "abcd"));
    EXPECT_FALSE(execute_nfa(abc, ""));
}

TEST(NFATest, NestedAlternation) {
    // Regex: "a|(b|c)"
    NFA a = build_literal('a');
    NFA b = build_literal('b');
    NFA c = build_literal('c');
    NFA b_or_c = alternation(b, c);
    NFA a_or_b_or_c = alternation(a, b_or_c);

    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "b"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "c"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, "d"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, ""));
}

TEST(NFATest, ComplexRepetition) {
    // Regex: "a*(b|c)"
    NFA a = build_literal('a');
    NFA a_star = repetition(a);
    NFA b = build_literal('b');
    NFA c = build_literal('c');
    NFA b_or_c = alternation(b, c);
    NFA pattern = concatenate(a_star, b_or_c);

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
    NFA a = build_literal('a');
    NFA b = build_literal('b');
    NFA group_start = begin_group(1);
    NFA group_end = end_group(1);

    NFA alt = alternation(a, b);
    NFA group = concatenate(group_start, concatenate(alt, group_end));

    group.accept->is_accept = true;

    EXPECT_TRUE(execute_nfa(group, "a")); // Should capture group 1: "a"
    EXPECT_TRUE(execute_nfa(group, "b")); // Should capture group 1: "b"
    EXPECT_FALSE(execute_nfa(group, "c"));
}

TEST(NFATest, EmptyString) {
    // Regex: ""
    NFA empty_nfa = build_literal('\0');
    empty_nfa.accept->is_accept = true;

    EXPECT_TRUE(execute_nfa(empty_nfa, ""));
    EXPECT_FALSE(execute_nfa(empty_nfa, "a"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}