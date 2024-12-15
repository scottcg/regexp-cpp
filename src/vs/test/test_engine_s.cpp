#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <gtest/gtest.h>

// Represents an NFA state
struct State {
    int id;
    bool is_accept = false;
    std::vector<std::pair<char, State*>> transitions; // (symbol, next state)
};

struct NFA {
    State* start;
    State* accept;
};

// Global state counter for unique IDs
int state_counter = 0;

// Utility to create a new state
State* create_state(bool is_accept = false) {
    return new State{state_counter++, is_accept, {}};
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
    std::unordered_set<State*> current_states = {nfa.start};
    epsilon_closure(current_states);

    for (char c : input) {
        std::unordered_set<State*> next_states;

        for (State* state : current_states) {
            for (const auto& [symbol, next] : state->transitions) {
                if (symbol == c) next_states.insert(next);
            }
        }

        current_states = std::move(next_states);
        epsilon_closure(current_states); // Compute ε-closure of new states
    }

    for (State* state : current_states) {
        if (state->is_accept) return true;
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}