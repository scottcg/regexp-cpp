#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <string>

// Base Transition Class
struct transition {
    std::shared_ptr<struct state> target;

    explicit transition(std::shared_ptr<state> t) : target(std::move(t)) {}
    virtual ~transition() = default;

    // Virtual method for matching input or checking position
    virtual bool matches(char input, size_t pos, size_t input_size) const {
        return false; // Default: No match
    }

    // Virtual method for getting a label for visualization
    virtual std::string label() const {
        return "unknown";
    }
};

// Epsilon Transition (ε-transition)
struct epsilon_transition : transition {
    explicit epsilon_transition(std::shared_ptr<state> t) : transition(std::move(t)) {}

    bool matches(char, size_t, size_t) const override {
        return true; // Always matches
    }

    std::string label() const override {
        return "ε";
    }
};

// Literal Transition
struct literal_transition : transition {
    char literal;

    literal_transition(char c, std::shared_ptr<state> t)
        : transition(std::move(t)), literal(c) {}

    bool matches(char input, size_t, size_t) const override {
        return input == literal; // Match the specific character
    }

    std::string label() const override {
        return std::string(1, literal);
    }
};

// Character Class Transition
struct character_class_transition : transition {
    std::unordered_set<char> char_set;

    character_class_transition(const std::unordered_set<char>& set, std::shared_ptr<state> t)
        : transition(std::move(t)), char_set(set) {}

    bool matches(char input, size_t, size_t) const override {
        return char_set.count(input) > 0;
    }

    std::string label() const override {
        return "[class]";
    }
};

// Negated Character Class Transition
struct negated_class_transition : transition {
    std::unordered_set<char> char_set;

    negated_class_transition(const std::unordered_set<char>& set, std::shared_ptr<state> t)
        : transition(std::move(t)), char_set(set) {}

    bool matches(char input, size_t, size_t) const override {
        return char_set.count(input) == 0;
    }

    std::string label() const override {
        return "[^class]";
    }
};

// Start Anchor Transition (^): Match only at the start of input
struct start_anchor_transition : transition {
    explicit start_anchor_transition(std::shared_ptr<state> t) : transition(std::move(t)) {}

    bool matches(char, size_t pos, size_t) const override {
        return pos == 0; // Match if position is at the start of input
    }

    std::string label() const override {
        return "^";
    }
};

// End Anchor Transition ($): Match only at the end of input
struct end_anchor_transition : transition {
    explicit end_anchor_transition(std::shared_ptr<state> t) : transition(std::move(t)) {}

    bool matches(char, size_t pos, size_t input_size) const override {
        return pos == input_size; // Match if position is at the end of input
    }

    std::string label() const override {
        return "$";
    }
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


class builder {
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
        // Directly connect the first NFA's accept state to the second NFA's start state
        nfa1.accept->is_accept = false; // Remove accept status from the first NFA
        nfa1.accept->transitions.insert(
            nfa1.accept->transitions.end(),
            nfa2.start->transitions.begin(),
            nfa2.start->transitions.end()
        );

        // Return the combined NFA
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

    // Alternation: (a|b)
    nfa add_alternation(const nfa &nfa1, const nfa &nfa2) const {
        // Create new start and accept states
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        // Redirect transitions to skip unnecessary intermediate steps
        for (const auto &transition : nfa1.start->transitions) {
            start->transitions.push_back(transition);
        }
        for (const auto &transition : nfa2.start->transitions) {
            start->transitions.push_back(transition);
        }

        // Redirect the accepting states of nfa1 and nfa2
        for (const auto &transition : nfa1.accept->transitions) {
            accept->transitions.push_back(transition);
        }
        for (const auto &transition : nfa2.accept->transitions) {
            accept->transitions.push_back(transition);
        }

        // Ensure the new states are connected to preserve alternation logic
        nfa1.accept->is_accept = false;
        nfa1.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(accept));

        nfa2.accept->is_accept = false;
        nfa2.accept->transitions.emplace_back(std::make_shared<epsilon_transition>(accept));

