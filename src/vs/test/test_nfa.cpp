#include "test_nfa.h"

#include <gtest/gtest.h>


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
            } else if (auto start_anchor = std::dynamic_pointer_cast<start_anchor_transition>(t)) {
                label = "^";
            } else if (auto end_anchor = std::dynamic_pointer_cast<end_anchor_transition>(t)) {
                label = "$";
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

TEST(NFA_Processor_Test, Alternation_Expression) {
    builder builder;

    // Build the expression: (a|b)
    nfa expression = builder.add_alternation(
        builder.add_literal('a'),
        builder.add_literal('b')
    );

    build_result result = builder.complete(expression);

    // Valid inputs
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);       // Matches "a"
    EXPECT_TRUE(nfa_processor::run(result, "b").matched);       // Matches "b"

    // Invalid inputs
    EXPECT_FALSE(nfa_processor::run(result, "c").matched);      // Does not match "c"
    EXPECT_FALSE(nfa_processor::run(result, "ab").matched);     // Does not match "ab"
    EXPECT_FALSE(nfa_processor::run(result, "").matched);       // Does not match empty string
}

TEST(NFA_Processor_Test, Complex_Expression) {
    builder builder;

    // Build the expression: ^(a(bc)?d|e[fg]+h)$
    nfa expression = builder.add_concatenation(
        builder.add_start_anchor(),
        builder.add_concatenation(
            builder.add_alternation(
                builder.add_concatenation(
                    builder.add_literal('a'),
                    builder.add_concatenation(
                        builder.add_optionality(
                            builder.add_concatenation(
                                builder.add_literal('b'),
                                builder.add_literal('c')
                            )
                        ),
                        builder.add_literal('d')
                    )
                ),
                builder.add_concatenation(
                    builder.add_literal('e'),
                    builder.add_concatenation(
                        builder.add_one_or_more(
                            builder.add_character_class_range("fg")
                        ),
                        builder.add_literal('h')
                    )
                )
            ),
            builder.add_end_anchor()
        )
    );

    build_result result = builder.complete(expression);
    visualize_nfa_dot(result.automaton, std::cout);

    // Valid inputs
    EXPECT_TRUE(nfa_processor::run(result, "ad").matched);       // Matches "ad"
    EXPECT_TRUE(nfa_processor::run(result, "abcd").matched);     // Matches "abcd"
    EXPECT_TRUE(nfa_processor::run(result, "efgh").matched);     // Matches "efgh"
    EXPECT_TRUE(nfa_processor::run(result, "effgh").matched);    // Matches "effgh"
    EXPECT_TRUE(nfa_processor::run(result, "eggh").matched);     // Matches "eggh"

    // Invalid inputs
    EXPECT_FALSE(nfa_processor::run(result, "").matched);       // Incomplete match
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Incomplete match
    EXPECT_FALSE(nfa_processor::run(result, "abc").matched);     // Incomplete match
    EXPECT_FALSE(nfa_processor::run(result, "efg").matched);     // Incomplete match
    EXPECT_FALSE(nfa_processor::run(result, "adf").matched);     // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "abcdh").matched);   // Extra character
}

TEST(NFA_Processor_Test, Concatenation) {
    builder builder;
    const build_result result = builder.complete(builder.add_concatenation("ab"));

    visualize_nfa_dot(result.automaton, std::cout);

    auto [matched, captured_groups, named_captures] = nfa_processor::run(result, "ab");
    EXPECT_TRUE(matched);
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);
    EXPECT_FALSE(nfa_processor::run(result, "abc").matched);
}

