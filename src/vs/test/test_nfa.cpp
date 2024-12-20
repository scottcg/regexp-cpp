#include "test_nfa.h"
#include <gtest/gtest.h>


TEST(NFA_Builder_Test, Build_Concatenation) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_concatenation("ab"));

    visualize_nfa_dot(result.automaton, std::cout);

    auto [matched, captured_groups, named_captures] = nfa_processor::run(result, "ab");
    EXPECT_TRUE(matched);
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);
    EXPECT_FALSE(nfa_processor::run(result, "abc").matched);
}

TEST(NFA_Builder_Test, Build_ZeroOrMore) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_zero_or_more(builder.add_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_OneOrMore) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_one_or_more('a'));

    EXPECT_FALSE(nfa_processor::run(result, "").matched);        // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_TRUE(nfa_processor::run(result, "aaaa").matched);     // Multiple occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
    EXPECT_FALSE(nfa_processor::run(result, "aaab").matched);    // Ends with invalid character
}

TEST(NFA_Builder_Test, Build_Optionality) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_optionality(builder.add_literal('a')));

    EXPECT_TRUE(nfa_processor::run(result, "").matched);         // Zero occurrences
    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // One occurrence
    EXPECT_FALSE(nfa_processor::run(result, "aa").matched);      // Too many occurrences
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Invalid character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Range) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Lower boundary
    EXPECT_TRUE(nfa_processor::run(result, "m").matched);        // Middle of the range
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Upper boundary
    EXPECT_FALSE(nfa_processor::run(result, "A").matched);       // Out of range
    EXPECT_FALSE(nfa_processor::run(result, "1").matched);       // Non-alphabetic character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("abc"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Matches 'a'
    EXPECT_TRUE(nfa_processor::run(result, "b").matched);        // Matches 'b'
    EXPECT_TRUE(nfa_processor::run(result, "c").matched);        // Matches 'c'
    EXPECT_FALSE(nfa_processor::run(result, "d").matched);       // Not in the set
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Not in the set
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedRange) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("^a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "A").matched);        // Uppercase letter
    EXPECT_TRUE(nfa_processor::run(result, "1").matched);        // Digit
    EXPECT_TRUE(nfa_processor::run(result, "!").matched);        // Special character
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Lowercase letter in range
    EXPECT_FALSE(nfa_processor::run(result, "m").matched);       // Middle of the range
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Upper boundary of range
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class_range("^abc"));

    EXPECT_TRUE(nfa_processor::run(result, "d").matched);        // Outside explicit set
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Another character outside the set
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "b").matched);       // Matches negated set
    EXPECT_FALSE(nfa_processor::run(result, "c").matched);       // Matches negated set
}

TEST(NFA_Processor_Test, Execute_With_Groups_Single_Test) {
    nfa_builder builder;

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
    EXPECT_EQ(captured_groups.size(), 2);       // Whole match + last group match
    EXPECT_EQ(captured_groups[0], "ababab");    // Whole match
    EXPECT_EQ(captured_groups[1], "ab");        // Last group match
}

TEST(NFA_Builder_Test, Build_NonCapturingGroup) {
    nfa_builder builder;
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
    nfa_builder builder;

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

TEST(NFA_Builder, Build_Anchors) {
    nfa_builder builder;

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

TEST(NFA_Processor, Test_AnchoredCharacterClass) {
    nfa_builder builder;

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
