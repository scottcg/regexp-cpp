#include <iostream>
#include <cassert>
#include "source.h"
#include "rcimpl.h"
#include "traits.h"
#include "engine.h"

using namespace re;
using namespace std;


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
    re_ctext<re_char_traits<char>> text(testString, -1);
    re_char_traits<char>::int_type lastch;
    const bool result = text.has_substring(0, 5, lastch);
    std::cout << "Has substring result: " << (result ? "true" : "false") << std::endl;
    assert(result == true);
}

void testNoSubstring() {
    const auto testString = "Hello, World!";
    re_ctext<re_char_traits<char>> text(testString, -1);
    re_char_traits<char>::int_type lastch;
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

void test_precedence_vec() {
    // Create an instance of re_precedence_vec with default initialization
    re_precedence_vec<NUM_LEVELS> vec;

    // Check the initial values
    std::cout << "Initial values: ";
    for (int i = 0; i < NUM_LEVELS; ++i) {
        std::cout << vec[i] << " ";
        assert(vec[i] == 0);
    }
    std::cout << std::endl;

    // Modify the values
    for (int i = 0; i < NUM_LEVELS; ++i) {
        vec[i] = i * 10;
    }

    // Check the modified values
    std::cout << "Modified values: ";
    for (int i = 0; i < NUM_LEVELS; ++i) {
        std::cout << vec[i] << " ";
        assert(vec[i] == i * 10);
    }
    std::cout << std::endl;
}

void test_precedence_stack() {
    // Create an instance of re_precedence_stack
    re_precedence_stack stack;

    // Check the initial current precedence value
    std::cout << "Initial current precedence: " << stack.current() << std::endl;
    assert(stack.current() == 0);

    // Check the initial start value
    std::cout << "Initial start value: " << stack.start() << std::endl;
    assert(stack.start() == 0);

    // Modify the current precedence value
    stack.current(2);
    std::cout << "Modified current precedence: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Modify the start value
    stack.start(10);
    std::cout << "Modified start value: " << stack.start() << std::endl;
    assert(stack.start() == 10);

    // Push a new element onto the stack
    stack.push(re_precedence_element());
    std::cout << "Pushed new element onto the stack." << std::endl;

    // Check the current precedence value after push
    std::cout << "Current precedence after push: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Check the start value after push
    std::cout << "Start value after push: " << stack.start() << std::endl;
    assert(stack.start() == 0);

    // Pop the element from the stack
    stack.pop();
    std::cout << "Popped element from the stack." << std::endl;

    // Check the current precedence value after pop
    std::cout << "Current precedence after pop: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Check the start value after pop
    std::cout << "Start value after pop: " << stack.start() << std::endl;
    assert(stack.start() == 10);
}

void test_traits() {
    std::wcout << re_char_traits<char>::length("Hello") << std::endl;
    std::wcout << re_char_traits<char>::isdigit('0') << std::endl;
    std::wcout << re_char_traits<char>::isdigit('A') << std::endl;
}

void test_basic_expression() {
    cout << endl << "Testing compile" << endl;
    using my_traits = re_char_traits<char>;

    re_engine< generic_syntax<my_traits>> engine;
    cout << "Created re_engine" << endl;
    auto const compileResult = engine.exec_compile("dog*");
    cout << "Compile result: " << compileResult << endl;
    engine.dump_code();
}

void test_perl() {
    cout << endl << "Testing compile" << endl;
    using my_traits = re_char_traits<char>;

#if 0
    generic_syntax<my_traits> syntax;
    cout << "Created generic_syntax" << endl;

    re_code_vec<my_traits> code;
    cout << "Creating re_compile_state" << endl;

    re_input_string<my_traits> input("[Hh]ello, [Ww]orld");
    cout << "Input string length: " << input.length() << endl;

    re_compile_state cs(syntax, code, input);
    cout << "Created re_compile_state" << endl;

    const auto result = syntax.compile(cs);
    cout << "Compile result: " << result << endl;
#endif

    re_engine<perl_syntax<my_traits>> engine;
    cout << "Created re_engine" << endl;
    auto const compileResult = engine.exec_compile("[Hh]+ello, [Ww]?orld");
    cout << "Compile result: " << compileResult << endl;
    engine.dump_code();

    re_ctext<my_traits> text("Hello, World!");
    //auto const searchResult = engine.exec_search(text);
    //cout << "Search result: " << searchResult << endl;

    //auto const matchResult = engine.exec_match(text);
    //cout << "Match result: " << matchResult << endl;
}

#if 0
void test_compiled_code_vector() {
    // Create an instance of compiled_code_vector
    compiled_code_vector<traitsT> vec;

    // Check the initial offset
    std::cout << "Initial offset: " << vec.offset() << std::endl;
    assert(vec.offset() == 0);

    // Store a single code_type value
    vec.store(static_cast<traitsT::char_type>('A'));
    std::cout << "Offset after storing one value: " << vec.offset() << std::endl;
    assert(vec.offset() == 1);
    assert(vec[0] == 'A');

    // Store two code_type values
    vec.store(static_cast<traitsT::char_type>('B'), static_cast<traitsT::char_type>('C'));
    std::cout << "Offset after storing two values: " << vec.offset() << std::endl;
    assert(vec.offset() == 3);
    assert(vec[1] == 'B');
    assert(vec[2] == 'C');

    // Modify a value
    vec[1] = static_cast<traitsT::char_type>('D');
    std::cout << "Modified value at index 1: " << vec[1] << std::endl;
    assert(vec[1] == 'D');

    // Test put_address
    vec.put_address(1, 10);
    std::cout << "Value at index 1 after put_address: " << static_cast<int>(vec[1]) << std::endl;
    std::cout << "Value at index 2 after put_address: " << static_cast<int>(vec[2]) << std::endl;
    assert(static_cast<int>(vec[1]) == 8);
    assert(static_cast<int>(vec[2]) == 0);

    // Test store_jump
    vec.store_jump(0, 1, 5);
    std::cout << "Values after store_jump: ";
    for (int i = 0; i < vec.offset(); ++i) {
        std::cout << static_cast<int>(vec[i]) << " ";
    }
    std::cout << std::endl;

    // Check the final offset
    std::cout << "Final offset: " << vec.offset() << std::endl;
    assert(vec.offset() == 6);
}
#endif

void test_basic_regular_expression() {

    using my_traits = re_char_traits<char>;

    re::grep_syntax<my_traits> grep_syntax;
    re::awk_syntax<my_traits> awk_syntax;
    re::egrep_syntax<my_traits> egrep_syntax;
    re::perl_syntax<my_traits> perl_syntax;

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
    re_input_string<re_char_traits<char>> inputString(testString);

    std::cout << "Length of input string: " << inputString.length() << std::endl;
    assert(inputString.length() == 13);

    re_char_traits<char>::int_type ch;
    while (inputString.get(ch) != -1) {
        std::cout << "Next character: " << static_cast<char>(ch) << std::endl;
    }

    const auto str1 = "Hello, ";
    const auto str2 = "World!";
    const re_ctext<re_char_traits<char>> combinedText(str1, -1, str2);

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

    test_precedence_vec();
    test_precedence_stack();

    test_char_traits();
    test_wchar_traits();

    test_basic_expression();
    test_perl();

    //test_basic_regular_expression();

    //re_compile_state<re_char_traits<char>> validState();    // OK
    //re_compile_state<re_char_traits<wchar_t>> validStateW;

    return 0;
}
