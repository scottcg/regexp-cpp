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
    const build_result result = builder.complete(builder.add_character_class("a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Lower boundary
    EXPECT_TRUE(nfa_processor::run(result, "m").matched);        // Middle of the range
    EXPECT_TRUE(nfa_processor::run(result, "z").matched);        // Upper boundary
    EXPECT_FALSE(nfa_processor::run(result, "A").matched);       // Out of range
    EXPECT_FALSE(nfa_processor::run(result, "1").matched);       // Non-alphabetic character
}

TEST(NFA_Builder_Test, Build_CharacterClass_Explicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class("abc"));

    EXPECT_TRUE(nfa_processor::run(result, "a").matched);        // Matches 'a'
    EXPECT_TRUE(nfa_processor::run(result, "b").matched);        // Matches 'b'
    EXPECT_TRUE(nfa_processor::run(result, "c").matched);        // Matches 'c'
    EXPECT_FALSE(nfa_processor::run(result, "d").matched);       // Not in the set
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Not in the set
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedRange) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class("^a-z"));

    EXPECT_TRUE(nfa_processor::run(result, "A").matched);        // Uppercase letter
    EXPECT_TRUE(nfa_processor::run(result, "1").matched);        // Digit
    EXPECT_TRUE(nfa_processor::run(result, "!").matched);        // Special character
    EXPECT_FALSE(nfa_processor::run(result, "a").matched);       // Lowercase letter in range
    EXPECT_FALSE(nfa_processor::run(result, "m").matched);       // Middle of the range
    EXPECT_FALSE(nfa_processor::run(result, "z").matched);       // Upper boundary of range
}

TEST(NFA_Builder_Test, Build_CharacterClass_NegatedExplicit) {
    nfa_builder builder;
    const build_result result = builder.complete(builder.add_character_class("^abc"));

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
