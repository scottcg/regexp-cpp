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
    // Single Character NFA
    nfa build_single_character(char c) const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(c, accept);
        return {start, accept};
    }

    // Generalized Concatenation: Combine two NFAs
    nfa build_concatenation(const nfa &nfa1, const nfa &nfa2) const {
        nfa1.accept->is_accept = false;  // Remove accept status from first NFA
        nfa1.accept->transitions.emplace_back(nfa2.start); // Link first NFA to second NFA
        return {nfa1.start, nfa2.accept};
    }

    // Character-based Concatenation: Forward to generalized method
    nfa build_concatenation(const std::string &input) const {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        nfa result = build_single_character(input[0]);
        for (size_t i = 1; i < input.size(); ++i) {
            result = build_concatenation(result, build_single_character(input[i]));
        }
        return result;
    }

    // Generalized Zero or More: Kleene Star
    nfa build_zero_or_more(const nfa &input_nfa) const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA
        start->transitions.emplace_back(accept);         // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(input_nfa.start); // Loop back
        input_nfa.accept->transitions.emplace_back(accept);          // ε-transition to accept state

        return {start, accept};
    }

    // Character-based Zero or More: Forward to generalized method
    nfa build_zero_or_more(char c) const {
        return build_zero_or_more(build_single_character(c));
    }

    // Generalized One or More
    nfa build_one_or_more(const nfa &input_nfa) const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(input_nfa.start); // Loop back
        input_nfa.accept->transitions.emplace_back(accept);          // ε-transition to accept state

        return {start, accept};
    }

    // Character-based One or More: Forward to generalized method
    nfa build_one_or_more(char c) const {
        return build_one_or_more(build_single_character(c));
    }

    // Generalized Optionality
    nfa build_optionality(const nfa &input_nfa) const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(input_nfa.start); // ε-transition to input NFA
        start->transitions.emplace_back(accept);          // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(accept); // ε-transition to accept state

        return {start, accept};
    }

    // Character-based Optionality: Forward to generalized method
    nfa build_optionality(char c) const {
        return build_optionality(build_single_character(c));
    }

    // Generalized Character Class
    nfa build_character_class(const std::unordered_set<char> &char_set, bool negated = false) const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(char_set, accept, negated);
        return {start, accept};
    }

    // String-based Character Class
    nfa build_character_class(const std::string &input) const {
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
        return build_character_class(char_set, is_negated);
    }
};

