#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>


// Transition object
struct transition {
    std::shared_ptr<struct state> target;
    explicit transition(std::shared_ptr<state> t) : target(std::move(t)) {}
    virtual ~transition() = default;

    virtual bool matches(char) const { return false; } // Default: No match
};


struct epsilon_transition : transition {
    explicit epsilon_transition(std::shared_ptr<state> t) : transition(std::move(t)) {}
    bool matches(char) const override { return true; }
};


struct literal_transition : transition {
    char literal;
    literal_transition(char c, std::shared_ptr<state> t)
        : transition(std::move(t)), literal(c) {}
    bool matches(char input) const override { return input == literal; }
};


struct character_class_transition : transition {
    std::unordered_set<char> char_set;
    character_class_transition(const std::unordered_set<char>& set, std::shared_ptr<state> t)
        : transition(std::move(t)), char_set(set) {}
    bool matches(char input) const override { return char_set.count(input) > 0; }
};


struct negated_class_transition : transition {
    std::unordered_set<char> char_set;
    negated_class_transition(const std::unordered_set<char>& set, std::shared_ptr<state> t)
        : transition(std::move(t)), char_set(set) {}
    bool matches(char input) const override { return char_set.count(input) == 0; }
};

// State of an NFA
struct state {
    static int next_id;
    int id;
    bool is_accept = false;
    int group_start_index = -1;
    int group_end_index = -1;
    std::vector<std::shared_ptr<transition>> transitions;

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
    build_result complete(const nfa &input_nfa) const {
        return {input_nfa, named_groups};
    }

    // Single Character NFA
    nfa add_literal(char c) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);
        start->transitions.emplace_back(std::make_shared<literal_transition>(c, accept));
        return {start, accept};
    }

    // Generalized Concatenation: Combine two NFAs
    nfa add_concatenation(const nfa &nfa1, const nfa &nfa2) const {
        nfa1.accept->is_accept = false; // Remove accept status from first NFA
        nfa1.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(nfa2.start));
        return {nfa1.start, nfa2.accept};
    }

    // Character-based Concatenation: Forward to generalized method
    nfa add_concatenation(const std::string &input) const {
        if (input.empty()) throw std::invalid_argument("Empty input for concatenation.");

        nfa result = add_literal(input[0]);
        for (size_t i = 1; i < input.size(); ++i) {
            result = add_concatenation(result, add_literal(input[i]));
        }
        return result;
    }

    // Generalized Zero or More: Kleene Star
    nfa add_zero_or_more(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(std::make_shared<epsilon_transition>(input_nfa.start)); // ε-transition to input NFA
        start->transitions.emplace_back(std::make_shared<epsilon_transition>(accept)); // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(input_nfa.start)); // Loop back
        input_nfa.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(accept)); // ε-transition to accept state

        return {start, accept};
    }

    // Generalized One or More
    nfa add_one_or_more(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(std::make_shared<epsilon_transition>(input_nfa.start));

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(input_nfa.start)); // Loop back
        input_nfa.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(accept));

        return {start, accept};
    }

    // Character-based One or More: Forward to generalized method
    nfa add_one_or_more(char c) const {
        return add_one_or_more(add_literal(c));
    }

    // Generalized Optionality
    nfa add_optionality(const nfa &input_nfa) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(std::make_shared<epsilon_transition>(input_nfa.start)); // ε-transition to input NFA
        start->transitions.emplace_back(std::make_shared<epsilon_transition>(accept)); // ε-transition to accept state

        input_nfa.accept->is_accept = false;
        input_nfa.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(accept)); // ε-transition to accept state

        return {start, accept};
    }

    // Generalized Character Class
    nfa add_character_class(const std::unordered_set<char> &char_set, bool negated = false) const {
        const auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        if (negated) {
            start->transitions.emplace_back(std::make_shared<negated_class_transition>(char_set, accept));
        } else {
            start->transitions.emplace_back(std::make_shared<character_class_transition>(char_set, accept));
        }
        return {start, accept};
    }

    // String-based Character Class
    nfa add_character_class(const std::string &input) const {
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
        return add_character_class(char_set, is_negated);
    }

    // Add a capturing group
    nfa add_group(const nfa &input_nfa, const std::string &name = "") {
        const int group_index = group_counter++; // Assign a group number

        input_nfa.start->group_start_index = group_index;
        input_nfa.accept->group_end_index = group_index;

        if (!name.empty()) {
            named_groups[name] = group_index; // Map group name to index
        }

        return input_nfa;
    }

    // Non-Capturing Group
    nfa add_non_capturing_group(const nfa &input_nfa) const {
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
    // Run NFA and return results (standard run with named groups and captures)
    static execute_results run(const build_result &result, const std::string &input) {
        const auto &automaton = result.automaton;
        const auto &named_groups = result.named_groups;

        // Processing queue: state, position, group stack, captures
        std::queue<std::tuple<std::shared_ptr<state>, size_t, std::stack<std::pair<size_t, int>>, std::vector<std::string>>>
            to_process;

        // Visited states: avoid revisiting state-position pairs
        std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

        std::vector<std::string> captured_groups(1); // Group 0: whole match
        std::stack<std::pair<size_t, int>> group_stack;

        to_process.emplace(automaton.start, 0, group_stack, captured_groups);

        while (!to_process.empty()) {
            auto [current, pos, groups, captures] = to_process.front();
            to_process.pop();

            // Skip visited state-position pairs
            if (!visited.insert({current, pos}).second) continue;

            // Check if the input is consumed and we're at an accepting state
            if (current->is_accept && pos == input.size()) {
                captures[0] = input; // Full match
                std::unordered_map<std::string, std::string> named_captures;

                // Extract named groups
                for (const auto &[name, index] : named_groups) {
                    if (index < captures.size()) {
                        named_captures[name] = captures[index];
                    }
                }
                return {true, captures, named_captures};
            }

            // Process transitions
            for (const auto &t : current->transitions) {
                auto new_groups = groups;
                auto new_captures = captures;

                // Handle group starts
                if (current->group_start_index != -1) {
                    new_groups.emplace(pos, current->group_start_index);
                }

                // Handle group ends
                if (current->group_end_index != -1 && !new_groups.empty()) {
                    auto [start_pos, group_index] = new_groups.top();
                    new_groups.pop();

                    if (new_captures.size() <= group_index) {
                        new_captures.resize(group_index + 1);
                    }
                    new_captures[group_index] = input.substr(start_pos, pos - start_pos);
                }

                // Process epsilon transitions
                if (auto epsilon = std::dynamic_pointer_cast<epsilon_transition>(t)) {
                    to_process.emplace(epsilon->target, pos, new_groups, new_captures);
                }
                // Process matching transitions
                else if (pos < input.size() && t->matches(input[pos])) {
                    to_process.emplace(t->target, pos + 1, new_groups, new_captures);
                }
            }
        }

        return {false, {}, {}}; // No match
    }

private:
    // Hash function for visited state-position pairs
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};

