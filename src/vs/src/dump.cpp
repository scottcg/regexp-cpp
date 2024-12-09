#include <iostream>
#include <cassert>
#include "regex.h"
#include "input_string.h"
#include "traits.h"
#include "engine.h"
#include "syntax_grep.h"
#include "syntax_perl.h"

using namespace re;
using namespace std;
using my_traits = re_char_traits<char>;
using target_syntax = syntax_perl<my_traits>;

void dump_code(const my_traits::char_type *expr) {
    re_engine<target_syntax> r;
    const int compileResult = r.exec_compile(expr);
    cout << "Compile result: " << compileResult << " expr:" << expr << endl;
    r.dump_code(cout);
}

template<class traitsT>
struct instruction {
    typedef traitsT traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::char_type code_type;

    opcodes op; // Opcode (e.g., OP_CHAR, OP_RANGE_CHAR, etc.)
    int_type arg1; // First argument (e.g., character, range start, jump target, etc.)
    int_type arg2; // Second argument (e.g., range end, repetition max, etc.)

    explicit instruction(const opcodes opcode)
        : instruction(opcode, 0, 0) {
    } // Delegates to 3-argument constructor

    instruction(const opcodes opcode, const int_type argument1)
        : instruction(opcode, argument1, 0) {
    } // Delegates to 3-argument constructor

    instruction(const opcodes opcode, const int_type argument1, const int_type argument2)
        : op(opcode), arg1(argument1), arg2(argument2) {
    }
};


int main() {
    std::cout << "dump stuff" << std::endl;

    //test_syntax_perl_dump();
    dump_code("[a-c]+");
    dump_code("\\w{3,4}");
    dump_code("[a-zA-Z0-9_]");
    dump_code("[^a-zA-Z0-9_]+");
    dump_code("[^ab12DE]+"); // "A1B2C3D4"
    dump_code(R"((\w+)\s+(\w+) \((.+)\))"); // "John Doe (johndoe@example.com)"
    dump_code(R"(https?:\/\/(?:www\.)?([^\/]+))"); //  "http://www.example.com/page.html"
    dump_code(R"((?<=\d)\w+(?=\d))"); // "abc123def456"
}