        // Return the optimized NFA
        return {start, accept};
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
    nfa add_character_class_range(const std::string &input) const {
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

    // Start anchor (^)
    nfa add_start_anchor() const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(std::make_shared<start_anchor_transition>(accept));
        return {start, accept};
    }

    // End anchor ($)
    nfa add_end_anchor() const {
        auto start = std::make_shared<state>();
        auto accept = std::make_shared<state>(true);

        start->transitions.emplace_back(std::make_shared<end_anchor_transition>(accept));
        return {start, accept};
    }
};

// NFA Exec Result
struct execute_results {
    bool matched = false;
    std::vector<std::string> groups;
    std::unordered_map<std::string, std::string> named_groups; // Named group matches
};

class nfa_processor {
public:
    static execute_results run(const build_result &result, const std::string &input, bool debug = false) {
        const auto &automaton = result.automaton;
        const auto &named_groups = result.named_groups;

        std::queue<std::tuple<std::shared_ptr<state>, size_t, std::stack<std::pair<size_t, int>>, std::vector<std::string>>>
            to_process;
        std::unordered_set<std::pair<std::shared_ptr<state>, size_t>, pair_hash> visited;

        std::vector<std::string> captured_groups(1);
        std::stack<std::pair<size_t, int>> group_stack;

        to_process.emplace(automaton.start, 0, group_stack, captured_groups);

        while (!to_process.empty()) {
            auto [current, pos, groups, captures] = to_process.front();
            to_process.pop();

            if (debug) {
                std::cout << "Processing state: " << current->id << ", position: " << pos << std::endl;
            }

            if (!visited.insert({current, pos}).second) {
                if (debug) {
                    std::cout << "State " << current->id << " at position " << pos << " already visited, continuing." << std::endl;
                }
                continue;
            }

            if (current->is_accept && pos == input.size()) {
                if (debug) {
                    std::cout << "Accepting state " << current->id << " reached at end of input." << std::endl;
                }
                captures[0] = input;
                std::unordered_map<std::string, std::string> named_captures;

                for (const auto &[name, index] : named_groups) {
                    if (static_cast<size_t>(index) < captures.size()) {
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

                    if (new_captures.size() <= static_cast<size_t>(group_index)) {
                        new_captures.resize(group_index + 1);
                    }
                    new_captures[group_index] = input.substr(start_pos, pos - start_pos);
                }

                if (auto epsilon = std::dynamic_pointer_cast<epsilon_transition>(t)) {
                    if (debug) {
                        std::cout << "Epsilon transition to state: " << epsilon->target->id << std::endl;
                    }
                    to_process.emplace(epsilon->target, pos, new_groups, new_captures);
                } else if (auto start_anchor = std::dynamic_pointer_cast<start_anchor_transition>(t)) {
                    if (t->matches('\0', pos, input.size())) {
                        if (debug) {
                            std::cout << "Start anchor transition to state: " << start_anchor->target->id << std::endl;
                        }
                        to_process.emplace(start_anchor->target, pos, new_groups, new_captures);
                    }
                } else if (auto end_anchor = std::dynamic_pointer_cast<end_anchor_transition>(t)) {
                    if (t->matches('\0', pos, input.size())) {
                        if (debug) {
                            std::cout << "End anchor transition to state: " << end_anchor->target->id << std::endl;
                        }
                        to_process.emplace(end_anchor->target, pos, new_groups, new_captures);
                    }
                } else if (t->matches(input[pos], pos, input.size())) {
                    if (debug) {
                        std::cout << "Transition on '" << input[pos] << "' to state: " << t->target->id
                                  << " (type: " << typeid(*t).name() << ")" << std::endl;
                    }
                    to_process.emplace(t->target, pos + 1, new_groups, new_captures);
                }
            }
        }

        if (debug) {
            std::cout << "No match found." << std::endl;
        }
        return {false, {}, {}};
    }

private:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return std::hash<int>()(p.first->id) ^ std::hash<T2>()(p.second);
        }
    };
};
