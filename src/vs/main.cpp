#include <iostream>
#include <cassert>
#include "source.h"
#include "rcimpl.h"
#include "traits.h"
#include "engine.h"

using namespace re;

class char_traits {
public:
    typedef char char_type;
    typedef int int_type;

    static size_t length(const char_type* s) {
        return std::strlen(s);
    }
};

class Test {
public:
    explicit Test(const int value) : value(value) {}
    int getValue() const { return value; }
    void setValue(const int newValue) { value = newValue; }

private:
    int value;
};

void testCreation() {
    const auto test1 = new Test(10);
    rcimpl ptr1(test1);
    std::cout << "Value of ptr1: " << ptr1->getValue() << std::endl;
    assert(ptr1->getValue() == 10);
}

void testCopying() {
    const auto test1 = new Test(10);
    const rcimpl ptr1(test1);
    rcimpl ptr2(ptr1);
    std::cout << "Value of ptr2: " << ptr2->getValue() << std::endl;
    assert(ptr2->getValue() == 10);
}

void testModification() {
    const auto test1 = new Test(10);
    rcimpl ptr1(test1);
    rcimpl ptr2(ptr1);
    ptr1->setValue(20);
    std::cout << "Value of ptr1 after modification: " << ptr1->getValue() << std::endl;
    std::cout << "Value of ptr2 after ptr1 modification: " << ptr2->getValue() << std::endl;
    assert(ptr1->getValue() == 20);
    assert(ptr2->getValue() == 20);
}

void testReferenceCount() {
    const auto test1 = new Test(10);
    const rcimpl ptr1(test1);
    const rcimpl ptr2(ptr1);
    std::cout << "Reference count of ptr1: " << ptr1.reference_count() << std::endl;
    std::cout << "Reference count of ptr2: " << ptr2.reference_count() << std::endl;
    assert(ptr1.reference_count() == 2);
    assert(ptr2.reference_count() == 2);
}

void testHasSubstring() {
    const auto testString = "Hello, World!";
    re_ctext<char_traits> text(testString, -1);
    char_traits::int_type lastch;
    const bool result = text.has_substring(0, 5, lastch);
    std::cout << "Has substring result: " << (result ? "true" : "false") << std::endl;
    assert(result == true);
}

void testNoSubstring() {
    const auto testString = "Hello, World!";
    re_ctext<char_traits> text(testString, -1);
    char_traits::int_type lastch;
    const bool result = text.has_substring(0, 3, lastch); // Check for "XYZ" in "Hello, World!"
    std::cout << "No substring result: " << (result ? "true" : "false") << std::endl;
    assert(result == false);
}

void test_char_traits() {
    const auto s1 = "Hello";
    const auto s2 = "hello";
    const size_t len = re_char_traits<char>::length(s1);
    const int cmp = re_char_traits<char>::strncmp(s1, s2, len);
    const int icmp = re_char_traits<char>::istrncmp(s1, s2, len);

    std::cout << "Length of s1: " << len << std::endl;
    std::cout << "Comparison of s1 and s2: " << cmp << std::endl;
    std::cout << "Case-insensitive comparison of s1 and s2: " << icmp << std::endl;

    // Assert-style checks
    assert(len == 5);
    assert(cmp != 0);
    assert(icmp == 0);
}

void test_wchar_traits() {
    const auto ws1 = L"Hello";
    const auto ws2 = L"hello";
    const size_t wlen = re_char_traits<wchar_t>::length(ws1);
    const int wcmp = re_char_traits<wchar_t>::strncmp(ws1, ws2, wlen);
    const int wicmp = re_char_traits<wchar_t>::istrncmp(ws1, ws2, wlen);

    std::wcout << L"Length of ws1: " << wlen << std::endl;
    std::wcout << L"Comparison of ws1 and ws2: " << wcmp << std::endl;
    std::wcout << L"Case-insensitive comparison of ws1 and ws2: " << wicmp << std::endl;

    // Assert-style checks
    assert(wlen == 5);
    assert(wcmp != 0);
    assert(wicmp == 0);
}

// Example usage in main.cpp
void test_basic_regular_expression() {
    std::wcout << re_char_traits<char>::length("Hello") << std::endl;
    std::wcout << re_char_traits<char>::isdigit('0') << std::endl;
    std::wcout << re_char_traits<char>::isdigit('A') << std::endl;


    using my_traits = re_char_traits<char>;
    //re::grep_syntax<re::generic_syntax< my_traits >> grep_syntax;
    re::grep_syntax<my_traits> grep_syntax;
    re::re_engine<re::grep_syntax<my_traits>> r; // ("pattern");
    r.exec_compile("[a-c]+");
    std::string test_string = "test string";
    re_match_vector matches;

    // Assuming there is a method to match the regex with a string
    //bool result = r.exec_search(test_string);

    //re::basic_regular_expression<re::grep_syntax<re::generic_syntax< re_char_traits<char> >>> regex("pattern");
    /*
    // Create a basic_regular_expression using egrep_syntax
    re::basic_regular_expression<re::grep_syntax<re::generic_syntax< re_char_traits<char> >>> regex("pattern");

    // Test the regex object
    std::string test_string = "test string";
    re_match_vector matches;

    // Assuming there is a method to match the regex with a string
    bool result = regex.match(test_string, matches);

    // Print the result
    std::cout << "Match result: " << (result ? "Matched" : "Not Matched") << std::endl;
    for (const auto& match : matches) {
        std::cout << "Match from " << match.first << " to " << match.second << std::endl;
    }
    */
}

int main() {
    // Existing code
    const auto testString = "Hello, World!";
    re_input_string<char_traits> inputString(testString);

    std::cout << "Length of input string: " << inputString.length() << std::endl;
    assert(inputString.length() == 13);

    char_traits::int_type ch;
    while (inputString.get(ch) != -1) {
        std::cout << "Next character: " << static_cast<char>(ch) << std::endl;
    }

    const auto str1 = "Hello, ";
    const auto str2 = "World!";
    const re_ctext<char_traits> combinedText(str1, -1, str2);

    std::cout << "Combined text: ";
    for (int i = 0; i < combinedText.length(); ++i) {
        std::cout << combinedText[i];
    }
    std::cout << std::endl;

    // New test methods
    testCreation();
    testCopying();
    testModification();
    testReferenceCount();
    testHasSubstring();
    // todo testNoSubstring();

    test_char_traits();
    test_wchar_traits();

    test_basic_regular_expression();

    //re_compile_state<re_char_traits<char>> validState();    // OK
    //re_compile_state<re_char_traits<wchar_t>> validStateW;

    return 0;
}
