#pragma once

#include <utility> // For std::pair
#include <vector>  // For std::vector
#include "concepts.h"
#include "traits.h"
#include "ctext.h"
#include "rcimpl.h"

///////////////////////////////////////////////////////////////////////////////////////////
// class description ---
//  basic_regular_expression is a class to compile/apply regular expressions to strings.
//  a regular expression must be compiled before one can compare another string to
//  the expression. compilation is done using "::compile" (go figure). after the
//  regular expression has been compiled there are three techniques to compare a
//  string to the regular expression:
//		::match compare a string to a re, the match must be exact. will return the
//			length of the match (>=0), or -1 for failed match, and finally -2 for an
//			internal error (closure stack overflow).
//		::search will walk through the string parameter calling ::match looking for
//			the string. returns (>=0) for the starting position for a successful
//			match, or -1 for failed search, and -2 for an error (closure stack overflow).
//		::partial_match will try to match a string "partially" to a regular expression.
//			returns the length of a partial match, -1 for no matches, -2 for an error
//			(closure stack overflow).
//
// additional members ---
//	caseless_compares can be used to "turn off/on" comparisons using casesless 
//		operators (that is "A+" will match "aA"), similar to "grep -i" or 
//		perl "/i"
//	lower_caseless_compares can be used to "turn off/on" lower caseness in comparisons.
//		so, "Aa" will match "AA" but not "aa". similar to "grep -y".
//
// implementation notes ---
//	the implementation is hidden via the rcimpl template. the basic_regular_expression_impl
//  is a reference counted/copy on write class, that means copying these is very 
//  inexpensive. a copy_on_write copy will cost a new/copy of the internal runtime
//  regular expression code.
// 
//  i've tried to stay away from new'ing memory from any of the member functions; you
//  shouldn't have to worry about deleting memory some any reference parameter or return
//  value (instead i'm using std::string's everywhere).
//

// forward declarations for classes we are going to need in basic_regular_expression.
template <class syntaxType> class re_engine;
template <class traitsType> class syntax_base;


namespace re {
	// when returning backreference matches, these are the classes used to do that
	typedef std::pair<int, int>			re_match_type;
	typedef std::vector<re_match_type>	re_match_vector;


	template <class synType> class basic_regular_expression {
	public:
		typedef synType						syntax_type;
		typedef typename syntax_type::traits_type	traits_type;
		typedef typename traits_type::string_type	string_type;
		typedef typename traits_type::int_type		int_type;
		typedef typename traits_type::char_type		char_type;
		typedef re_ctext<traits_type>		ctext_type;
		typedef re_engine<syntax_type>		engine_type;
		typedef rcimpl<engine_type>			engine_impl_type;

	public:
		basic_regular_expression() : m_impl(new engine_type()) {}
		basic_regular_expression(const char_type* s, size_t slen) : m_impl(new engine_type()) {
			compile(s, slen);
		}

		basic_regular_expression(const string_type& str) : m_impl(new engine_type()) {
			compile(str.data(), str.length());
		}

		basic_regular_expression(const basic_regular_expression& rhs) : m_impl(rhs.m_impl) {}

		virtual ~basic_regular_expression() {}

		const basic_regular_expression& operator = (const basic_regular_expression& rhs) {
			if ( this != &rhs ) {
				m_impl = rhs.m_impl;
			}
			return *this;
		}

	public:
		void caseless_compares(bool c) { m_impl->caseless_cmps = c; }
		void lower_caseless_compares(bool c) { m_impl->lower_caseless_cmps = c; }
		size_t maximum_closure_stack() const { return m_impl->maximum_closure_stack; }
		void maximum_closure_stack(size_t mx) { m_impl->maximum_closure_stack = mx; }

	public:
		int compile(const char_type* s, size_t slen = -1, int* err_pos = 0) {
			return m_impl->exec_compile(s, slen, err_pos);
		}
		int compile(const string_type& s, int* err_pos = 0) {
			return m_impl->exec_compile(s.data(), s.length(), err_pos);
		}

		int optimize() { return m_impl->exec_optimize(); }

	public:
		int match(const char_type* s, size_t slen = -1, size_t n = -1) const {
			ctext_type text(s, slen, 0, n);
			return m_impl->exec_match(text);
		}
		int match(const char_type* s, re_match_vector& m, size_t slen = -1, size_t n = -1) const {
			ctext_type text(s, slen, 0, n);
			return m_impl->exec_match(text, false, m);
		}
		
		int match(const string_type& s, size_t pos = 0, size_t n = -1) const {
			ctext_type text(s.data(), s.length(), 0, 0, pos, n);
			return m_impl->exec_match(text);
		}
		int match(const string_type& s, re_match_vector& m, size_t pos = 0, size_t n = -1) const {
			ctext_type text(s.data(), s.length(), 0, 0, pos, n);
			return m_impl->exec_match(text, false, m);
		}

	public:
		int partial_match(const char_type* s, size_t slen = -1, size_t n = -1) const {
			ctext_type text(s, slen, 0, 0, 0, n);
			return m_impl->exec_match(text, true);
		}
		int partial_match(const string_type& s, size_t pos = 0, size_t n = -1) const {
			ctext_type text(s.data(), s.length(), 0, 0, pos, n);
			return m_impl->exec_match(text, true);
		}

	public:
		int search(const char_type* s, size_t slen = -1, size_t n = -1) const {
			ctext_type text(s, slen, 0, n);
			return m_impl->exec_search(text);
		}
		int search(const char_type* s, re_match_vector& m, size_t slen = -1, size_t n = -1) const {
			ctext_type text(s, slen, 0, 0, n);
			return m_impl->exec_search(text, 0, m);
		}

		int search(const string_type& s, size_t pos = 0, size_t n = -1) const {
			ctext_type text(s.data(), s.length(), 0, 0, pos, n);
			return m_impl->exec_search(text);
		}
		int search(const string_type& s, re_match_vector& m, size_t pos = 0, size_t n = -1) const {
			ctext_type text(s.data(), s.length(), 0, 0, pos, n);
			return m_impl->exec_search(text, 0, m);
		}

	#if 0
		typedef std::vector < string_type > string_vector_type;
		int split(const char_type* s, size_t slen = -1, string_vector_type&, size_t pos = 0, size_t n = -1);
		int split(const char_type* s, size_t slen = -1, re_match_vector&, size_t pos = 0, size_t n = -1);
		int split(const string_type& s, string_vector_type&, size_t pos = 0, size_t n = -1);
		int split(const string_type& s, re_match_vector&, size_t pos = 0, size_t n = -1);

		int substitute(const string_type& s, const string_type& w, bool global = false,  size_t pos = 0, size_t n = -1);
		int substitute(const string_type& s, const string_vector_type&, bool global = false,  size_t pos = 0, size_t n = -1);
	#endif

	private:
		engine_impl_type	m_impl;
	};
}