TEST(NFA_Processor_Test, ZeroOrMore) {
    builder builder;
    const build_result result = builder.complete(builder.add_zero_or_more(builder.add_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Processor_Test, OneOrMore) {
    builder builder;
    const build_result result = builder.complete(builder.add_one_or_more('a'));

    EXPECT_FALSE(nfa_processor::run(result, "").matched);        // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Processor_Test, Optionality) {
    builder builder;
    const build_result result = builder.complete(builder.add_optionality(builder.add_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_FALSE(nfa_processor::run(result, "aa").matched);      // Too many occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
}

TEST(NFA_Processor_Test, CharacterClass_Range) {
    builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Lower boundary
    EXPECT_TRUE(nfa_processor::run(result, "m").matched);        // Middle of the range
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Upper boundary
    EXPECT_FALSE(nfa_processor::run(result, "A").matched);       // Out of range
    EXPECT_FALSE(nfa_processor::run(result, "1").matched);       // Non-alphabetic character
}

TEST(NFA_Processor_Test, CharacterClass_Explicit) {
    builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("abc"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Matches 'a'
    EXPECT_TRUE(nfa_processor::run(result, "b").matched);        // Matches 'b'
    EXPECT_TRUE(nfa_processor::run(result, "c").matched);        // Matches 'c'
    EXPECT_FALSE(nfa_processor::run(result, "d").matched);       // Not in the set
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Not in the set
}

TEST(NFA_Processor_Test, CharacterClass_NegatedRange) {
    builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("^a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "A").matched);        // Uppercase letter
    EXPECT_TRUE(nfa_processor::run(result, "1").matched);        // Digit
    EXPECT_TRUE(nfa_processor::run(result, "!").matched);        // Special character
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Lowercase letter in range
    EXPECT_FALSE(nfa_processor::run(result, "m").matched);       // Middle of the range
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Upper boundary of range
}

TEST(NFA_Processor_Test, CharacterClass_NegatedExplicit) {
    builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("^abc"));

    EXPECT_TRUE(nfa_processor::run(result, "d").matched);        // Outside explicit set
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Another character outside the set
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "c").matched);       // Matches negated set
}

TEST(NFA_Processor_Test, Execute_With_Groups_Single_Test) {
    builder builder;

    const build_result result = builder.complete(
        builder.add_zero_or_more(
            builder.add_group(
                builder.add_concatenation(builder.add_literal('a'), builder.add_literal('b'))
            )
        )
    );

    visualize_nfa_dot(result.automaton, std::cout);

    auto [matched, captured_groups, ignore] = nfa_processor::run(result, "ababab");

    EXPECT_TRUE(matched);
    EXPECT_EQ(captured_groups.size(), static_cast<size_t>(2));       // Whole match + last group match
    EXPECT_EQ(captured_groups[0], "ababab");    // Whole match
    EXPECT_EQ(captured_groups[1], "ab");        // Last group match
}

TEST(NFA_Processor_Test, NonCapturingGroup) {
    builder builder;
    const build_result result = builder.complete(
        builder.add_one_or_more(
            builder.add_non_capturing_group(
                builder.add_concatenation(builder.add_literal('a'), builder.add_literal('b'))
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
    builder builder;

    const build_result result = builder.complete(
        builder.add_zero_or_more(
            builder.add_group(
                builder.add_concatenation(builder.add_literal('a'), builder.add_literal('b')),
                "word"
            )
        )
    );

    auto [matched, captured_groups, named_captures] = nfa_processor::run(result, "ababab");

    EXPECT_TRUE(matched);
    EXPECT_EQ(named_captures.at("word"), "ab"); // Last match for the named group
}

TEST(NFA_Processor_Test, Basic_Anchors) {
    builder builder;

    // ^a$
    nfa start_anchor = builder.add_start_anchor();
    nfa literal_a = builder.add_literal('a');
    nfa end_anchor = builder.add_end_anchor();

    nfa combined = builder.add_concatenation(start_anchor, literal_a);
    combined = builder.add_concatenation(combined, end_anchor);

    auto result = nfa_processor::run(builder.complete(combined), "a");
    EXPECT_TRUE(result.matched); // Should match "a"

    result = nfa_processor::run(builder.complete(combined), "ba");
    EXPECT_FALSE(result.matched); // Should not match "ba"

    result = nfa_processor::run(builder.complete(combined), "ab");
    EXPECT_FALSE(result.matched); // Should not match "ab"
}

TEST(NFA_Processor_Test, AnchoredCharacterClass) {
    builder builder;

    // Step 1: Add Start Anchor (^)
    nfa start_anchor = builder.add_start_anchor();

    // Step 2: Add [a-z]+ (One or More Lowercase Letters)
    nfa char_class = builder.add_character_class_range("a-z"); // Compact string-based method
    nfa one_or_more = builder.add_one_or_more(char_class);

    // Step 3: Add End Anchor ($)
    nfa end_anchor = builder.add_end_anchor();

    // Combine: ^ -> [a-z]+ -> $
    nfa combined = builder.add_concatenation(start_anchor, one_or_more);
    combined = builder.add_concatenation(combined, end_anchor);

    // Run the NFA on test cases
    build_result result = builder.complete(combined);

    // Valid inputs
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);       // Single letter
    EXPECT_TRUE(nfa_processor::run(result, "abc").matched);     // Multiple letters
    EXPECT_TRUE(nfa_processor::run(result, "zxy").matched);     // Random sequence

    // Invalid inputs
    EXPECT_FALSE(nfa_processor::run(result, "").matched);        // Empty input
    EXPECT_FALSE(nfa_processor::run(result, "1abc").matched);    // Starts with a digit
    EXPECT_FALSE(nfa_processor::run(result, "abc1").matched);    // Ends with a digit
    EXPECT_FALSE(nfa_processor::run(result, "A").matched);       // Uppercase letter
    EXPECT_FALSE(nfa_processor::run(result, "aB").matched);      // Mixed case
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
