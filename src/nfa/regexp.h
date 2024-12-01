// $Header: /regexp/regexp.h 1     2/22/97 6:03p Scg $

#ifndef INCLUDE_BASIC_REGEXP_H
#define INCLUDE_BASIC_REGEXP_H

#include "rcimpl.h"


///////////////////////////////////////////////////////////////////////////////////////////
// each regular expression syntax is subclassed from this. the engine isn't completely
// (or nearly?) general enough to do whatever you want with regular expressions -- it was my
// impression that almost no one would be interested in writing new syntax classes to deal
// with different regular expression syntaxes.
//
// none of these classes contain cctor or assignment operators; since all of the derived
// classes (other than syntax_fast_all) don't have any data members they "work" exactly the
// same (thus static const objects of those classes is what i was planning to use).

// forward declaration for parameter to syntax_base. this is a implementation class (see
// syntax.h for definition) that's only interesting if you are writing a new syntax_base 
// derived class.

template <class strType> class compile_state_base;

const int SYNTAX_ERROR = -1;
const int BACKREFERENCE_OVERFLOW = -2;
const int EXPRESSION_TOO_LONG = -3;
const int ILLEGAL_BACKREFERENCE = -4;
const int ILLEGAL_CLOSURE = -5;
const int ILLEGAL_DELIMITER = -6;
const int ILLEGAL_OPERATOR = -7;
const int ILLEGAL_NUMBER = -8;
const int MISMATCHED_BRACES = -9;
const int MISMATCHED_BRACKETS = -10;
const int MISMATCHED_PARENTHESIS = -11;

template <class strType> class syntax_base {
public:
	typedef strType							string_type;
	typedef typename strType::traits_type::char_type	char_type;
	typedef typename strType::traits_type::int_type	int_type;
	typedef compile_state_base<strType>		compile_state_type;

public:
	syntax_base();
	virtual ~syntax_base();

public:
	virtual bool context_independent_ops() const; // vc5 = 0;

	virtual int precedence(int op) const; // vc5 = 0;
	virtual int compile(compile_state_type& cs) const;

	virtual int compile_opcode(compile_state_type& cs) const;
	virtual bool incomplete_eoi(compile_state_type& cs) const;
	virtual int translate_plain_op(compile_state_type& cs) const; // vc5 = 0;
	virtual int translate_escaped_op(compile_state_type& cs) const; // vc5 = 0;
	virtual int translate_cclass_escaped_op(compile_state_type& cs) const;
};


///////////////////////////////////////////////////////////////////////////////////////////
// generic syntax contains the syntax code for the "comman" re that existing in all of
// the derived classes. 
//
//  c			nonmetacharacter c
//  \c			escape sequence or literal character c
//  ^			beginning of string
//  $			end of string
//  .			any character
//  [ab...]		any character in ab...
//  [^ab...]	any character not in ab...
//  [a-b]		any character in the range a-b
//  [^a-b]		any character not in the range a-b
//  r*			zero or more occurrences of r
//

template <class strType>
class generic_syntax : public syntax_base<strType> {
public:
	typedef compile_state_base<strType>		compile_state_type;
	virtual bool context_independent_ops() const;

	virtual int translate_plain_op(compile_state_type&) const;
	virtual int translate_escaped_op(compile_state_type&) const;
	virtual int precedence(int op) const;
};


///////////////////////////////////////////////////////////////////////////////////////////
// grep contains tagged regular expressions.
//
//  c		any non-special character c matches itself
//  \c		turn off any special meaning of character c
//  ^		beginning of line
//  $		end of line
//  .		any single character
//  [...]	any one of character in ...; ranges (a-z) are legal
//  [^...]	any single character not in ...; ranges are legal
//  \n		with the n'th \( \) matched
//  r*		zero or more occurrences of r
//  r1r2	r1 followed by r2
//  \(...\)	tagged regular expression
//
//	no regular expression matches a newline (\n).

template <class strType>
class grep_syntax : public generic_syntax<strType> {
public:
	typedef compile_state_base<strType>		compile_state_type;
	virtual bool context_independent_ops() const;

