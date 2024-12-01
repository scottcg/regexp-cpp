#pragma once


/////////////////////////////////////////////////////////////////////////////////////
// these inlines are taking advantage of c++'s overloading; since i'll be using both
// wide and (narrow) char version of the is* and isw* functions, i'm using theses 
// rather than embedding some other kind of complication specializations.

inline int is_a_alpha(int c)		{ return isalpha(c); }
//inline int is_a_alpha(wint_t c)		{ return iswalpha(c); }
inline int is_a_alnum(int c)		{ return isalnum(c); }
//inline int is_a_alnum(wint_t c)		{ return iswalnum(c); }
inline int is_a_space(int c)		{ return isspace(c); }
//inline int is_a_space(wint_t c)		{ return iswspace(c); }
inline int is_a_digit(int c)		{ return isdigit(c); }
//inline int is_a_digit(wint_t c)		{ return iswdigit(c); }
inline int is_a_lower(int c)		{ return islower(c); }
//inline int is_a_lower(wint_t c)		{ return iswlower(c); }

inline int to_a_upper(int c)		{ return toupper(c); }
//inline int to_a_upper(wint_t c)	{ return towupper(c); }

// i really *really* want to get rid of these calls to sscanf so that i can drop
// #include <stdio.h> from the compiles; for now these are fine, but they must go.

inline int cstr_to_decimal_int(char* s) {
	int i = 0;
	sscanf(s, "%d", &i);
	return i;
}

inline int cstr_to_decimal_int(wchar_t* s) {
	int i = 0;
	swscanf(s, L"%d", &i);
	return i;
}

inline int cstr_to_octal_int(char* s) {
	int i = 0;
	sscanf(s, "%o", &i);
	return i;
}

inline int cstr_to_octal_int(wchar_t* s) {
	int i = 0;
	swscanf(s, L"%o", &i);
	return i;
}

inline int cstr_to_hex_int(char* s) {
	int i = 0;
	sscanf(s, "%x", &i);
	return i;
}

inline int cstr_to_hex_int(wchar_t* s) {
	int i = 0;
	swscanf(s, L"%x", &i);
	return i;
}

#if scg
inline int string_n_compare(const char* s1, const char* s2, size_t n) {
	return _strncoll(s1, s2, n);
}

inline int string_n_compare(const wchar_t* s1, const wchar_t* s2, size_t n) {
	return _wcsncoll(s1, s2, n);
}

inline int istring_n_compare(const char* s1, const char* s2, size_t n) {
	return _strnicoll(s1, s2, n);
}

inline int istring_n_compare(const wchar_t* s1, const wchar_t* s2, size_t n) {
	return _wcsnicoll(s1, s2, n);
}
#endif

inline int has_chars(const char* s1, const char* s2) {
	const char* t = strpbrk(s1, s2);
	return (t) ? (t - s1) : -1;
}

inline int has_chars(const wchar_t* s1, const wchar_t* s2) {
	const wchar_t* t = wcspbrk(s1, s2);
	return (t) ? (t - s1) : -1;
}

#include "code.h"
#include "source.h"


/////////////////////////////////////////////////////////////////////////////////////

enum opcodes {
	OP_END,				// end of compiled operators.
	OP_NOOP,			// no op; does nothing.
	OP_BACKUP,			// backup the current source character.
	OP_FORWARD,			// forward to the next source character.

	OP_BEGIN_OF_LINE,	// match to beginning of line.
	OP_END_OF_LINE,		// match to end of line.
	
	OP_STRING,			// match a string; string chars follow (+null)
	OP_BIN_CHAR,		// match a "binary" character (follows); usually constants.
	OP_NOT_BIN_CHAR,	// not (OP_BIN_CHAR).
	OP_ANY_CHAR,		// matches any single character (not a newline).
	OP_CHAR,			// match character (char follows), caseless if requested.
	OP_NOT_CHAR,		// does not match char (char follows), caseless if requested.
	OP_RANGE_CHAR,		// match a range of chars (two chars follow), caseless if requested.
	OP_NOT_RANGE_CHAR,	// not(OP_RANGE_CHAR).

	OP_BACKREF_BEGIN,	// a backref starts (followed by a backref number).
	OP_BACKREF_END,		// ends a backref address (followed by a backref number).
	OP_BACKREF,			// match a duplicate of backref contents (backref follows).
	OP_BACKREF_FAIL,	// check prior backref after failed alternate ex. ((a)|(b))

	OP_EXT_BEGIN,		// like a backref, that is "grouping ()", not in backref list (n follows)
	OP_EXT_END,			// closes an extension (n follows)
	OP_EXT,				// match a duplicate of extension contexts (n follows)
	OP_NOT_EXT,			// not (OP_EXT), (n follows)

