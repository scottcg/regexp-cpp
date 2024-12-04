#include <gtest/gtest.h>
#include "traits.h"
#include "input_string.h"

using ct = re_char_traits<char>;
using test_input_string = re::input_string<ct>;

namespace re {
    auto test_string = "Hello World";

    TEST(input_string, Constructor) {
        test_input_string input(re::test_string);
        ASSERT_EQ(input.length(), 11);
    }

    TEST(input_string, OperatorIndex) {
        test_input_string input(test_string);
        ASSERT_EQ(input[0], 'H');
        ASSERT_EQ(input[6], 'W');
    }

    TEST(input_string, Get) {
        test_input_string input(test_string);
        test_input_string::int_type ch;
        input.get(ch);
        ASSERT_EQ(ch, 'H');
        input.get(ch);
        ASSERT_EQ(ch, 'e');
    }

    TEST(input_string, Peek) {
        test_input_string input(test_string);
        ASSERT_EQ(input.peek(), 'H');
        test_input_string::int_type ch;
        input.get(ch);
        ASSERT_EQ(input.peek(), 'e');
    }

    TEST(input_string, Unget) {
        test_input_string input(test_string);
        test_input_string::int_type ch;
        input.get(ch);
        input.get(ch);
        input.unget();
        ASSERT_EQ(input.peek(), 'e');
    }

    TEST(input_string, Advance) {
        test_input_string input(test_string);
        input.advance(6);
        ASSERT_EQ(input.peek(), 'W');
    }

    TEST(input_string, GetNumber) {
        auto num_string = "12345";
        test_input_string input(num_string);
        test_input_string::int_type ch;
        input.get(ch);
        ASSERT_EQ(input.get_number(ch), 1);
        ASSERT_EQ(ch, '2');
    }

    TEST(input_string, PeekNumber) {
        auto num_string = "12345";
        test_input_string input(num_string);
        test_input_string::int_type n;
        ASSERT_EQ(input.peek_number(n), 5);
        ASSERT_EQ(n, 12345);
    }

    TEST(input_string, AtBeginAndAtEnd) {
        test_input_string input(test_string);
        ASSERT_TRUE(input.at_begin());
        test_input_string::int_type ch;
        while (input.get(ch) == 0);
        ASSERT_TRUE(input.at_end());
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}