// NFA Processor
class nfa_processor {
public:
    static bool execute(const nfa &nfa, const std::string &input, bool debug = false) {
    std::queue<std::pair<std::shared_ptr<state>, size_t>> to_process;
    std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

    to_process.emplace(nfa.start, 0);

    if (debug) {
        std::cout << "Starting NFA Execution\n";
        std::cout << "Input: \"" << input << "\"\n";
    }

    while (!to_process.empty()) {
        auto [current, pos] = to_process.front();
        to_process.pop();

        if (debug) {
            std::cout << "Current State: " << current->id
                      << " | Input Position: " << pos
                      << " | Accept State: " << (current->is_accept ? "YES" : "NO") << "\n";
        }

        // Check for success
        if (current->is_accept && pos == input.size()) {
            if (debug) {
                std::cout << "Matched: Reached accepting state with all input consumed.\n";
            }
            return true;
        }

        // Check if this state and position have already been visited
        if (!visited.insert({current, pos}).second) {
            if (debug) {
                std::cout << "State " << current->id << " at position " << pos << " already visited. Skipping.\n";
            }
            continue;
        }

        // Explore transitions
        for (const auto &t : current->transitions) {
            if (t.type == EPSILON) {
                if (debug) {
                    std::cout << "  Transition: ε -> State " << t.target->id << "\n";
                }
                to_process.emplace(t.target, pos);
            } else if (pos < input.size() && t.matches(input[pos])) {
                if (debug) {
                    std::cout << "  Transition: '" << input[pos] << "' -> State " << t.target->id << "\n";
                }
                to_process.emplace(t.target, pos + 1);
            } else if (debug) {
                std::cout << "  No transition for input '" << input[pos] << "' at State " << current->id << "\n";
            }
        }
    }

    if (debug) {
        std::cout << "Failed: No valid path to an accept state.\n";
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
    nfa_builder builder;
    nfa nfa = builder.build_concatenation("ab");
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, "ab"));
    EXPECT_FALSE(processor.execute(nfa, "a"));
    EXPECT_FALSE(processor.execute(nfa, "b"));
    EXPECT_FALSE(processor.execute(nfa, "abc"));
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_zero_or_more('a');
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, ""));       // Zero occurrences
    EXPECT_TRUE(processor.execute(nfa, "a"));      // One occurrence
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));   // Multiple occurrences
    EXPECT_FALSE(processor.execute(nfa, "b"));     // Invalid character
    EXPECT_FALSE(processor.execute(nfa, "aaab"));  // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa_builder builder;
    nfa nfa = builder.build_one_or_more('a');
    nfa_processor processor;

    EXPECT_FALSE(processor.execute(nfa, ""));      // Zero occurrences
    EXPECT_TRUE(processor.execute(nfa, "a"));      // One occurrence
    EXPECT_TRUE(processor.execute(nfa, "aaaa"));   // Multiple occurrences
    EXPECT_FALSE(processor.execute(nfa, "b"));     // Invalid character
    EXPECT_FALSE(processor.execute(nfa, "aaab"));  // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa_builder builder;
    nfa nfa = builder.build_optionality('a');
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, ""));       // Zero occurrences
    EXPECT_TRUE(processor.execute(nfa, "a"));      // One occurrence
    EXPECT_FALSE(processor.execute(nfa, "aa"));    // Too many occurrences
    EXPECT_FALSE(processor.execute(nfa, "b"));     // Invalid character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Range) {
    nfa_builder builder;
    nfa nfa = builder.build_character_class("a-z");
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, "a"));      // Lower boundary
    EXPECT_TRUE(processor.execute(nfa, "m"));      // Middle of the range
    EXPECT_TRUE(processor.execute(nfa, "z"));      // Upper boundary
    EXPECT_FALSE(processor.execute(nfa, "A"));     // Out of range
    EXPECT_FALSE(processor.execute(nfa, "1"));     // Non-alphabetic character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_builder builder;
    nfa nfa = builder.build_character_class("abc");
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, "a"));      // Matches 'a'
    EXPECT_TRUE(processor.execute(nfa, "b"));      // Matches 'b'
    EXPECT_TRUE(processor.execute(nfa, "c"));      // Matches 'c'
    EXPECT_FALSE(processor.execute(nfa, "d"));     // Not in the set
    EXPECT_FALSE(processor.execute(nfa, "z"));     // Not in the set
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedRange) {
    nfa_builder builder;
    nfa nfa = builder.build_character_class("^a-z");
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, "A"));      // Uppercase letter
    EXPECT_TRUE(processor.execute(nfa, "1"));      // Digit
    EXPECT_TRUE(processor.execute(nfa, "!"));      // Special character
    EXPECT_FALSE(processor.execute(nfa, "a"));     // Lowercase letter in range
    EXPECT_FALSE(processor.execute(nfa, "m"));     // Middle of the range
    EXPECT_FALSE(processor.execute(nfa, "z"));     // Upper boundary of range
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_builder builder;
    nfa nfa = builder.build_character_class("^abc");
    nfa_processor processor;

    EXPECT_TRUE(processor.execute(nfa, "d"));      // Outside explicit set
    EXPECT_TRUE(processor.execute(nfa, "z"));      // Another character outside the set
    EXPECT_FALSE(processor.execute(nfa, "a"));     // Matches negated set
    EXPECT_FALSE(processor.execute(nfa, "b"));     // Matches negated set
    EXPECT_FALSE(processor.execute(nfa, "c"));     // Matches negated set
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