	OP_GOTO,			// followed by 2 bytes (lsb, msb) of displacement.
	OP_PUSH_FAILURE,	// jump to address on failure.
	OP_PUSH_FAILURE2,	// i'm completely out of control
	OP_POP_FAILURE,		// pop the last failure off the stack
	OP_POP_FAILURE_GOTO,// combination of OP_GOTO and OP_POP_FAILURE
	OP_FAKE_FAILURE_GOTO,// push a dummy failure point and jump.
	
	OP_CLOSURE,			// push a min,max pair for {} counted matching.
	OP_CLOSURE_INC,		// add a completed match and loop if more to match.
	OP_TEST_CLOSURE,	// test min,max pair and jump or fail.
	
	OP_BEGIN_OF_BUFFER,	// match at beginning of buffer.
	OP_END_OF_BUFFER,	// match at end of buffer.

	OP_BEGIN_OF_WORD,	// match at the beginning of a word.
	OP_END_OF_WORD,		// match at the end of a word.

	OP_DIGIT,			// match using isdigit (one byte follow != 0 for complement)
	OP_SPACE,			// match using isspace (one byte follow != 0 for complement)
	OP_WORD,			// match using isalnum (one byte follow != 0 for complement)
	OP_WORD_BOUNDARY,	// match a word boundary (one byte follow != 0 for complement)

	OP_CASELESS,		// turn on caseless compares
	OP_NO_CASELESS,		// turn off caseless compares
	OP_LCASELESS,		// turn on lower caseless compares
	OP_NO_LCASELESS,	// turn off lower caseless compares
};

enum tokens {	// syntax codes for plain and quoted characters
	TOK_END				= 0xE00,	// signals end of stream (artifical).
	TOK_CHAR			= 0xE01,	// match single character.
	TOK_CTRL_CHAR		= 0xE02,	// a control character.
	TOK_ESCAPE			= 0xE03,	// the escape/quote (usually \) token.
	TOK_REGISTER		= 0xE04,	// match a backref.
	TOK_BACKREF			= 0xE05,	// match a backreference
	TOK_EXT_REGISTER	= 0xE06	// match \vnn style backrefs (10-99)
};

const int MAX_BACKREFS = 256;

/////////////////////////////////////////////////////////////////////////////////////
// compile_state contains all of the interesting bits necessary to pass around when
// compiling a regular expression. in more basic terms, i could replace this class
// by lengthening the function parameter lists, instead i have this class with a
// bunch of public members (and lengthening parameter lists using vc++ is a dangerous
// undertaking -- especially in templates).

template <class strT> class compile_state_base {
public:
	//typedef charT							char_type;
	//typedef intT							int_type;
	//typedef strT								string_type;
	typedef typename strT::char_traits::char_type		char_type;
	typedef typename strT::char_traits::int_type			int_type;
	typedef typename syntax_base<strT>		syntax_type;
	typedef typename basic_code_vector<strT>	code_vector_type;
	typedef typename source_help_vector<strT> source_vector_type;
	typedef typename std::stack<int>					open_backref_stack;

public:
	compile_state_base(const syntax_type& syn, code_vector_type& out, source_vector_type& in) 
		: op(0),
		ch(0),
		beginning_context(1),
		cclass_complement(false),
		number_of_backrefs(0),
		next_backref(1),
		backref_stack(),
		parenthesis_nesting(0),
		group_nesting(0),
		syntax(syn),
		output(out),
		input(in),
		jump_stack(),
		prec_stack() {}

	~compile_state_base() {}

public:
	int_type				op;						// current converted operation.
	int_type				ch;						// current input character.
	int						beginning_context;		// are we at the "start".
	bool					cclass_complement;		// is the current [] a [^].
	
	int						parenthesis_nesting;	// should be 0 when compile is done.
	int						group_nesting;			// should be 0 when compile is done.
	int						number_of_backrefs;		// number of () found.

	int						next_backref;			// next free backref number.
	open_backref_stack		backref_stack;			// stack for nesting backrefs.
	future_jump_stack		jump_stack;				// left open gotos.
	precedence_stack		prec_stack;				// operator precedence stack.

	const syntax_type&		syntax;					// what is our syntax object, this is
													// a const reference and passed in; i know
													// this violates the linton rule, but rules
													// are made to be broken.
	source_vector_type&		input;					// ref to the input character stream.
	code_vector_type&		output;					// ref to where the output code is going.

private:
	compile_state_base(const compile_state_base&);
	const compile_state_base& operator = (const compile_state_base&);
};
