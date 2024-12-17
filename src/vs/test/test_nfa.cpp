#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <memory>
#include <sstream>
#include <functional>
#include <gtest/gtest.h>

enum state_type {
    NORMAL,          // Regular transition
    BEGIN_GROUP,     // Start of a group
    END_GROUP,       // End of a group
    BACK_REFERENCE   // Back-reference to a group
};

// State of an NFA
struct state {
    static int next_id;
    int id;
    bool is_accept = false;
    state_type type = NORMAL;

    // Use a matcher function instead of fixed 'char'
    std::vector<std::pair<std::function<bool(char)>, std::shared_ptr<state>>> transitions;

    explicit state(bool is_accept = false) : id(next_id++), is_accept(is_accept) {}
};

int state::next_id = 0;

// NFA structure
struct nfa {
    std::shared_ptr<state> start;
    std::shared_ptr<state> accept;
};

class nfa_builder {
public:
    // Concatenation: "ab"
    nfa build_concatenation(const std::string& input) {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        auto start = std::make_shared<state>();
        auto current = start;

        for (char c : input) {
            auto next = std::make_shared<state>();
            current->transitions.emplace_back([c](char input) { return input == c; }, next);
            current = next;
        }

        current->is_accept = true;
        return {start, current};
    }

    // Zero or More: a*
    nfa build_zero_or_more(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        auto loop = std::make_shared<state>();

        start->transitions.emplace_back([](char) { return true; }, accept); // ε-transition
        start->transitions.emplace_back([](char) { return true; }, loop);   // ε-transition
        loop->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([](char) { return true; }, accept);  // ε-transition

        return {start, accept};
    }

    // One or More: a+
    nfa build_one_or_more(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        auto loop = std::make_shared<state>();

        start->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([](char) { return true; }, accept);  // ε-transition

        return {start, accept};
    }

    // Optionality: a?
    nfa build_optionality(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back([](char) { return true; }, accept);  // ε-transition
        start->transitions.emplace_back([c](char input) { return input == c; }, accept);

        return {start, accept};
    }
};

// NFA engine
class nfa_processor {
public:
    // Simulate the execution of the NFA for a given input string
    bool execute(const nfa& nfa, const std::string& input) {
        std::queue<std::pair<std::shared_ptr<state>, size_t>> to_process;
        std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

        to_process.emplace(nfa.start, 0);

        while (!to_process.empty()) {
            auto [current_state, position] = to_process.front();
            to_process.pop();

            if (current_state->is_accept && position == input.size()) {
                return true; // Reached an accept state after consuming all input
            }

            if (!visited.insert({current_state, position}).second) {
                continue; // Skip if already visited this state at this input position
            }

            for (const auto& [matcher, next_state] : current_state->transitions) {
                if (matcher('\0')) { // ε-transition
                    to_process.emplace(next_state, position);
                } else if (position < input.size() && matcher(input[position])) {
                    to_process.emplace(next_state, position + 1);
                }
            }
        }

        return false;
    }

private:
    // Hash function for unordered_set
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};

// DOT Visualization
void visualize_nfa_dot(const nfa& nfa, std::ostream& out) {
    out << "digraph NFA {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=circle];\n";
    out << "  start [shape=point];\n";

    std::queue<std::shared_ptr<state>> to_process;
    std::unordered_set<int> visited;

    out << "  start -> " << nfa.start->id << " [label=\"ε\"];\n";
    to_process.push(nfa.start);
    visited.insert(nfa.start->id);

    while (!to_process.empty()) {
        auto current = to_process.front();
        to_process.pop();

        if (current->is_accept) {
            out << "  " << current->id << " [shape=doublecircle];\n";
        }

        for (const auto& [matcher, next] : current->transitions) {
            out << "  " << current->id << " -> " << next->id << " [label=\"match\"];\n";

            if (visited.insert(next->id).second) {
                to_process.push(next);
            }
        }
    }

    out << "}\n";
}

// Unit Tests
TEST(NFA_Builder_Test, Build_Concatenation) {
    nfa_builder builder;
    nfa nfa = builder.build_concatenation("ab");

    std::stringstream ss;
    visualize_nfa_dot(nfa, ss);

    std::cout << ss.str();

    EXPECT_EQ(nfa.start->transitions.size(), 1);
    auto middle_state = nfa.start->transitions[0].second;
    ASSERT_EQ(middle_state->transitions.size(), 1);
    EXPECT_TRUE(middle_state->transitions[0].second->is_accept);
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_zero_or_more('a');

    std::stringstream ss;
    visualize_nfa_dot(nfa, ss);

    std::cout << ss.str();

    EXPECT_EQ(nfa.start->transitions.size(), 2);
    auto loop_state = nfa.start->transitions[1].second;
    ASSERT_EQ(loop_state->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_one_or_more('a');

    std::stringstream ss;
    visualize_nfa_dot(nfa, ss);

    std::cout << ss.str();

    EXPECT_EQ(nfa.start->transitions.size(), 1);
    auto loop_state = nfa.start->transitions[0].second;
    ASSERT_EQ(loop_state->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa_builder builder;
    nfa nfa = builder.build_optionality('a');

    std::stringstream ss;
    visualize_nfa_dot(nfa, ss);

    std::cout << ss.str();

    EXPECT_EQ(nfa.start->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}


TEST(NFA_Processor_Test, Execute_Concatenation) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_concatenation("ab");
    EXPECT_TRUE(processor.execute(nfa, "ab"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "abc"));
}

TEST(NFA_Processor_Test, Execute_ZeroOrMore) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_zero_or_more('a');
    EXPECT_TRUE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}

TEST(NFA_Processor_Test, Execute_OneOrMore) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_one_or_more('a');
    EXPECT_FALSE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}

TEST(NFA_Processor_Test, Execute_Optionality) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_optionality('a');
    EXPECT_TRUE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "aa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}