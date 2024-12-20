#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>
#include <memory>
#include <sstream>
#include <gtest/gtest.h>

enum transition_type {
    EPSILON, // ε-transition
    LITERAL, // Single character match
    CHARACTER_CLASS, // Character class (e.g., [a-z])
    NEGATED_CLASS // Negated character class (e.g., [^a-z])
};

// Transition object
struct transition {
    transition_type type;
    std::shared_ptr<struct state> target;
    char literal; // For LITERAL transitions
    std::unordered_set<char> char_set; // For CHARACTER_CLASS and NEGATED_CLASS

    // Epsilon constructor
    explicit transition(std::shared_ptr<state> target)
        : type(EPSILON), target(std::move(target)), literal('\0') {
    }

    // Literal constructor
    transition(char c, std::shared_ptr<state> target)
        : type(LITERAL), target(std::move(target)), literal(c) {
    }

    // Character class constructor
    transition(const std::unordered_set<char> &set, std::shared_ptr<state> target, bool negated = false)
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
    int group_start_index = -1;
    int group_end_index = -1;
    std::vector<transition> transitions;

    explicit state(bool is_accept = false) : id(next_id++), is_accept(is_accept) {
    }
};

int state::next_id = 0;


struct nfa {
    std::shared_ptr<state> start;
    std::shared_ptr<state> accept;
};


struct build_result {
    nfa automaton;
    std::unordered_map<std::string, int> named_groups;
};


class nfa_builder {
    int group_counter = 1; // Start group numbering from 1
    mutable std::unordered_map<std::string, int> named_groups;

public:
    const std::unordered_map<std::string, int> &get_named_groups() const {
        return named_groups;
    }

    // Complete the build process
    build_result complete(const nfa &input_nfa) {
        return {input_nfa, named_groups};
    }

    // Single Character NFA
    nfa build_literal(char c) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(c, accept);
        return {start, accept};
    }

    // Generalized Concatenation: Combine two NFAs
    nfa build_concatenation(const nfa &nfa1, const nfa &nfa2) const {
        nfa1.accept->is_accept = false; // Remove accept status from first NFA
        nfa1.accept->transitions.emplace_back(nfa2.start); // Link first NFA to second NFA
        return {nfa1.start, nfa2.accept};
    }

    // Character-based Concatenation: Forward to generalized method
    nfa build_concatenation(const std::string &input) const {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        nfa result = build_literal(input[0]);
        for (size_t i = 1; i < input.size(); ++i) {
            result = build_concatenation(result, build_literal(input[i]));
        }
        return result;
    }

    // Generalized Zero or More: Kleene Star
    nfa build_zero_or_more(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA
        start->transitions.emplace_back(accept); // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(input_nfa.start); // Loop back
        input_nfa.accept->transitions.emplace_back(accept); // ε-transition to accept state

        return {start, accept};
    }

    // Generalized One or More
    nfa build_one_or_more(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(input_nfa.start); // Loop back
        input_nfa.accept->transitions.emplace_back(accept); // ε-transition to accept state

        return {start, accept};
    }

    // Character-based One or More: Forward to generalized method
    nfa build_one_or_more(char c) const {
        return build_one_or_more(build_literal(c));
    }

    // Generalized Optionality
    nfa build_optionality(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA
        start->transitions.emplace_back(accept); // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(accept); // ε-transition to accept state

        return {start, accept};
    }

    // Generalized Character Class
    nfa build_character_class(const std::unordered_set<char> &char_set, bool negated = false) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(char_set, accept, negated);
        return {start, accept};
    }

    // String-based Character Class
    nfa build_character_class(const std::string &input) const {
        const bool is_negated = input[0] == '^';
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
        return build_character_class(char_set, is_negated);
    }

    nfa build_group(const nfa &input_nfa, const std::string &name = "") {
        const int group_index = group_counter++; // Assign a group number

        input_nfa.start->group_start_index = group_index;
        input_nfa.accept->group_end_index = group_index;

        if (!name.empty()) {
            named_groups[name] = group_index; // Map group name to index
        }

        return input_nfa;
    }

    // Non-Capturing Group
    nfa build_non_capturing_group(const nfa &input_nfa) const {
        return input_nfa; // Simply return the input NFA without modifying group indices
    }
};

// NFA Exec Result
struct execute_results {
    bool matched = false;
    std::vector<std::string> groups;
    std::unordered_map<std::string, std::string> named_groups; // Named group matches
};

