#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <memory>
#include <sstream>
#include <gtest/gtest.h>

enum state_type {
    NORMAL,          // Regular transition
    LOOKAHEAD_POS,   // Positive Lookahead
    LOOKAHEAD_NEG,   // Negative Lookahead
    BEGIN_GROUP,     // Start of a group
    END_GROUP,       // End of a group
    BACK_REFERENCE   // Back-reference to a group
};

enum transition_type {
    EPSILON,          // ε-transition
    LITERAL,          // Single character match
    CHARACTER_CLASS,  // Character class (e.g., [a-z])
    NEGATED_CLASS     // Negated character class (e.g., [^a-z])
};

// Transition object
struct transition {
    transition_type type;
    std::shared_ptr<struct state> target;
    char literal;                          // For LITERAL transitions
    std::unordered_set<char> char_set;     // For CHARACTER_CLASS and NEGATED_CLASS

    // Epsilon constructor
    explicit transition(std::shared_ptr<state> target)
        : type(EPSILON), target(std::move(target)), literal('\0') {}

    // Literal constructor
    transition(char c, std::shared_ptr<state> target)
        : type(LITERAL), target(std::move(target)), literal(c) {}

    // Character class constructor
    transition(const std::unordered_set<char>& set, std::shared_ptr<state> target, bool negated = false)
        : type(negated ? NEGATED_CLASS : CHARACTER_CLASS), target(std::move(target)), literal(0), char_set(set) {
    }

    // Matches input character
    bool matches(const char input) const {
        switch (type) {
            case EPSILON: return true;
            case LITERAL: return input == literal;
            case CHARACTER_CLASS: return char_set.count(input) > 0;
            case NEGATED_CLASS: return char_set.count(input) == 0;
            default: return false;
        }
    }
};

// State of an NFA
struct state {
    static int next_id;
    int id;
    bool is_accept = false;
    state_type type = NORMAL;
    std::vector<transition> transitions;

    explicit state(bool is_accept = false) : id(next_id++), is_accept(is_accept) {}
};

int state::next_id = 0;

// NFA structure
struct nfa {
    std::shared_ptr<state> start;
    std::shared_ptr<state> accept;
};

// NFA Builder
class nfa_builder {
public:
    // Concatenation: "ab"
    static nfa build_concatenation(const std::string &input) {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        auto start = std::make_shared<state>();
        auto current = start;

        for (char c : input) {
            auto next = std::make_shared<state>();
            current->transitions.emplace_back(c, next);
            current = next;
        }

        current->is_accept = true;
        return {start, current};
    }

    // Zero or More: a*
    static nfa build_zero_or_more(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        auto loop = std::make_shared<state>();

        start->transitions.emplace_back(loop);   // ε-transition to loop
        start->transitions.emplace_back(accept); // ε-transition to accept
        loop->transitions.emplace_back(c, loop); // Match c and loop
        loop->transitions.emplace_back(accept);  // ε-transition to accept

        return {start, accept};
    }

    // One or More: a+
    static nfa build_one_or_more(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        auto loop = std::make_shared<state>();

        start->transitions.emplace_back(c, loop);
        loop->transitions.emplace_back(c, loop);
        loop->transitions.emplace_back(accept);

        return {start, accept};
    }

    // Optionality: a?
    static nfa build_optionality(char c) {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(c, accept);
        start->transitions.emplace_back(accept); // ε-transition

        return {start, accept};
    }

    // Character Classes: [a-z], [^a-z], [abc]
    static nfa build_character_class(const std::string &input) {
        bool is_negated = input[0] == '^';
        std::unordered_set<char> char_set;

        for (size_t i = (is_negated ? 1 : 0); i < input.size(); ++i) {
            if (i + 2 < input.size() && input[i + 1] == '-') {
                for (char c = input[i]; c <= input[i + 2]; ++c) {
                    char_set.insert(c);
                }
                i += 2;
            } else {
                char_set.insert(input[i]);
            }
        }

        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(char_set, accept, is_negated);

        return {start, accept};
    }
};

// NFA Processor
class nfa_processor {
public:
    static bool execute(const nfa &nfa, const std::string &input) {
        std::queue<std::pair<std::shared_ptr<state>, size_t>> to_process;
        std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

        to_process.emplace(nfa.start, 0);

        while (!to_process.empty()) {
            auto [current, pos] = to_process.front();
            to_process.pop();

            if (current->is_accept && pos == input.size()) return true;

            if (!visited.insert({current, pos}).second) continue;

            for (const auto &t : current->transitions) {
                if (t.type == EPSILON) {
                    to_process.emplace(t.target, pos);
                } else if (pos < input.size() && t.matches(input[pos])) {
                    to_process.emplace(t.target, pos + 1);
                }
            }
        }

        return false;
    }

private:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};

