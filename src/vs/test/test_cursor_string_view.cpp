#include "cursor_string_view.h"
#include <gtest/gtest.h>
#include "traits.h"
#include "cursor_string_view.h"

using ct = re_char_traits<char>;
using test_view = re::cursor_string_view<ct>;

namespace re {

    TEST(CursorStringViewTest, NextCharacter) {
        test_view view("Hello");

        EXPECT_EQ(view.next(), 'H');
        EXPECT_EQ(view.next(), 'e');
        EXPECT_EQ(view.position(), (size_t)2);
    }

    TEST(CursorStringViewTest, PreviousCharacter) {
        test_view view("Hello");

        view.advance(4); // Move cursor forward
        EXPECT_EQ(view.prev(), 'l');
        EXPECT_EQ(view.prev(), 'l');
        EXPECT_EQ(view.position(), (size_t)2);
    }

    TEST(CursorStringViewTest, AtBeginAndAtEnd) {
        test_view view("Hello");

        EXPECT_TRUE(view.at_begin());
        EXPECT_FALSE(view.at_end());

        view.advance(5);
        EXPECT_TRUE(view.at_end());
        EXPECT_FALSE(view.at_begin());
    }

    TEST(CursorStringViewTest, CurrentCharacter) {
        test_view view("Hello");

        EXPECT_EQ(view.current(), 'H');
        view.next();
        EXPECT_EQ(view.current(), 'e');
    }

    TEST(CursorStringViewTest, RemainingView) {
        test_view view("Hello, World!");

        view.advance(7);
        EXPECT_EQ(view.remaining(), "World!");
    }

    TEST(CursorStringViewTest, ResetCursor) {
        test_view view("Test");

        view.advance(3);
        EXPECT_EQ(view.position(), (size_t)3);

        view.reset();
        EXPECT_TRUE(view.at_begin());
        EXPECT_EQ(view.position(), (size_t)0);
    }

    TEST(CursorStringViewTest, EdgeCases) {
        test_view view("");

        EXPECT_TRUE(view.at_begin());
        EXPECT_TRUE(view.at_end());
        EXPECT_EQ(view.next(), '\0');
        EXPECT_EQ(view.prev(), '\0');
        EXPECT_EQ(view.current(), '\0');
    }

    TEST(CursorStringViewTest, AdvanceBeyondBounds) {
        test_view view("Bounds");

        view.advance(10); // Move beyond the string
        EXPECT_TRUE(view.at_end());
        EXPECT_EQ(view.position(), (size_t)6);
        EXPECT_EQ(view.next(), '\0');
    }

} // namespace re