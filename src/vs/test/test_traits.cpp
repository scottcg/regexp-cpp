#include <gtest/gtest.h>
#include "traits.h"
#include "compile.h"

using cht = re_char_traits<char>;
using wcht = re_char_traits<wchar_t>;

namespace re {
    TEST(TraitsTest, ToDecimal) {
        auto hex = cht::cstr_to_decimal_int("123");
        EXPECT_EQ(123, hex);
    }

    TEST(TraitsTest, WToDecimal) {
        auto hex = wcht::cstr_to_decimal_int(L"123");
        EXPECT_EQ(123, hex);
    }

    TEST(TraitsTest, ToHexidecimal) {
        auto hex = cht::hexadecimal_to_decimal('f');
        EXPECT_EQ(15, hex);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}