// DOT Visualization
void visualize_nfa_dot(const nfa &nfa, std::ostream &out) {
    out << "digraph NFA {\n  rankdir=LR;\n  node [shape=circle];\n  start [shape=point];\n";
    out << "  start -> " << nfa.start->id << " [label=\"ε\"];\n";

    std::queue<std::shared_ptr<state>> to_process;
    std::unordered_set<int> visited;
    to_process.push(nfa.start);
    visited.insert(nfa.start->id);

    while (!to_process.empty()) {
        auto current = to_process.front();
        to_process.pop();

        if (current->is_accept) out << "  " << current->id << " [shape=doublecircle];\n";

        for (const auto &t : current->transitions) {
            std::string label = (t.type == EPSILON) ? "ε" : (t.type == LITERAL) ? std::string(1, t.literal)
                                                                               : "class";
            out << "  " << current->id << " -> " << t.target->id << " [label=\"" << label << "\"];\n";

            if (visited.insert(t.target->id).second) {
                to_process.push(t.target);
            }
        }
    }
    out << "}\n";
}

TEST(NFA_Builder_Test, Build_Concatenation) {
    nfa nfa = nfa_builder::build_concatenation("ab");

    visualize_nfa_dot(nfa, std::cout);

    // Start state should have one transition to the next state
    ASSERT_EQ(nfa.start->transitions.size(), 1);
    const auto &first_transition = nfa.start->transitions[0];
    EXPECT_EQ(first_transition.type, LITERAL);
    EXPECT_EQ(first_transition.literal, 'a');

    // Intermediate state should transition to accept state
    auto middle_state = first_transition.target;
    ASSERT_EQ(middle_state->transitions.size(), 1);
    const auto &second_transition = middle_state->transitions[0];
    EXPECT_EQ(second_transition.type, LITERAL);
    EXPECT_EQ(second_transition.literal, 'b');

    // Final state should be accept
    EXPECT_TRUE(second_transition.target->is_accept);
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa nfa = nfa_builder::build_zero_or_more('a');

    visualize_nfa_dot(nfa, std::cout);

    ASSERT_EQ(nfa.start->transitions.size(), 2);
    const auto &epsilon_to_loop = nfa.start->transitions[0];
    const auto &epsilon_to_accept = nfa.start->transitions[1];

    EXPECT_EQ(epsilon_to_loop.type, EPSILON);
    EXPECT_EQ(epsilon_to_accept.type, EPSILON);

    auto loop_state = epsilon_to_loop.target;
    ASSERT_EQ(loop_state->transitions.size(), 2);
    const auto &match_loop = loop_state->transitions[0];
    const auto &epsilon_to_accept_from_loop = loop_state->transitions[1];

    EXPECT_EQ(match_loop.type, LITERAL);
    EXPECT_EQ(match_loop.literal, 'a');
    EXPECT_EQ(epsilon_to_accept_from_loop.type, EPSILON);

    EXPECT_TRUE(epsilon_to_accept.target->is_accept);
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa nfa = nfa_builder::build_one_or_more('a');

    visualize_nfa_dot(nfa, std::cout);

    ASSERT_EQ(nfa.start->transitions.size(), 1);
    const auto &first_transition = nfa.start->transitions[0];
    EXPECT_EQ(first_transition.type, LITERAL);
    EXPECT_EQ(first_transition.literal, 'a');

    auto loop_state = first_transition.target;
    ASSERT_EQ(loop_state->transitions.size(), 2);

    const auto &match_loop = loop_state->transitions[0];
    const auto &epsilon_to_accept = loop_state->transitions[1];

    EXPECT_EQ(match_loop.type, LITERAL);
    EXPECT_EQ(match_loop.literal, 'a');
    EXPECT_EQ(epsilon_to_accept.type, EPSILON);

    EXPECT_TRUE(epsilon_to_accept.target->is_accept);
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa nfa = nfa_builder::build_optionality('a');

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_EQ(nfa.start->transitions.size(), 2);
    EXPECT_TRUE(nfa.accept->is_accept);
}


TEST(NFA_Processor_Test, Execute_Concatenation) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_concatenation("ab");
    EXPECT_TRUE(processor.execute(nfa, "ab"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "abc"));
}

TEST(NFA_Processor_Test, Execute_ZeroOrMore) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_zero_or_more('a');
    EXPECT_TRUE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}

TEST(NFA_Processor_Test, Execute_OneOrMore) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_one_or_more('a');
    EXPECT_FALSE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}

TEST(NFA_Processor_Test, Execute_Optionality) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_optionality('a');
    EXPECT_TRUE(processor.execute(nfa, ""));
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "aa"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
}


TEST(NFA_Builder_Test, Build_CharacterClass_Range) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_character_class("a-z");
    EXPECT_TRUE(processor.execute(nfa, "a"));
    EXPECT_TRUE(processor.execute(nfa, "m"));
    EXPECT_TRUE(processor.execute(nfa, "z"));
    EXPECT_FALSE(processor.execute(nfa, "A"));
    EXPECT_FALSE(processor.execute(nfa, "1"));
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_character_class("abc");
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
    nfa_processor processor;

    nfa nfa = nfa_builder::build_character_class("^a-z");

    visualize_nfa_dot(nfa, std::cout);

    EXPECT_TRUE(processor.execute(nfa, "A"));
    EXPECT_TRUE(processor.execute(nfa, "1"));
    EXPECT_TRUE(processor.execute(nfa, "!"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "m"));
    EXPECT_FALSE(processor.execute(nfa, "z"));
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_processor processor;

    nfa nfa = nfa_builder::build_character_class("^abc");
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