	virtual bool incomplete_eoi(compile_state_type& cs) const;
	virtual int translate_escaped_op(compile_state_type& cs) const;
};


///////////////////////////////////////////////////////////////////////////////////////////
// egrep. the most interesting part of this is the lack of registers.
//
//  c		any non-special character c matches itself
//  \c		turn off any special meaning of character c
//  ^		beginning of line
//  $		end of line
//  .		any single character
//  [...]	any one of character in ...; ranges (a-z) are legal
//  [^...]	any single character not in ...; ranges are legal
//  r*		zero or more occurrences of r
//  r+		one or more occurrences of r
//  r?		zero or one occurrences of r
//  r1r2	r1 followed by r2
//  r1|r2	r1 or r2
//  (r)		regular expression r. () are used for grouping.
//
//	no regular expression matches a newline (\n).

template <class strType>
class egrep_syntax : public generic_syntax<strType> {
public:
	typedef compile_state_base<strType>		compile_state_type;
	virtual bool context_independent_ops() const;

	virtual bool incomplete_eoi(compile_state_type& cs) const;
	virtual int translate_plain_op(compile_state_type& cs) const;
	virtual int translate_escaped_op(compile_state_type& cs) const;
};


///////////////////////////////////////////////////////////////////////////////////////////
// awk regular expressions are almost just like egrep. like egrep, awk doesn't know
// anything about registers (tagged) but does allow () for grouping.
//

template<class strType>
class awk_syntax : public egrep_syntax<strType> {};


///////////////////////////////////////////////////////////////////////////////////////////
// perl extensions to awk; actually extensions to awk with some grep thrown in just to
// muddy things up. the grep part is registers, but perl registers don't need escaping.
//
//  these are some additional escapes, most of these look like they came
//  from emacs.
//	\d matches [0-9]
//	\D matches [^0-9]
//	\b matches on a word boundary (between \w and \W)
//	\B matches on a non-word boundary
//	\s [ \t\r\n\f]
//	\S [^ \t\r\n\f]
//	\w [0-9a-zA-Z]
//	\W [^0-9a-zA-Z]
//
//	{n,m} must occur at least n times but no more than m times.
//	{n,} must occur at least n times
//	{n} must match exactly n times
//	just a note:
//		* same as {0,}
//		+ same as {1,}
//		? same as {0,1}
//
//	\r\f\b\n inherited from awk.
//	\000 is an octal number, unless the number would match a prior
//		register.
//	\xNN as in \x7f matches that character (hex matching)
//	\cD matches control character \cL matches ^L
//
//  the really complicated part here is that one can use escapes inside
//  sets: so, \d\w\s\n\r\t\f\b are interpolated within a set...

template <class strType>
class perl_syntax : public awk_syntax<strType> {
public:
	typedef compile_state_base<strType>		compile_state_type;
	virtual bool incomplete_eoi(compile_state_type& cs) const;
	virtual int compile_opcode(compile_state_type& cs) const;
	virtual int translate_plain_op(compile_state_type& cs) const;
	virtual int translate_escaped_op(compile_state_type& cs) const;
	virtual int translate_cclass_escaped_op(compile_state_type& cs) const;
};


///////////////////////////////////////////////////////////////////////////////////////////
// class description ---
//  basic_regexp is a class to compile/compare regular expressions by strings.
//  a regular expression must be compiled before one can compare another string to
//  the expression. compilation is done using "::compile" (go figure). after the
//  regular expression has been compile there are three techniques to compare a
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
//	the implementation is hidden via the rcimpl template. the basic_regexp_impl is
//  is a reference counted/copy on write class, that means copying these is very 
//  inexpensive. a copy_on_write copy will cost a new/copy of the internal runtime
//  regular expression code.
//

// forward declarations for classes we are going to need in basic_regexp.
template <class strT> class basic_regexp_impl;


// when returning backreference matches, these are the classes used to do that
typedef std::pair<int, int>			match_type;
typedef std::vector<match_type>		match_vector;