// NFA Processor
class nfa_processor {
public:
    static execute_results run(const build_result &result, const std::string &input) {
    const auto &automaton = result.automaton;
    const auto &named_groups = result.named_groups;

    // Same logic as before, but using result.named_groups
    std::queue<std::tuple<std::shared_ptr<state>, size_t, std::stack<std::pair<size_t, int>>, std::vector<std::string>>>
        to_process;
    std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

    std::vector<std::string> captured_groups(1); // Group 0 (whole match)
    std::stack<std::pair<size_t, int>> group_stack;

    to_process.emplace(automaton.start, 0, group_stack, captured_groups);

    while (!to_process.empty()) {
        auto [current, pos, groups, captures] = to_process.front();
        to_process.pop();

        if (!visited.insert({current, pos}).second) continue;

        if (current->is_accept && pos == input.size()) {
            captures[0] = input;
            std::unordered_map<std::string, std::string> named_captures;

            for (const auto &[name, index] : named_groups) {
                if (index < captures.size()) {
                    named_captures[name] = captures[index];
                }
            }
            return {true, captures, named_captures};
        }

        for (const auto &t : current->transitions) {
            auto new_groups = groups;
            auto new_captures = captures;

            if (current->group_start_index != -1) {
                new_groups.emplace(pos, current->group_start_index);
            }
            if (current->group_end_index != -1 && !new_groups.empty()) {
                auto [start_pos, group_index] = new_groups.top();
                new_groups.pop();

                if (new_captures.size() <= group_index) {
                    new_captures.resize(group_index + 1);
                }
                new_captures[group_index] = input.substr(start_pos, pos - start_pos);
            }

            if (t.type == EPSILON) {
                to_process.emplace(t.target, pos, new_groups, new_captures);
            } else if (pos < input.size() && t.matches(input[pos])) {
                to_process.emplace(t.target, pos + 1, new_groups, new_captures);
            }
        }
    }

    return {false, {}, {}};
}
private:
    struct pair_hash {
        template<class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};

// DOT Visualization
void visualize_nfa_dot(const nfa &nfa, std::ostream &out) {
    out << "digraph NFA {\n  rankdir=LR;\n  node [shape=circle];\n  start [shape=point];\n";
    out << "  start -> " << nfa.start->id << " [label=\"ε\"];\n";

    std::queue<std::shared_ptr<state> > to_process;
    std::unordered_set<int> visited;
    to_process.push(nfa.start);
    visited.insert(nfa.start->id);

    while (!to_process.empty()) {
        const auto current = to_process.front();
        to_process.pop();

        if (current->is_accept) out << "  " << current->id << " [shape=doublecircle];\n";

        for (const auto &t: current->transitions) {
            std::string label = (t.type == EPSILON)
                                    ? "ε"
                                    : (t.type == LITERAL)
                                          ? std::string(1, t.literal)
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
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_concatenation("ab"));

    visualize_nfa_dot(result.automaton, std::cout);

    auto [matched, captured_groups, named_captures] = nfa_processor::run(result, "ab");
    EXPECT_TRUE(matched);
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);
    EXPECT_FALSE(nfa_processor::run(result, "abc").matched);
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_zero_or_more(builder.build_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_one_or_more('a'));

    EXPECT_FALSE(nfa_processor::run(result, "").matched);        // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_optionality(builder.build_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_FALSE(nfa_processor::run(result, "aa").matched);      // Too many occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Range) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_character_class("a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Lower boundary
    EXPECT_TRUE(nfa_processor::run(result, "m").matched);        // Middle of the range
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Upper boundary
    EXPECT_FALSE(nfa_processor::run(result, "A").matched);       // Out of range
    EXPECT_FALSE(nfa_processor::run(result, "1").matched);       // Non-alphabetic character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_character_class("abc"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Matches 'a'
    EXPECT_TRUE(nfa_processor::run(result, "b").matched);        // Matches 'b'
    EXPECT_TRUE(nfa_processor::run(result, "c").matched);        // Matches 'c'
    EXPECT_FALSE(nfa_processor::run(result, "d").matched);       // Not in the set
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Not in the set
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedRange) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_character_class("^a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "A").matched);        // Uppercase letter
    EXPECT_TRUE(nfa_processor::run(result, "1").matched);        // Digit
    EXPECT_TRUE(nfa_processor::run(result, "!").matched);        // Special character
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Lowercase letter in range
    EXPECT_FALSE(nfa_processor::run(result, "m").matched);       // Middle of the range
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Upper boundary of range
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.build_character_class("^abc"));

    EXPECT_TRUE(nfa_processor::run(result, "d").matched);        // Outside explicit set
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Another character outside the set
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "c").matched);       // Matches negated set
}

TEST(NFA_Processor_Test, Execute_With_Groups_Single_Test) {
    nfa_builder builder;

    const build_result result = builder.complete(
        builder.build_zero_or_more(
            builder.build_group(
                builder.build_concatenation(builder.build_literal('a'), builder.build_literal('b'))
            )
        )
    );

    visualize_nfa_dot(result.automaton, std::cout);

    auto [matched, captured_groups, ignore] = nfa_processor::run(result, "ababab");

    EXPECT_TRUE(matched);
    EXPECT_EQ(captured_groups.size(), 2);       // Whole match + last group match
    EXPECT_EQ(captured_groups[0], "ababab");    // Whole match
    EXPECT_EQ(captured_groups[1], "ab");        // Last group match
}

TEST(NFA_Builder_Test, Build_NonCapturingGroup) {
    nfa_builder builder;
    const build_result result = builder.complete(
        builder.build_one_or_more(
            builder.build_non_capturing_group(
                builder.build_concatenation(builder.build_literal('a'), builder.build_literal('b'))
            )
        )
    );

    EXPECT_TRUE(nfa_processor::run(result, "ab").matched);       // Single occurrence
    EXPECT_TRUE(nfa_processor::run(result, "abab").matched);     // Two occurrences
    EXPECT_TRUE(nfa_processor::run(result, "ababab").matched);   // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "").matched);        // Zero occurrences
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Incomplete match
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid start
    EXPECT_FALSE(nfa_processor::run(result, "abx").matched);     // Ends with invalid character
}

TEST(NFA_Processor_Test, Execute_With_Named_Groups) {
    nfa_builder builder;

    const build_result result = builder.complete(
        builder.build_zero_or_more(
            builder.build_group(
                builder.build_concatenation(builder.build_literal('a'), builder.build_literal('b')),
                "word"
            )
        )
    );

    auto [matched, captured_groups, named_captures] = nfa_processor::run(result, "ababab");

    EXPECT_TRUE(matched);
    EXPECT_EQ(named_captures.at("word"), "ab"); // Last match for the named group
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
