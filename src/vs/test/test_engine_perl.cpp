#include <gtest/gtest.h>

#include <cassert>
#include "input_string.h"
#include "traits.h"
#include "engine.h"
#include "syntax_grep.h"
#include "syntax_perl.h"


#include "syntax_perl.h"
#include "traits.h"
#include "engine.h"

using ct = re_char_traits<char>;
using syntax_perl_t = re::syntax_perl<ct>;
using re_engine_t = re::re_engine<syntax_perl_t>;

namespace re {
    TEST(re_engine, CompileAndMatch) {
        re_engine_t engine;
        const char* pattern = "a*b";
        int err_pos = 0;

        // Compile the pattern
        ASSERT_EQ(engine.exec_compile(pattern, strlen(pattern), &err_pos), 0);

        // Test matching
        const char* text = "aaab";
        ctext<ct> ct(text, strlen(text));
        re_match_vector matches;
        ASSERT_EQ(engine.exec_match(ct), 4);
    }

    TEST(re_engine, NoMatch) {
        re_engine_t engine;
        const char* pattern = "a*b";
        int err_pos = 0;

        // Compile the pattern
        ASSERT_EQ(engine.exec_compile(pattern, strlen(pattern), &err_pos), 0);

        // Test no match
        const char* text = "aaac";
        ctext<ct> ct(text, strlen(text));
        re_match_vector matches;
        ASSERT_EQ(engine.exec_match(ct, false, matches), -1);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}