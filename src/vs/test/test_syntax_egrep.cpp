#include <gtest/gtest.h>
#include "syntax_egrep.h"
#include "compile.h"
#include "traits.h"
#include <iostream>

using namespace std;
using ct = re_char_traits<char>;
using syntax_egrep_t = re::syntax_egrep<ct>;
using compile_state_t = re::compile_state<ct>;

namespace re {
    TEST(syntax_egrep, BasicOperators) {
        const syntax_egrep_t syntax;
        std::string input_str = "a\\b^$.[][^a-z]*+?|()";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);

        // Test non-metacharacter 'a'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);

        // Test escape sequence '\b'
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_escaped_op(cs), TOK_CTRL_CHAR);

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
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR); // a
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), TOK_CHAR);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), ']');

        // Test zero or more occurrences 'r*'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '*');

        // Test one or more occurrences 'r+'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '+');

        // Test zero or one occurrences 'r?'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '?');

        // Test alternation 'r1|r2'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '|');

        // Test grouping '(r)'
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '(');
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), ')');
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}