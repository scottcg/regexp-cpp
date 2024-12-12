#include "gtest/gtest.h"
#include <vector>
#include <string>
#include "tokenizer.h"  // Include the tokenizer implementation

using namespace std;


std::pair<size_t, std::string> find_first_difference(const std::vector<token> &actual,
                                                     const std::vector<token> &expected) {
    const size_t min_size = std::min(actual.size(), expected.size());
    for (size_t i = 0; i < min_size; ++i) {
        if (actual[i].type != expected[i].type || actual[i].value != expected[i].value) {
            return {
                i,
                "Difference at index " + std::to_string(i) + ": actual(" + token_type_to_string(actual[i].type) + ", " +
                actual[i].value + ") vs expected(" + token_type_to_string(expected[i].type) + ", " + expected[i].value +
                ")"
            };
        }
    }
    return {std::string::npos, "No differences found"};
}

// Helper function to compare token outputs
bool compare_tokens(const std::vector<token> &actual, const std::vector<token> &expected) {
    if (actual.size() != expected.size()) {
        cout << "ERROR SIZE actual:" << actual.size() << " expected:" << expected.size() << endl;
        cout << "difference: " << find_first_difference(actual, expected).second << endl;
        return false;
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].type != expected[i].type || actual[i].value != expected[i].value) {
            return false;
        }
    }
    return true;
}

// Test Case 1: Basic tokens
TEST(tokenizer_tests, basic_literals_and_escapes) {
    const std::string regex = R"(\d+)";
    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();

    const std::vector<token> expected = {
        {token_type::escape, "\\d"},
        {token_type::quantifier, "+"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Test Case 2: Character classes
TEST(tokenizer_tests, character_classes) {
    const std::string regex = R"([^A-Z])";
    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();

    const std::vector<token> expected = {
        {token_type::negated_class_open, "[^"},
        {token_type::literal, "A"},
        {token_type::literal, "-"},
        {token_type::literal, "Z"},
        {token_type::character_class_close, "]"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Test Case 3: Groups and alternation
TEST(tokenizer_tests, groups_and_alternation) {
    const std::string regex = R"((\d+)|\w*)";
    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();

    const std::vector<token> expected = {
        {token_type::group_open, "("},
        {token_type::escape, "\\d"},
        {token_type::quantifier, "+"},
        {token_type::group_close, ")"},
        {token_type::alternation, "|"},
        {token_type::escape, "\\w"},
        {token_type::quantifier, "*"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Test Case 4: Quantifiers with trailing +
TEST(tokenizer_tests, complex_quantifiers) {
    const std::string regex = R"({2,}+)";
    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();

    const std::vector<token> expected = {
        {token_type::quantifier, "{2,}+"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Test Case 5: Full complex regex
TEST(tokenizer_tests, full_complex_regex) {
    const std::string regex = R"((\d+)|\w*\[a-z\]\?|.*(\w\w+).*|[^A-Z]{2,}+)";

    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();

    const std::vector<token> expected = {
        {token_type::group_open, "("},
        {token_type::escape, "\\d"},
        {token_type::quantifier, "+"},
        {token_type::group_close, ")"},
        {token_type::alternation, "|"},
        {token_type::escape, "\\w"},
        {token_type::quantifier, "*"},
        {token_type::character_class_open, "["},
        {token_type::literal, "a"},
        {token_type::literal, "-"},
        {token_type::literal, "z"},
        {token_type::character_class_close, "]"},
        {token_type::escape, "\\?"},
        {token_type::alternation, "|"},
        {token_type::meta_character, "."},
        {token_type::quantifier, "*"},
        {token_type::group_open, "("},
        {token_type::escape, "\\w"},
        {token_type::escape, "\\w"},
        {token_type::quantifier, "+"},
        {token_type::group_close, ")"},
        {token_type::meta_character, "."},
        {token_type::quantifier, "*"},
        {token_type::alternation, "|"},
        {token_type::negated_class_open, "[^"},
        {token_type::literal, "A"},
        {token_type::literal, "-"},
        {token_type::literal, "Z"},
        {token_type::character_class_close, "]"},
        {token_type::quantifier, "{2,}+"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Test Case 6: URL
TEST(tokenizer_tests, complex_url) {
    const std::string regex = R"(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{2,256}\.[a-z]{2,6}\b([-a-zA-Z0-9@:%_\+.~#()?&//=]*))";

    tokenizer t(regex);
    const std::vector<token> tokens = t.tokenize();
    const std::vector<token> expected = {
        {token_type::literal, "h"},
        {token_type::literal, "t"},
        {token_type::literal, "t"},
        {token_type::literal, "p"},
        {token_type::literal, "s"},
        {token_type::quantifier, "?"},
        {token_type::literal, ":"},
        {token_type::escape, "\\/"},
        {token_type::escape, "\\/"},
        {token_type::group_open, "("},
        {token_type::literal, "w"},
        {token_type::literal, "w"},
        {token_type::literal, "w"},
        {token_type::escape, "\\."},
        {token_type::group_close, ")"},
        {token_type::quantifier, "?"},
        {token_type::character_class_open, "["},
        {token_type::literal, "-"},
        {token_type::literal, "a"},
        {token_type::literal, "-"},
        {token_type::literal, "z"},
        {token_type::literal, "A"},
        {token_type::literal, "-"},
        {token_type::literal, "Z"},
        {token_type::literal, "0"},
        {token_type::literal, "-"},
        {token_type::literal, "9"},
        {token_type::literal, "@"},
        {token_type::literal, ":"},
        {token_type::literal, "%"},
        {token_type::meta_character, "."},
        {token_type::literal, "_"},
        {token_type::escape, "\\+"},
        {token_type::literal, "~"},
        {token_type::literal, "#"},
        {token_type::literal, "="},
        {token_type::character_class_close, "]"},
        {token_type::quantifier, "{2,256}"},
        {token_type::escape, "\\."},
        {token_type::character_class_open, "["},
        {token_type::literal, "a"},
        {token_type::literal, "-"},
        {token_type::literal, "z"},
        {token_type::character_class_close, "]"},
        {token_type::quantifier, "{2,6}"},
        {token_type::escape, "\\b"},
        {token_type::group_open, "("},
        {token_type::character_class_open, "["},
        {token_type::literal, "-"},
        {token_type::literal, "a"},
        {token_type::literal, "-"},
        {token_type::literal, "z"},
        {token_type::literal, "A"},
        {token_type::literal, "-"},
        {token_type::literal, "Z"},
        {token_type::literal, "0"},
        {token_type::literal, "-"},
        {token_type::literal, "9"},
        {token_type::literal, "@"},
        {token_type::literal, ":"},
        {token_type::literal, "%"},
        {token_type::literal, "_"},
        {token_type::escape, "\\+"},
        {token_type::meta_character, "."},
        {token_type::literal, "~"},
        {token_type::literal, "#"},
        {token_type::group_open, "("},
        {token_type::group_close, ")"},
        {token_type::quantifier, "?"},
        {token_type::literal, "&"},
        {token_type::literal, "/"},
        {token_type::literal, "/"},
        {token_type::literal, "="},
        {token_type::character_class_close, "]"},
        {token_type::quantifier, "*"},
        {token_type::group_close, ")"},
        {token_type::end_of_input, ""}
    };

    EXPECT_TRUE(compare_tokens(tokens, expected));
}

// Main for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
