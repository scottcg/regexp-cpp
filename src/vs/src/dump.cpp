#include <iostream>
#include <cassert>
#include "regex.h"
#include "input_string.h"
#include "traits.h"
#include "engine.h"
#include "syntax_grep.h"
#include "syntax_perl.h"
#include "instruction.h"

using namespace re;
using namespace std;
using my_traits = re_char_traits<char>;
using target_syntax = syntax_perl<my_traits>;

void dump_code(const my_traits::char_type *expr) {
    std::cout << "Dumping: " << expr << std::endl;
    re_engine<target_syntax> r;
    const int compileResult = r.exec_compile(expr);
    cout << "Compile result: " << compileResult << " expr:" << expr << endl;
    r.dump_code(cout);
}

int main(const int argc, char *argv[]) {
    std::cout << "dump stuff" << std::endl;

    if (argc > 1) {
        dump_code(argv[1]);
    } else {
        dump_code("[a-c]+");
        dump_code("\\w{3,4}");
        dump_code("[a-zA-Z0-9_]");
        dump_code("[^a-zA-Z0-9_]+");
        dump_code("[^ab12DE]+"); // "A1B2C3D4"
        dump_code(R"((\w+)\s+(\w+) \((.+)\))"); // "John Doe (johndoe@example.com)"
        dump_code(R"(https?:\/\/(?:www\.)?([^\/]+))"); //  "http://www.example.com/page.html"
        dump_code(R"((?<=\d)\w+(?=\d))"); // "abc123def456"
    }

    return 0;
}