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
    static int next_id; // Static counter for unique IDs
    int id;
    bool is_accept = false;
    int group_id = -1;
    std::vector<std::pair<char, std::shared_ptr<State>>> transitions;
    StateType type = NORMAL;

    explicit State(bool is_accept = false, int group_id = -1, StateType type = NORMAL)
        : id(next_id++), is_accept(is_accept), group_id(group_id), type(type) {}

    // Clone method that assigns new unique IDs
    std::shared_ptr<State> clone() const {
        auto new_state = std::make_shared<State>();
        new_state->is_accept = this->is_accept;
        new_state->group_id = this->group_id;
        new_state->type = this->type;

        // Recursively clone transitions
        for (const auto &[symbol, next] : this->transitions) {
            new_state->transitions.emplace_back(symbol, next->clone());
        }

        return new_state;
    }
};

// Initialize static counter
int State::next_id = 0;

struct NFA {
    std::shared_ptr<State> start;
    std::shared_ptr<State> accept;
};

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

class NFABuilder {
public:
    static NFA build_literal(const char c) {
        const auto start = create_state();
        const auto accept = create_state(true);
        add_transition(start, accept, c);
        return {start, accept};
    }

    static NFA concatenate(const NFA& first, const NFA& second) {
        add_transition(first.accept, second.start, '\0'); // ε-transition
        first.accept->is_accept = false;
        return {first.start, second.accept};
    }