template <class synType>
class basic_regexp {
public:
	typedef typename synType::string_type	string_type;
	typedef typename synType::int_type		int_type;
	typedef typename synType::char_type		char_type;
	typedef typename syntax_base<synType>	syntax_base_type;

public:
	basic_regexp();
	basic_regexp(const char_type* s, size_t slen);
	basic_regexp(const string_type& str);
	basic_regexp(const basic_regexp&);
	virtual ~basic_regexp();

	const basic_regexp& operator = (const basic_regexp&);

public:
	void caseless_compares(bool);
	void lower_caseless_compares(bool);
	size_t maximum_closure_stack() const;
	void maximum_closure_stack(size_t);

	int compile(const char_type* s, size_t slen = -1, int* err_pos = 0);
	int compile(const string_type& s, int* err_pos = 0);

	int optimize();

	int match(const char_type* s, size_t slen = -1, size_t n = -1) const;
	int match(const char_type* s, match_vector&, size_t slen = -1, size_t n = -1) const;
	
	int match(const string_type& s, size_t pos = 0, size_t n = -1) const;
	int match(const string_type& s, match_vector&, size_t pos = 0, size_t n = -1) const;

	int partial_match(const char_type* s, size_t slen = -1, size_t n = -1) const;
	int partial_match(const string_type& s, size_t pos = 0, size_t n = -1) const;

	int search(const char_type* s, size_t slen = -1, size_t n = -1) const;
	int search(const char_type* s, match_vector&, size_t slen = -1,	size_t n = -1) const;

	int search(const string_type&, size_t pos = 0, size_t n = -1) const;
	int search(const string_type&, match_vector&, size_t pos = 0, size_t n = -1) const;

	// void dump_code() const;

protected:
	virtual const syntax_base_type& syntax_ref() const = 0;
	// needed for derived classes; non-virtual.
	int compile(const syntax_base_type&, const char_type* s, size_t slen = -1, int* err_pos = 0);

private:
	rcimpl< basic_regexp_impl<string_type> >	m_impl;
};


///////////////////////////////////////////////////////////////////////////////////////////
//

template <class synType> class syntax_regexp : public basic_regexp<synType> {
public:
	typedef typename synType::string_type	string_type;
	typedef typename synType::char_type			char_type;
	typedef typename synType::int_type			int_type;
	typedef typename synType					syntax_type;
	typedef typename syntax_base<string_type>	syntax_base_type;

public:
	syntax_regexp();
	syntax_regexp(const char_type* s, size_t slen = -1);
	syntax_regexp(const string_type& s);

	virtual ~syntax_regexp();

protected:
	virtual const syntax_base_type& syntax_ref() const
	{
		return m_syntax;
	}

private:
	syntax_type		m_syntax;
};

template <class synT>
syntax_regexp<synT>::syntax_regexp()
	: basic_regexp<synT>() {
}

template <class synT>
syntax_regexp<synT>::syntax_regexp(const char_type* s, size_t slen)
	: basic_regexp<synT>() {
	compile(m_syntax, s, slen);
}

template <class synT>
syntax_regexp<synT>::syntax_regexp(const string_type& s)
	: basic_regexp<synT>() {
	compile(m_syntax, s.data(), s.length());
}

template <class synT>
syntax_regexp<synT>::~syntax_regexp() {
}


#ifdef _UNICODE

typedef syntax_regexp< perl_syntax<std::wstring> > wperl_regexp;
typedef syntax_regexp< grep_syntax<std::wstring> > wgrep_regexp;
typedef syntax_regexp< egrep_syntax<std::wstring> > wegrep_regexp;
typedef syntax_regexp< awk_syntax<std::wstring> > wawk_regexp;
typedef syntax_regexp< generic_syntax<std::wstring> > wgeneric_regexp;

#else

typedef syntax_regexp< perl_syntax<std::string> > perl_regexp;
typedef syntax_regexp< grep_syntax<std::string> > grep_regexp;
typedef syntax_regexp< egrep_syntax<std::string> > egrep_regexp;
typedef syntax_regexp< awk_syntax<std::string> > awk_regexp;
typedef syntax_regexp< generic_syntax<std::string> > generic_regexp;

#endif

#endif