#include <gtest/gtest.h>
#include "traits.h"
#include "ctext.h"

using ct = re_char_traits<char>;
using test_ctext = re::ctext<ct>;

TEST(ctext, Constructor) {
    auto str1 = "Hello";
    auto str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext.length(), 10);
}

TEST(ctext, OperatorIncrement) {
    auto str1 = "Hello";
    auto str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext++, 'H');
    ASSERT_EQ(ctext++, 'e');
    ASSERT_EQ(ctext++, 'l');
    ASSERT_EQ(ctext++, 'l');
    ASSERT_EQ(ctext++, 'o');
    ASSERT_EQ(ctext++, 'W');
}

TEST(ctext, OperatorDecrement) {
    auto str1 = "Hello";
    auto str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ctext++;
    ctext++;
    ctext++;
    ASSERT_EQ(ctext--, 'l');
    ASSERT_EQ(ctext--, 'e');
    ASSERT_EQ(ctext--, 'H');
}

TEST(ctext, AtBeginAndAtEnd) {
    auto str1 = "Hello";
    auto str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_TRUE(ctext.at_begin());
    while (!ctext.at_end()) {
        ctext++;
    }
    ASSERT_TRUE(ctext.at_end());
}

TEST(ctext, Position) {
    auto str1 = "Hello";
    auto str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext.position(), 0);
    ctext++;
    ASSERT_EQ(ctext.position(), 1);
    while (!ctext.at_end()) {
        ctext++;
    }
    ASSERT_EQ(ctext.position(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}