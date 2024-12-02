#include <concepts>
#include <locale>
#include <cstring>
#include <cwchar>
#include <string>
#include <tuple>
#include <iostream>

#include "tokens.h"

namespace re {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // number of precedence levels in use, if you change this number, be sure to add 1 so that
    // ::store_cclass will have the highest precedence for it's own use (it's using NUM_LEVELS - 1).
    constexpr int NUM_LEVELS = 6;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the re_precedence_vec and re_precedence_stack work together to provide and n precedence levels
    // and (almost) unlimited amount of nesting. the nesting occurs when via the precedence stack
    // (push/pop) and the re_precedence_vec stores the current offset of the output code via
    // a store current precedence. did you get all of that?

    template<int sz>
    class re_precedence_vec : public std::vector<int> {
    public:
        explicit re_precedence_vec(const int init = 0) : std::vector<int>(sz, init) {
        }
    };

    typedef re_precedence_vec<NUM_LEVELS> re_precedence_element;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the actual precedence stack; adds a couple of handy members to keep track of the positions
    // in the re_precedence_element.

    class re_precedence_stack : public std::stack<re_precedence_element> {
    public:
        re_precedence_stack() : m_current(0) {
            push(re_precedence_element());
        }

        // current precedence value.
        int current() const { return m_current; }
        void current(const int l) { m_current = l; }

        // where we are int the input stream; index in array indicates precedence
        int start() const { return top().at(m_current); }
        void start(const int offset) { top().at(m_current) = offset; }

    private:
        int m_current;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // trivial stack used to keep track of the offsets in the code vector where jumps
    // should occur when changing to lower precedence operators. after a successful
    // compilation of a regular expression the future jump stack should be empty.

    typedef std::stack<int> re_future_jump_stack;
    typedef re_precedence_stack re_precedence_stack;

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


  template<typename traitsType>
      class compile_state {
  };


  template <class traitsType>
    class compiled_code_vector {
    public:
        typedef traitsType traits_type;
        typedef typename traits_type::char_type char_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::char_type code_type;
        //typedef re_compile_state<traitsT> compile_state_type;
  };


  template <class traitsType> class compiled_expression {
    // compiled_code_vector<charTraits> code;
    // precedence_stack<charTraits> precedence;
  };


  template <class traitsType>
    class syntax_compiler_base {
      public:
        typedef typename traitsType::string_type string_type;

        std::tuple<int, compiled_code_vector<traitsType>> compile(string_type s) {
          compiled_code_vector<traitsType> cv;
          return std::make_tuple(-1, cv);
        }
  };


  template<class traitsType>
  	class perl_syntax_compiler : public syntax_compiler_base<traitsType> {
      public:
        typedef typename traitsType::string_type string_type;

        std::tuple<int, compiled_code_vector<traitsType>> compile(string_type s) {
          compiled_code_vector<traitsType> cv;
          return std::make_tuple(-1, cv);
        }
    };
}

using namespace re;

int main() {
  using traits_type = char_traits<char>;
  perl_syntax_compiler<traits_type> perl;

  auto [result, cv] = perl.compile("[Hh]ello [Ww]orld");

  std::cout << "Result: " << result << std::endl;

  //cv.dump(std::cout);
}

