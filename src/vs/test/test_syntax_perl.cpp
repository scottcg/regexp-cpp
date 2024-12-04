#include <gtest/gtest.h>
#include <cassert>
#include "input_string.h"
#include "traits.h"
#include "engine.h"
#include "syntax_grep.h"
#include "syntax_perl.h"


using ct = re_char_traits<char>;
using syntax_perl_t = re::syntax_perl<ct>;
using compile_state_t = re::compile_state<ct>;

namespace re {
    TEST(syntax_perl, Precedence) {
        syntax_perl_t syntax;
        ASSERT_EQ(syntax.precedence('{'), syntax.precedence('}'));
    }

    TEST(syntax_perl, ContextIndependentOps) {
        syntax_perl_t syntax;
        ASSERT_FALSE(syntax.context_independent_ops());
    }

    TEST(syntax_perl, IncompleteEOI) {
        syntax_perl_t syntax;
        std::string input_str = "test|";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        ASSERT_TRUE(syntax.incomplete_eoi(cs));
    }

    TEST(syntax_perl, TranslatePlainOp) {
        syntax_perl_t syntax;
        std::string input_str = "{";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_plain_op(cs), '{');
    }

    TEST(syntax_perl, TranslateEscapedOp) {
        syntax_perl_t syntax;
        std::string input_str = "\\d";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_escaped_op(cs), 'd');
    }

    TEST(syntax_perl, TranslateCharClassEscapedOp) {
        syntax_perl_t syntax;
        std::string input_str = "\\d";
        input_string<ct> input(input_str.c_str());
        compiled_code_vector<ct> output;
        compile_state_t cs(syntax, output, input);
        cs.input.get(cs.ch);
        cs.input.get(cs.ch);
        ASSERT_EQ(syntax.translate_char_class_escaped_op(cs), 0);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}