    static NFA alternation(const NFA& first, const NFA& second) {
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

    static NFA repetition(const NFA& nfa, bool one_or_more = false) {
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

    static NFA begin_group(const int group_id) {
        const auto start = create_state();
        start->type = BEGIN_GROUP;
        start->group_id = group_id;

        const auto accept = create_state();
        add_transition(start, accept, '\0'); // ε-transition
        return {start, accept};
    }

    static NFA end_group(const int group_id) {
        const auto start = create_state();
        start->type = END_GROUP;
        start->group_id = group_id;

        const auto accept = create_state();
        add_transition(start, accept, '\0'); // ε-transition
        return {start, accept};
    }

private:
    static std::shared_ptr<State> create_state(const bool is_accept = false) {
        return std::make_shared<State>(State{is_accept, -1, {}});
    }

    static void add_transition(const std::shared_ptr<State>& from, const std::shared_ptr<State>& to, char symbol) {
        from->transitions.emplace_back(symbol, to);
        //std::cout << "Transition added from state " << from->id << " to state " << to->id << " with symbol '" << symbol << "'\n";
    }
};


void visualize_nfa_dot(const NFA& nfa, std::ostream& out) {
    out << "https://dreampuf.github.io/GraphvizOnline/?engine=do\n";
    out << "\n";
    out << "digraph NFA {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=circle];\n";

    std::queue<std::shared_ptr<State>> to_process;
    std::unordered_set<int> visited;

    to_process.push(nfa.start);
    visited.insert(nfa.start->id);

    while (!to_process.empty()) {
        const auto current = to_process.front();
        to_process.pop();

        if (current->is_accept) {
            out << "  " << current->id << " [shape=doublecircle];\n";
        }

        for (const auto& [symbol, next] : current->transitions) {
            out << "  " << current->id << " -> " << next->id << " [label=\""
                << (symbol == '\0' ? "ε" : std::string(1, symbol)) << "\"];\n";
            if (visited.insert(next->id).second) {
                to_process.push(next);
            }
        }
    }

    out << "}\n";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


TEST(StateTest, CloneMethod) {
    // Create an original state
    auto original = std::make_shared<State>(true, 1, BEGIN_GROUP);
    original->transitions.emplace_back('a', std::make_shared<State>());

    // Clone the state
    auto clone = original->clone();

    // Check that the cloned state has the same properties
    EXPECT_EQ(clone->is_accept, original->is_accept);
    EXPECT_EQ(clone->group_id, original->group_id);
    EXPECT_EQ(clone->type, original->type);
    EXPECT_EQ(clone->transitions.size(), original->transitions.size());

    // Check that the cloned state has a different ID
    EXPECT_NE(clone->id, original->id);

    // Check that the transitions are cloned correctly
    EXPECT_EQ(clone->transitions[0].first, original->transitions[0].first);
    EXPECT_NE(clone->transitions[0].second->id, original->transitions[0].second->id);
}

TEST(NFATest, LiteralMatch) {
    // "a"
    const NFA nfa = NFABuilder::build_literal('a');

    EXPECT_TRUE(execute_nfa(nfa, "a"));
    EXPECT_FALSE(execute_nfa(nfa, "b"));
}

TEST(NFATest, Concatenation) {
    // "ab"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA ab = NFABuilder::concatenate(a, b);

    EXPECT_TRUE(execute_nfa(ab, "ab"));
    EXPECT_FALSE(execute_nfa(ab, "a"));
    EXPECT_FALSE(execute_nfa(ab, "b"));
}

TEST(NFATest, Alternation) {
    // "a|b"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA a_or_b = NFABuilder::alternation(a, b);

    visualize_nfa_dot(a_or_b, std::cout);

    EXPECT_TRUE(execute_nfa(a_or_b, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b, "b"));
    EXPECT_FALSE(execute_nfa(a_or_b, "c"));
}

TEST(NFATest, Repetition) {
    // "a*"
    const NFA a = NFABuilder::build_literal('a');
    const NFA a_star = NFABuilder::repetition(a);

    EXPECT_TRUE(execute_nfa(a_star, ""));
    EXPECT_TRUE(execute_nfa(a_star, "a"));
    EXPECT_TRUE(execute_nfa(a_star, "aaa"));
    EXPECT_FALSE(execute_nfa(a_star, "b"));
}

TEST(NFATest, PatternAPlusBCStar) {
    // Regex: "a+bc*"
    const NFA a = NFABuilder::build_literal('a');
    const NFA a_plus = NFABuilder::repetition(a, true); // One or more 'a'
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');
    const NFA c_star = NFABuilder::repetition(c); // Zero or more 'c'
    const NFA bc_star = NFABuilder::concatenate(b, c_star);
    const NFA pattern = NFABuilder::concatenate(a_plus, bc_star);

    visualize_nfa_dot(pattern, std::cout);
    // Expected Behavior:
    // Input: "a", Should fail (needs at least one 'b')
    // Input: "ab", Should pass
    // Input: "abc", Should pass
    // Input: "aab", Should pass
    // Input: "aabc", Should pass
    // Input: "aabcc", Should pass
    // Input: "b", Should fail (no 'a')
    // Input: "ac", Should fail (needs 'b' after 'a')

    EXPECT_FALSE(execute_nfa(pattern, "a"));
    EXPECT_TRUE(execute_nfa(pattern, "ab"));
    EXPECT_TRUE(execute_nfa(pattern, "abc"));
    EXPECT_TRUE(execute_nfa(pattern, "aab"));
    EXPECT_TRUE(execute_nfa(pattern, "aabc"));
    EXPECT_TRUE(execute_nfa(pattern, "aabcc"));
    EXPECT_FALSE(execute_nfa(pattern, "b"));
    EXPECT_FALSE(execute_nfa(pattern, "ac"));
}

TEST(NFATest, ComplexConcatenation) {
    // Regex: "abc"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');
    const NFA abc = NFABuilder::concatenate(NFABuilder::concatenate(a, b), c);

    EXPECT_TRUE(execute_nfa(abc, "abc"));
    EXPECT_FALSE(execute_nfa(abc, "ab"));
    EXPECT_FALSE(execute_nfa(abc, "abcd"));
    EXPECT_FALSE(execute_nfa(abc, ""));
}

TEST(NFATest, NestedAlternation) {
    // Regex: "a|(b|c)"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');
    const NFA b_or_c = NFABuilder::alternation(b, c);
    const NFA a_or_b_or_c = NFABuilder::alternation(a, b_or_c);

    visualize_nfa_dot(a_or_b_or_c, std::cout);

    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "a"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "b"));
    EXPECT_TRUE(execute_nfa(a_or_b_or_c, "c"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, "d"));
    EXPECT_FALSE(execute_nfa(a_or_b_or_c, ""));
}

TEST(NFATest, ComplexRepetition) {
    // Regex: "a*(b|c)"
    const NFA a = NFABuilder::build_literal('a');
    const NFA a_star = NFABuilder::repetition(a);
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');
    const NFA b_or_c = NFABuilder::alternation(b, c);
    const NFA pattern = NFABuilder::concatenate(a_star, b_or_c);

    visualize_nfa_dot(pattern, std::cout);

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
    // Regex: "(a|b)"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA group_start = NFABuilder::begin_group(1);
    const NFA group_end = NFABuilder::end_group(1);

    const NFA alt = NFABuilder::alternation(a, b);
    const NFA group = NFABuilder::concatenate(group_start, NFABuilder::concatenate(alt, group_end));

    group.accept->is_accept = true;

    visualize_nfa_dot(group, std::cout);

    EXPECT_TRUE(execute_nfa(group, "a")); // Should capture group 1: "a"
    EXPECT_TRUE(execute_nfa(group, "b")); // Should capture group 1: "b"
    EXPECT_FALSE(execute_nfa(group, "c"));
}

TEST(NFATest, GroupCaptureAdvanced) {
    // Regex: ""(a|b)c*"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');
    const NFA group_start = NFABuilder::begin_group(1);
    const NFA group_end = NFABuilder::end_group(1);

    const NFA a_or_b = NFABuilder::alternation(a, b);
    const NFA group = NFABuilder::concatenate(group_start, NFABuilder::concatenate(a_or_b, group_end));
    const NFA c_star = NFABuilder::repetition(c);
    const NFA pattern = NFABuilder::concatenate(group, c_star);

    visualize_nfa_dot(pattern, std::cout);

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
    // Regex: ""(a(b))c"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA c = NFABuilder::build_literal('c');

    const NFA group1_start = NFABuilder::begin_group(1);
    const NFA group1_end = NFABuilder::end_group(1);

    const NFA group2_start = NFABuilder::begin_group(2);
    const NFA group2_end = NFABuilder::end_group(2);

    // Build nested groups
    const NFA inner_group = NFABuilder::concatenate(group2_start, NFABuilder::concatenate(b, group2_end));
    const NFA outer_group = NFABuilder::concatenate(group1_start, NFABuilder::concatenate(a, NFABuilder::concatenate(inner_group, group1_end)));
    const NFA pattern = NFABuilder::concatenate(outer_group, c);

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
    // Regex: ""(a|b)+"
    const NFA a = NFABuilder::build_literal('a');
    const NFA b = NFABuilder::build_literal('b');
    const NFA group_start = NFABuilder::begin_group(1);
    const NFA group_end = NFABuilder::end_group(1);

    const NFA a_or_b = NFABuilder::alternation(a, b);
    const NFA group = NFABuilder::concatenate(group_start, NFABuilder::concatenate(a_or_b, group_end));
    const NFA repeated_group = NFABuilder::repetition(group, true); // Use one_or_more = true

    visualize_nfa_dot(repeated_group, std::cout);

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
    const NFA empty_nfa = NFABuilder::build_literal('\0');
    empty_nfa.accept->is_accept = true;

    EXPECT_TRUE(execute_nfa(empty_nfa, ""));
    EXPECT_FALSE(execute_nfa(empty_nfa, "a"));
}
