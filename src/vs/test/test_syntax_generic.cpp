#include <gtest/gtest.h>
#include "syntax_generic.h"
#include "compile.h"
#include "traits.h"

using ct = re_char_traits<char>;
using syntax_generic_t = re::syntax_generic<ct>;
using compile_state_t = re::compile_state<ct>;

namespace re {
    TEST(syntax_generic, Precedence) {
        const syntax_generic_t syntax;
        ASSERT_EQ(syntax.precedence('^'), 3);
        ASSERT_EQ(syntax.precedence('$'), 3);
        ASSERT_EQ(syntax.precedence('|'), 2);
        ASSERT_EQ(syntax.precedence(')'), 1);
        ASSERT_EQ(syntax.precedence(TOK_END), 0);
    }

    TEST(syntax_generic, ContextIndependentOps) {
        const syntax_generic_t syntax;
        ASSERT_TRUE(syntax.context_independent_ops());
    }

    TEST(syntax_generic, TranslatePlainOp) {
        const syntax_generic_t syntax;
        const std::string input_str = "\\";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        cs.input.get(cs.ch);
        //ASSERT_EQ(syntax.translate_plain_op(cs), TOK_ESCAPE);
    }

    TEST(syntax_generic, TranslateEscapedOp) {
        const syntax_generic_t syntax;
        const std::string input_str = "\\d";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        //ASSERT_EQ(syntax.translate_escaped_op(cs), TOK_CHAR);
    }

    TEST(syntax_generic, CharacterClass) {
        const syntax_generic_t syntax;
        std::string input_str = "[abc]";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '[');
    }

    TEST(syntax_generic, NegatedCharacterClass) {
        const syntax_generic_t syntax;
        std::string input_str = "[^abc]";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '[');
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '^');
    }

    TEST(syntax_generic, CharacterRange) {
        const syntax_generic_t syntax;
        std::string input_str = "[a-c]";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '[');
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), ']');
    }

    TEST(syntax_generic, NegatedCharacterRange) {
        const syntax_generic_t syntax;
        std::string input_str = "[^a-c]";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '[');
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '^');
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), ']');
    }

    TEST(syntax_generic, BasicOperations) {
        const syntax_generic_t syntax;
        std::string input_str = "a\\b^$.[][^a-z]*";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        // Test non-metacharacter 'a'
        cs.input.get(cs.ch);
        //ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);

        // Test escape sequence '\b'
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        //ASSERT_EQ(syntax.translate_plain_op(cs), TOK_ESCAPE);

        // Test beginning of string '^'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '^');

        // Test end of string '$'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '$');

        // Test any character '.'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '.');

        // Test character class '[ab...]'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '[');

        // Test negated character class '[^ab...]'
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '^');

        // Test character range '[a-b]'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), ']');

        // Test zero or more occurrences 'r*'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '*');
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
