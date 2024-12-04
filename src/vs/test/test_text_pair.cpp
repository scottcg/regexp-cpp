#include <gtest/gtest.h>
#include "traits.h"
#include "ctext.h"

using ct = re_char_traits<char>;
using test_ctext = re::re_ctext<ct>;

TEST(re_ctext, Constructor) {
    const char* str1 = "Hello";
    const char* str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext.length(), 10);
}

TEST(re_ctext, OperatorIncrement) {
    const char* str1 = "Hello";
    const char* str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext++, 'H');
    ASSERT_EQ(ctext++, 'e');
    ASSERT_EQ(ctext++, 'l');
    ASSERT_EQ(ctext++, 'l');
    ASSERT_EQ(ctext++, 'o');
    ASSERT_EQ(ctext++, 'W');
}

TEST(re_ctext, OperatorDecrement) {
    const char* str1 = "Hello";
    const char* str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ctext++;
    ctext++;
    ctext++;
    ASSERT_EQ(ctext--, 'l');
    ASSERT_EQ(ctext--, 'e');
    ASSERT_EQ(ctext--, 'H');
}

TEST(re_ctext, AtBeginAndAtEnd) {
    const char* str1 = "Hello";
    const char* str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_TRUE(ctext.at_begin());
    while (!ctext.at_end()) {
        ctext++;
    }
    ASSERT_TRUE(ctext.at_end());
}

TEST(re_ctext, Position) {
    const char* str1 = "Hello";
    const char* str2 = "World";
    test_ctext ctext(str1, -1, str2, -1);
    ASSERT_EQ(ctext.position(), 0);
    ctext++;
    ASSERT_EQ(ctext.position(), 1);
    while (!ctext.at_end()) {
        ctext++;
    }
    ASSERT_EQ(ctext.position(), 10);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}