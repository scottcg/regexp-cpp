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
    NORMAL, // Regular transition
    BEGIN_GROUP, // Start of a group
    END_GROUP, // End of a group
    BACK_REFERENCE // Back-reference to a group
};

// State of an NFA
struct state {
    static int next_id;
    int id;
    bool is_accept = false;
    state_type type = NORMAL;

    // Use a matcher function instead of fixed 'char'
    std::vector<std::pair<std::function<bool(char)>, std::shared_ptr<state> > > transitions;

    explicit state(bool is_accept = false) : id(next_id++), is_accept(is_accept) {
    }
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
    nfa build_concatenation(const std::string &input) {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        auto start = std::make_shared<state>();
        auto current = start;

        for (char c: input) {
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
        start->transitions.emplace_back([](char) { return true; }, loop); // ε-transition
        loop->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([](char) { return true; }, accept); // ε-transition

        return {start, accept};
    }

    // One or More: a+
    nfa build_one_or_more(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        auto loop = std::make_shared<state>();

        start->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([c](char input) { return input == c; }, loop);
        loop->transitions.emplace_back([](char) { return true; }, accept); // ε-transition

        return {start, accept};
    }

    // Optionality: a?
    nfa build_optionality(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back([](char) { return true; }, accept); // ε-transition
        start->transitions.emplace_back([c](char input) { return input == c; }, accept);

        return {start, accept};
    }

    // Character Classes: [a-z], [^a-z], [abc]
    nfa build_character_class(const std::string &input) {
        bool is_negated = input[0] == '^';
        auto matcher = parse_character_class(is_negated ? input.substr(1) : input, is_negated);

        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(matcher, accept);
        return {start, accept};
    }

private:
    // Helper to parse ranges and sets into a matcher function
    std::function<bool(char)> parse_character_class(const std::string &input, bool is_negated) {
        std::unordered_set<char> char_set;

        for (size_t i = 0; i < input.size(); ++i) {
            if (i + 2 < input.size() && input[i + 1] == '-') {
                // Handle ranges like a-z
                for (char c = input[i]; c <= input[i + 2]; ++c) {
                    char_set.insert(c);
                }
                i += 2; // Skip the range
            } else {
                char_set.insert(input[i]); // Add individual characters
            }
        }

        if (is_negated) {
            return [char_set](char c) {
                // Explicitly match anything NOT in the char_set
                return c != '\0' && char_set.find(c) == char_set.end();
            };
        } else {
            return [char_set](char c) {
                // Match anything in the char_set
                return c != '\0' && char_set.find(c) != char_set.end();
            };
        }
    }
};

// NFA engine
class nfa_processor {
public:
    // Simulate the execution of the NFA for a given input string
    bool execute(const nfa& nfa, const std::string& input, bool debug = false) {
        std::queue<std::pair<std::shared_ptr<state>, size_t>> to_process;
        std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

        to_process.emplace(nfa.start, 0);

        while (!to_process.empty()) {
            auto [current_state, position] = to_process.front();
            to_process.pop();

            if (debug)
                std::cout << "State: " << current_state->id
                          << ", Input Position: " << position
                          << ", Accept: " << (current_state->is_accept ? "YES" : "NO") << "\n";

            if (current_state->is_accept && position == input.size()) {
                if (debug)
                    std::cout << "Matched! Reached accept state.\n";
                return true;
            }

            if (!visited.insert({current_state, position}).second) {
                continue;
            }

            for (const auto& [matcher, next_state] : current_state->transitions) {
                if (matcher('\0')) { // ε-transition
                    if (debug)
                        std::cout << "  ε-transition to State " << next_state->id << "\n";
                    to_process.emplace(next_state, position);
                } else if (position < input.size() && matcher(input[position])) {
                    if (debug)
                        std::cout << "  Match '" << input[position] << "' -> State " << next_state->id << "\n";
                    to_process.emplace(next_state, position + 1);
                } else if (debug) {
                    std::cout << "  No match for '" << input[position] << "' at State " << current_state->id << "\n";
                }
            }
        }

        if (debug)
            std::cout << "No valid path found.\n";
        return false;
    }

private:
    // Hash function for unordered_set
    struct pair_hash {
        template<class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};

// DOT Visualization
void visualize_nfa_dot(const nfa &nfa, std::ostream &out) {
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

        for (const auto &[matcher, next] : current->transitions) {
            std::string label = (matcher('\0')) ? "ε" : "match"; // Adjust for matchers
            out << "  " << current->id << " -> " << next->id << " [label=\"" << label << "\"];\n";

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

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_EQ(nfa.start->transitions.size(), 1);
    auto middle_state = nfa.start->transitions[0].second;
    ASSERT_EQ(middle_state->transitions.size(), 1);
    EXPECT_TRUE(middle_state->transitions[0].second->is_accept);
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_zero_or_more('a');

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_EQ(nfa.start->transitions.size(), 2);
    auto loop_state = nfa.start->transitions[1].second;
    ASSERT_EQ(loop_state->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_one_or_more('a');

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_EQ(nfa.start->transitions.size(), 1);
    auto loop_state = nfa.start->transitions[0].second;
    ASSERT_EQ(loop_state->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa_builder builder;
    nfa nfa = builder.build_optionality('a');

    visualize_nfa_dot(nfa, std::cout);

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


TEST(NFA_Builder_Test, Build_CharacterClass_Range) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_character_class("a-z");
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "m"));
    EXPECT_TRUE(processor.execute(nfa, "z"));
    EXPECT_FALSE(processor.execute(nfa, "A"));
    EXPECT_FALSE(processor.execute(nfa, "1"));
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_character_class("abc");
    std::stringstream ss;
    visualize_nfa_dot(nfa, ss);
    std::cout << ss.str();

    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "b"));
    EXPECT_TRUE(processor.execute(nfa, "c"));
    EXPECT_FALSE(processor.execute(nfa, "d"));
    EXPECT_FALSE(processor.execute(nfa, "z"));
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedRange) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_character_class("^a-z");

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_TRUE(processor.execute(nfa, "A"));
    EXPECT_TRUE(processor.execute(nfa, "1"));
    EXPECT_TRUE(processor.execute(nfa, "!"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "m"));
    EXPECT_FALSE(processor.execute(nfa, "z"));
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_builder builder;
    nfa_processor processor;

    nfa nfa = builder.build_character_class("^abc");
    visualize_nfa_dot(nfa, std::cout);

    EXPECT_TRUE(processor.execute(nfa, "d"));
    EXPECT_TRUE(processor.execute(nfa, "z"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
    EXPECT_FALSE(processor.execute(nfa, "c"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
