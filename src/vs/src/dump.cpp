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

int main() {
  std::cout <<"dump stuff" << std::endl;

  //test_syntax_perl_dump();
  dump_code("[a-c]+");
  dump_code("\\w{3,4}");
  dump_code("[a-zA-Z0-9_]");
  dump_code("[^a-zA-Z0-9_]+");
  dump_code("[^ab12DE]+"); // "A1B2C3D4"
  dump_code("(\\w+)\\s+(\\w+) \\((.+)\\)"); // "John Doe (johndoe@example.com)"
  dump_code("https?:\\/\\/(?:www\\.)?([^\\/]+)"); //  "http://www.example.com/page.html"
  dump_code("(?<=\\d)\\w+(?=\\d)"); // "abc123def456"
}