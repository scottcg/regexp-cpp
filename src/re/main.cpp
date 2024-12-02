#include <locale>
#include <cstring>
#include <cwchar>
#include <string>
#include <tuple>
#include <iostream>

namespace re {
  template<class T>
  struct char_traits : std::char_traits<T> {
    typedef T char_type;
    typedef typename std::char_traits<T>::int_type int_type;
  	typedef std::basic_string<T> string_type;
  };

  template<>
  struct char_traits<char> : std::char_traits<char> {
    using traits_type = std::char_traits<char>;
  	typedef std::basic_string<char> string_type;

    static size_t length(const char_type *x) { return traits_type::length(x); }

    static int isalpha(const int_type c) { return std::isalpha(c); }
    static int isalnum(const int_type c) { return std::isalnum(c); }
    static int isspace(const int_type c) { return std::isspace(c); }
    static int isdigit(const int_type c) { return std::isdigit(c); }
    static int islower(const int_type c) { return std::islower(c); }
    static int toupper(const int_type c) { return std::toupper(c); }
    static int tolower(const int_type c) { return std::tolower(c); }
  };


  template <class charTraits> class compiled_code_vector {
    public:
      void dump(std::ostream& os) {
        os << "todo" << std::endl;
      }

      int offset() {
        return 0;
      }
  };


  template <class charTraits> class compiled_expression {
    // compiled_code_vector<charTraits> code;
    // precedence_stack<charTraits> precedence;
  };


  template <class charTraits>
    class syntax_compiler_base {
      public:
        typedef typename charTraits::string_type string_type;

        std::tuple<int, compiled_code_vector<charTraits>> compile(string_type s) {
          compiled_code_vector<charTraits> cv;
          return std::make_tuple(-1, cv);
        }
  };


  template<class charTraits>
  	class perl_syntax_compiler : public syntax_compiler_base<charTraits> {
      public:
        typedef typename charTraits::string_type string_type;

        std::tuple<int, compiled_code_vector<charTraits>> compile(string_type s) {
          compiled_code_vector<charTraits> cv;
          return std::make_tuple(-1, cv);
        }
    };
}

using namespace re;

int main() {
  using traits_type = char_traits<char>;
  perl_syntax_compiler<traits_type> perl;

  auto [result, cv] = perl.compile("[Hh]ello [Ww]orld");

  cv.dump(std::cout);
}

