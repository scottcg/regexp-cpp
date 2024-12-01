// $Header: /regexp/regexp.cpp 1     2/22/97 6:04p Scg $

/////////////////////////////////////////////////////////////////////////////////
// copyright (c) 1993-1996 jungleware, inc. w. falmouth, ma. 02574
//
// this is proprietary unpublished source code of jungleware, inc. it may not
// be reproduced or redistributed in whole or in part without the express written
// permission of jungleware, inc.
//

#include <assert.h>
#include <ctype.h>
#include <string>

#include <stack>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>

#include "regexp.h"
#include "syntax.h"
#include "regexpimpl.h"

#ifdef _UNICODE
template basic_regexp_impl<wstring>;
#else
template basic_regexp_impl<std::string>;
#endif


/////////////////////////////////////////////////////////////////////////////////
// these are some handy functions to handle checking the string parameters
// for defaults and nulls; almost every one of regexp's member functions 
// that take a string argument needs to call one of these functions.

inline void check_string(const char* s, size_t& slen) {
	if ( s != 0 && slen == std::basic_string<char>::npos ) {
		slen = strlen(s);
	}

	if ( slen == std::basic_string<char>::npos ) {
		slen = 0;
	}
}

inline void check_string(const wchar_t* s, size_t& slen) {
	if ( s != 0 && slen == std::basic_string<wchar_t>::npos ) {
		slen = wcslen(s);
	}

	if ( slen == std::basic_string<wchar_t>::npos ) {
		slen = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////

template <class synT>
basic_regexp<synT>::basic_regexp()
: m_impl(new basic_regexp_impl<synT::string_type>()) {}

template <class synT>
basic_regexp<synT>::basic_regexp(const char_type* s, size_t slen)
		: m_impl(new basic_regexp_impl<synT::string_type>()) {
	compile(s, slen);
}

template <class synT>
basic_regexp<synT>::basic_regexp(const string_type& str) 
		: m_impl(new basic_regexp_impl<synT::string_type>() ) {
	compile(str.data(), str.length());
}

template <class synT>
basic_regexp<synT>::basic_regexp(const basic_regexp& that)
		: m_impl(that.m_impl) {
}

template <class synT>
basic_regexp<synT>::~basic_regexp() {
}

template <class synT>
const basic_regexp<synT>&
basic_regexp<synT>::operator = (const basic_regexp& that) {
	if ( this != &that ) {
		m_impl = that.m_impl;
	}
	return *this;
}

template <class synT>
void basic_regexp<synT>::caseless_compares(bool c) {
	assert(m_impl != 0);
	m_impl->caseless_compares = c;
}

template <class synT>
void basic_regexp<synT>::lower_caseless_compares(bool c) {
	assert(m_impl != 0);
	m_impl->lower_caseless_compares = c;
}

template <class synT>
size_t basic_regexp<synT>::maximum_closure_stack() const {
	assert(m_impl != 0);
	return m_impl->maximum_closure_stack;
}

template <class synT>
void basic_regexp<synT>::maximum_closure_stack(size_t mx) {
	assert(m_impl != 0);
	m_impl->maximum_closure_stack = mx;
}

template <class synT>
int basic_regexp<synT>::compile(const char_type* s, size_t slen, int* err_pos) {
	assert(m_impl != 0);
	return m_impl->exec_compile(syntax_ref(), s, slen, err_pos);
}

template <class synT>
int basic_regexp<synT>::compile(const string_type& str, int* err_pos) {
	assert(m_impl != 0);
	return m_impl->exec_compile(syntax_ref(), str.data(), str.length(), err_pos);
}

template <class synT>
int basic_regexp<synT>::optimize() {
	assert(m_impl != 0);
	return m_impl->exec_optimize();
}

template <class synT>
int basic_regexp<synT>::match(const char_type* str1, size_t slen1,
										   size_t pos_stop) const {
	ctext<string_type> text(str1, slen1, 0, pos_stop);
	return m_impl->exec_match(text);
}

template <class synT>
int basic_regexp<synT>::match(const char_type* str1, match_vector& m, size_t slen1,
										   size_t pos_stop) const {
	ctext<string_type> text(str1, slen1, 0, pos_stop);
	return m_impl->exec_match(text, false, m);
}

template <class synT>
int basic_regexp<synT>::match(const string_type& str, size_t pos, size_t pos_stop) const {
	ctext<string_type> text(str.data(), str.length(), pos, pos_stop);
	return m_impl->exec_match(text);
}

template <class synT>
int basic_regexp<synT>::match(const string_type& str, match_vector& m, size_t pos, size_t pos_stop) const {
	ctext<string_type> text(str.data(), str.length(), pos, pos_stop);
	return m_impl->exec_match(text, false, m);
}

template <class synT>
int basic_regexp<synT>::partial_match(const char_type* str, size_t slen, size_t pos_stop) const {
	ctext<string_type> text(str, slen, 0, pos_stop);
	return m_impl->exec_match(text, true);
}

template <class synT>
int basic_regexp<synT>::partial_match(const string_type& str, size_t pos, size_t pos_stop) const {
	ctext<string_type> text(str.data(), str.length(), pos, pos_stop);
	return m_impl->exec_match(text, true);
}

template <class synT>
int basic_regexp<synT>::search(const char_type* str, size_t slen, size_t pos_stop) const {
	ctext<string_type> text(str, slen, 0, pos_stop);
	return m_impl->exec_search(text);
}

template <class synT>
int basic_regexp<synT>::search(const char_type* str, match_vector& m, size_t slen,
											size_t pos_stop) const {
	ctext<string_type> text(str, slen, 0, pos_stop);
	return m_impl->exec_search(text, 0, m);
}

template <class synT>
int basic_regexp<synT>::search(const string_type& str, size_t pos, size_t pos_stop) const {
	ctext<string_type> text(str.data(), str.length(), pos, pos_stop);
	return m_impl->exec_search(text);
}

template <class synT>
int basic_regexp<synT>::search(const string_type& str, match_vector& m,
							   size_t pos, size_t pos_stop) const {
	ctext<string_type> text(str.data(), str.length(), pos, pos_stop);
	return m_impl->exec_search(text, 0, m);
}

template <class synT>
int basic_regexp<synT>::compile(const syntax_base_type& syntax,
								const char_type* s, size_t slen, int* err_pos) {
	assert(m_impl != 0);
	return m_impl->exec_compile(syntax, s, slen, err_pos);
}


#if 0
#include <iostream>

template <class synT>
void basic_regexp<synT>::dump_code() const {
	std::ostream& out = std::cerr;
	const charT* cp = m_impl->code.code();
	int pos = 0;
	while ( (pos = (cp - m_impl->code.code())) < m_impl->code.offset() ) {
		out << '\t' << pos << '\t';

		switch ( *cp++ ) {
		case OP_END:
			out << "OP_END" << std::endl;
			break;

		case OP_BACKUP:
			out << "OP_BACKUP" << std::endl;
			break;

		case OP_FORWARD:
			out << "OP_FORWARD" << std::endl;
			break;

		case OP_BEGIN_OF_LINE:
			out << "OP_BEGIN_OF_LINE" << std::endl;
			break;

		case OP_BEGIN_OF_BUFFER:
		case OP_END_OF_BUFFER:
			out << "OP_BEGIN_OF_BUFFER/OP_END_OF_BUFFER" << std::endl;
			break;

		case OP_BEGIN_OF_WORD:
			out << "OP_BEGIN_OF_WORD" << std::endl;
			break;

		case OP_END_OF_WORD:
			out << "OP_END_OF_WORD" << std::endl;
			break;

		case OP_WORD:
			out << "OP_WORD (" << int(*cp++) << ")\n";
			break;

		case OP_WORD_BOUNDARY:
			out << "OP_WORD_BOUNDARY (" << int(*cp++) << ")\n";
			break;
		
		case OP_END_OF_LINE:
			out << "OP_END_OF_LINE" << std::endl;
			break;

		case OP_CHAR:
			out << "OP_CHAR (" << char(*cp++) << ")\n";
			break;

		case OP_NOT_CHAR:
			out << "OP_NOT_CHAR (" << char(*cp++) << ")\n";
			break;

		case OP_ANY_CHAR:
			out << "ANY_CHAR" << std::endl;
			break;
			
		case OP_RANGE_CHAR:
			out << "OP_RANGE_CHAR (" << char(*cp++);
			out << ',' << char(*cp++) << ")\n";
			break;

		case OP_NOT_RANGE_CHAR:
			out << "OP_NOT_RANGE_CHAR (" << char(*cp++);
			out << ',' << char(*cp++) << ")\n";
			break;

		case OP_BACKREF_BEGIN:
			out << "OP_BACKREF_BEGIN (" << int(*cp++) << ")\n";
			break;

		case OP_BACKREF_END:
			out << "OP_BACKREF_END (" << int(*cp++) << ")\n";
			break;
		
		case OP_BACKREF:
			out << "OP_BACKREF (" << int(*cp++) << ")\n";
			break;

		case OP_BACKREF_FAIL:
			out << "OP_BACKREF_FAIL" << std::endl;
			break;

		case OP_CLOSURE:
			out << "OP_CLOSURE (" << decode_address_and_advance(cp) << ") ";
			out << "{" << decode_address_and_advance(cp) << ",";
			out << decode_address_and_advance(cp) << "}" << std::endl;
			break;

		case OP_CLOSURE_INC:
			out << "OP_CLOSURE_INC (" << decode_address_and_advance(cp) << ")";
			out << " {" << decode_address_and_advance(cp) << ",";
			out << decode_address_and_advance(cp) << "}" << std::endl;
			break;

		case OP_TEST_CLOSURE:
			out << "OP_CLOSURE_TEST\n";
			break;

		case OP_DIGIT:
			out << "OP_DIGIT (" << int(*cp++) << ")\n";
			break;

		case OP_SPACE:
			out << "OP_SPACE (" << int(*cp++) << ")\n";
			break;

		case OP_GOTO:
			out << "OP_GOTO (" << decode_address_and_advance(cp) << ")\n";
			break;

		case OP_POP_FAILURE_GOTO:
			out << "OP_POP_FAILURE_GOTO (" << decode_address_and_advance(cp) << ")\n";
			break;

		case OP_FAKE_FAILURE_GOTO:
			out << "OP_FAKE_FAILURE_GOTO (" << decode_address_and_advance(cp) << ")\n";
			break;
		
		case OP_PUSH_FAILURE:
			out << "OP_PUSH_FAILURE (" << decode_address_and_advance(cp) << ")\n";
			break;

		case OP_PUSH_FAILURE2:
			out << "OP_PUSH_FAILURE2 (" << decode_address_and_advance(cp) << ")\n";
			break;

		case OP_POP_FAILURE:
			out << "OP_POP_FAILURE" << std::endl;
			break;

		case OP_NOOP:
			out << "OP_NOOP" << std::endl;
			break;

		default:
			out << "BAD CASE" << std::endl;
			break;
		}
	}
}
#endif