// DOT Visualization
inline void visualize_nfa_dot(const nfa &nfa, std::ostream &out) {
    out << "digraph NFA {\n  rankdir=LR;\n  node [shape=circle];\n  start [shape=point];\n";
    out << "  start -> " << nfa.start->id << " [label=\"ε\"];\n";

    std::queue<std::shared_ptr<state>> to_process;
    std::unordered_set<int> visited;

    to_process.push(nfa.start);
    visited.insert(nfa.start->id);

    while (!to_process.empty()) {
        const auto current = to_process.front();
        to_process.pop();

        // Double circle for accept states
        if (current->is_accept) {
            out << "  " << current->id << " [shape=doublecircle];\n";
        }

        // Process transitions
        for (const auto &t : current->transitions) {
            std::string label;

            // Determine transition type and label
            if (std::dynamic_pointer_cast<epsilon_transition>(t)) {
                label = "ε";
            } else if (auto lt = std::dynamic_pointer_cast<literal_transition>(t)) {
                label = std::string(1, lt->literal);
            } else if (auto ct = std::dynamic_pointer_cast<character_class_transition>(t)) {
                label = "class";
            } else if (auto nct = std::dynamic_pointer_cast<negated_class_transition>(t)) {
                label = "negated class";
            } else {
                label = "unknown";
            }

            // Output edge
            out << "  " << current->id << " -> " << t->target->id
                << " [label=\"" << label << "\"];\n";

            // Enqueue unvisited states
            if (visited.insert(t->target->id).second) {
                to_process.push(t->target);
            }
        }
    }

    out << "}\n";
}