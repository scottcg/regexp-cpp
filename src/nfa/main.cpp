
#include <string>
#include "ctext.h"

#if 0	// skip all of this for now

#pragma warning( disable : 4786 )

#include <vector>
#include "regexp.h"

#include <algorithm>
#include <map>
#include <iostream>
using namespace std;


int debugging_switch = 0;


// stuff for debugging (regardless of build)
extern "C" {
_CRTIMP void __cdecl _assert(void *, void *, unsigned);
};
#define always(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )


#define assert_match(N, S, E) {	always(m[N].first == S && m[N].second == E); }

void print_m(const match_vector& m) {
	cerr << endl;
	int j = 0;
	for ( match_vector::const_iterator i = m.begin(); i != m.end(); i++, j++ ) {
		cerr << "assert_match(" << j << ", "
			<< i->first << ", " << i->second << ");" << endl;
	}
}

#ifdef _UNICODE
typedef wperl_regexp regexp;
#define WIDE(STR) L##STR
typedef wchar_t CHAR;
#else
#define WIDE(STR) STR
typedef perl_regexp regexp;
typedef char CHAR;

#endif



#ifdef _UNICODE

template <class strT, class charT, class intT>
class universal_regexp_base : public basic_regexp<strT, charT, intT> {
public:
	typedef strT				string_type;
	typedef charT				char_type;
	typedef intT				int_type;
	typedef syntax_base<charT, intT>	syntax_base_type;

	enum syntax_type { PERL, AWK, EGREP, GREP };
public:
	universal_regexp_base(const char_type* s = 0, size_t slen = -1, syntax_type syn = PERL) {
		P_syntax = syn;
		if ( s ) {
			universal_compile(s, slen);
		}
	}

	universal_regexp_base(const string_type& s, syntax_type syn = PERL) {
		P_syntax = syn;
		universal_compile(s.data(), s.length());
	}

	universal_regexp_base(const universal_regexp_base<strT, charT, intT>& s) {
		P_syntax = s.P_syntax;
	}

	const universal_regexp_base& operator = (const universal_regexp_base& that) {
		if ( this != &that ) {
			P_syntax = that.P_syntax;
			basic_regexp<strT, charT, intT>::operator = (that);
		}
		return *this;
	}

	virtual ~universal_regexp_base() {
	}

	void syntax(syntax_type s) {
		P_syntax = s;
	}

protected:
	typedef syntax_regexp< awk_syntax<charT, intT>, strT > awk_regexp_type;
	typedef syntax_regexp< grep_syntax<charT, intT>, strT > grep_regexp_type;
	typedef syntax_regexp< egrep_syntax<charT, intT>, strT > egrep_regexp_type;
	typedef syntax_regexp< perl_syntax<charT, intT>, strT > perl_regexp_type;

	typedef awk_syntax<charT, intT> awk_syntax_type;
	typedef grep_syntax<charT, intT> grep_syntax_type;
	typedef egrep_syntax<charT, intT> egrep_syntax_type;
	typedef perl_syntax<charT, intT> perl_syntax_type;

	virtual const syntax_base_type& syntax_ref() const {
		switch ( P_syntax ) {
		default:
		case PERL: {
				static perl_syntax_type static_object;
				return static_object;
			}

		case AWK: {
				static awk_syntax_type static_object;
				return static_object;
			}

		case EGREP: {
				static egrep_syntax_type static_object;
				return static_object;
			}

		case GREP: {
				static grep_syntax_type static_object;
				return static_object;
			}
		}
	}

private:
	int universal_compile(const charT* s, size_t slen) {
		switch ( P_syntax ) {
		default:
		case PERL:
			return compile(perl_syntax_type(), s, slen, 0);
		case AWK:
			return compile(awk_syntax_type(), s, slen, 0);
		case EGREP:
			return compile(egrep_syntax_type(), s, slen, 0);
		case GREP:
			return compile(grep_syntax_type(), s, slen, 0);
		}
	}

	syntax_type	P_syntax;
};

template universal_regexp_base<std::wstring, wchar_t, wint_t>;
typedef universal_regexp_base<std::wstring, wchar_t, wint_t> wuniversal_regexp;
typedef universal_regexp_base<std::string, char, int> universal_regexp;

void test_universal_regexp_base() {
	wuniversal_regexp re(WIDE("[a-z]+[A-Z]"));
	re.compile(WIDE("ABC+"));
	int i = 0;

	i = re.match(WIDE("ABD"));
	always(i == -1);
	i = re.match(WIDE("ABCC"));
	always(i == 4);
}

#else
void test_universal_regexp_base() {}
#endif

void test_pre() {}

void test_simple() {
	regexp re;
	
	re.compile(WIDE("ABC+"));
	int i = 0;

	i = re.match(WIDE("ABD"));
	always(i == -1);
	i = re.match(WIDE("ABCC"));
	always(i == 4);
}


void test_compiler() {
	regexp re;

	re.compile(WIDE("AB*"));
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("ABBB")) == 4);
	always(re.match(WIDE("C")) == -1);

	re.compile(WIDE("AC*"));
	always(re.match(WIDE("ABBB")) == 1);
	always(re.match(WIDE("C")) == -1);
	always(re.match(WIDE("AC")) == 2);
	always(re.match(WIDE("BB")) == -1);

	re.compile(WIDE("AB+"));
	always(re.match(WIDE("ABBB")) == 4);

	re.compile(WIDE("AC+"));
	always(re.match(WIDE("ACCC")) == 4);

	re.compile(WIDE("A|B|C"));
	always(re.match(WIDE("A")) == 1 && re.match(WIDE("B")) == 1 && re.match(WIDE("C")));

	re.compile(WIDE("(A|B|C)"));
	always(re.match(WIDE("A")) == 1);

	re.compile(WIDE(""));
	always(re.match(WIDE("hi")) == 0);
	always(re.match(WIDE("")) == -1);
}


void test_char_class_parse() {
	regexp re;

	re.compile(WIDE("[-]"));
	always(re.match(WIDE("-")) == 1 && re.match(WIDE("a")) == -1);

	re.compile(WIDE("[+-]"));
	always(re.match(WIDE("+")) == 1);
	re.match(WIDE("-"));
	always(re.match(WIDE("-")) == 1);

	re.compile(WIDE("[-+]"));
	always(re.match(WIDE("-")) == 1);
	always(re.match(WIDE("+")) == 1);

	re.compile(WIDE("[A-B]"));
	always(re.match(WIDE("A")) == 1 && re.match(WIDE("B")) == 1);

	re.compile(WIDE("[-A-B]"));
	always(re.match(WIDE("-")) == 1);
	always(re.match(WIDE("A")) == 1 && re.match(WIDE("B")) == 1);

	re.compile(WIDE("[A-B-]"));
	always(re.match(WIDE("-")) == 1);
	always(re.match(WIDE("A")) == 1 && re.match(WIDE("B")) == 1);

	re.compile(WIDE("[-AB]"));
	always(re.match(WIDE("-")) == re.match(WIDE("A")) && re.match(WIDE("A")) == re.match(WIDE("B")));
	always(re.match(WIDE("-")) == 1);

	re.compile(WIDE("[A-Ca-c01234D-St-z]"));
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("z")) == 1);

	re.compile(WIDE("[-A-Ca-c01234D-St-z]"));
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("z")) == 1);
	always(re.match(WIDE("-")) == 1);

	re.compile(WIDE("[A-Ca-c01234D-St-z-]"));
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("z")) == 1);
	always(re.match(WIDE("-")) == 1);
}


void test_char_class() {
	regexp re;

	re.compile(WIDE("[a]"));
	always(re.match(WIDE("a")) == 1);

	re.compile(WIDE("[abcdef]"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("b")) == 1 && re.match(WIDE("c")) == 1 && re.match(WIDE("d")) == 1);

	re.compile(WIDE("a[b]c"));
	always(re.match(WIDE("abc")) == 3);
	always(re.match(WIDE("aBc")) == -1);
	
	re.compile(WIDE("a[bc]d"));
	always(re.match(WIDE("abd")) == re.match(WIDE("acd")));
	always(re.match(WIDE("ad")) == -1);

	re.compile(WIDE("[a-z]+"));
	always(re.match(WIDE("a")) == 1);	
	re.compile(WIDE("a[b]+d"));
	always(re.match(WIDE("abd")) == 3);
	always(re.match(WIDE("abbbd")) == 5);
	
	re.compile(WIDE("a[a-z]d"));
	always(re.match(WIDE("abd")) == 3);
	always(re.match(WIDE("aAd")) == -1);
	always(re.match(WIDE("aad")) == 3);

	re.compile(WIDE("[a-z]+1"));
	always(re.match(WIDE("a1")) == 2);
	always(re.match(WIDE("1")) == -1);

	re.compile(WIDE("[A-C]"));
	always(re.match(WIDE("A")) == 1 && re.match(WIDE("B")) == 1 && re.match(WIDE("C")) == 1);
	always(re.match(WIDE("D")) == -1);
}


void test_n_char_class() {
	regexp re;
	re.compile(WIDE("[a-z0-9]"));
	always(re.match(WIDE("a")) == 1 && re.match(WIDE("9")) == 1);

	re.compile(WIDE("[0-9a-z]"));
	always(re.match(WIDE("a")) == 1 && re.match(WIDE("9")) == 1);

	re.compile(WIDE("[0-9a-z]+"));
	always(re.match(WIDE("ab12")) == 4);

	re.compile(WIDE("[a-cc-ee-z]"));
	always(re.match(WIDE("a")) == 1 && re.match(WIDE("z")) == 1);
	always(re.match(WIDE("e")) == 1 && re.match(WIDE("c")) == 1);
}

void test_hat_char_class() {
	regexp re;
	re.compile(WIDE("[^a]"));
	always(re.match(WIDE("a")) == -1);
	re.compile(WIDE("a[^a-z]d"));
	always(re.match(WIDE("aAd")) == 3);
	always(re.match(WIDE("aad")) == -1);
	
	re.compile(WIDE("a[^a-z0-9]z"));
	always(re.match(WIDE("aAz")) == 3);
	always(re.match(WIDE("aaz")) == -1);
	always(re.match(WIDE("a0z")) == -1);

	re.compile(WIDE("[^ab]"));
	always(re.match(WIDE("c")) == 1);
	always(re.match(WIDE("a")) == -1);
	always(re.match(WIDE("b")) == -1);

	re.compile(WIDE("[^ab]C+"));
	always(re.match(WIDE("cC")) == 2);
	always(re.match(WIDE("C")) == -1);
	always(re.match(WIDE("aC")) == -1);
}


void test_nested_char_class() {
	regexp re;
	re.compile(WIDE("([A-Z]+)012"));
	always(re.match(WIDE("ABC012")) == 6);

}


void test_valid_patterns() {
	regexp re;
	re.compile(WIDE("^[ab]C+"));
	always(re.match(WIDE("aC")) == 2);
	always(re.match(WIDE("bC")) == 2);
	always(re.match(WIDE("foobC")) == -1);
	always(re.match(WIDE("C")) == -1);

	re.compile(WIDE("(a)(b)"));
	always(re.match(WIDE("ab")) == 2);

	re.compile(WIDE("a|b"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("b")) == 1);

	re.compile(WIDE("^[ab]C+"));
	always(re.match(WIDE("aC")) == 2);
	always(re.match(WIDE("bC")) == 2);
	always(re.match(WIDE("bCC")) == 3);

	re.compile(WIDE("^[^ab]C+"));
	always(re.match(WIDE("cC")) == 2);
	always(re.match(WIDE("aC")) == -1);
	always(re.match(WIDE("foocC")) == -1);

	re.compile(WIDE("^[ab]C+|^[^ab]C+"));
	re.match(WIDE("cC"));
	always(re.match(WIDE("cC")) == 2);
	always(re.match(WIDE("aC")) == 2);
	always(re.match(WIDE("bC")) == 2 && re.match(WIDE("aC")) == 2);

	re.compile(WIDE("ABC[a-z]*"));
	always(re.match(WIDE("ABC")) == 3);
	always(re.match(WIDE("ABCdd")) == 5);
	always(re.match(WIDE("ABD")) == -1);

	re.compile(WIDE("(ABC)([a-z]*)"));
	always(re.match(WIDE("ABC")) == 3);
	always(re.match(WIDE("ABCdd")) == 5);
	always(re.match(WIDE("ABCD")) == 3);
	always(re.match(WIDE("ABa")) == -1);

	re.compile(WIDE("(AB)([a-z]+)"));
	always(re.match(WIDE("ABc")) == 3);
	always(re.match(WIDE("AB")) == -1);
	always(re.match(WIDE("ABcc")) == 4);

	re.compile(WIDE("^(AB)([a-z]+)([0-9]?)(Z)"));
	always(re.match(WIDE("ABa1Z")) == 5);
	always(re.match(WIDE("ABaZ")) == 4);
	always(re.match(WIDE("ABaAZ")) == -1);

	re.compile(WIDE("^([0-9]+\\.?[0-9]*|\\.[0-9]+)$"));
	always(re.match(WIDE("123")) == 3);
	always(re.match(WIDE("123.34")) == 6);
	always(re.match(WIDE(".12")) == 3);
	always(re.match(WIDE("12.")) == 3);
	always(re.match(WIDE(" 12.")) == -1);

	re.compile(WIDE("^([^ ]*) *([^ ]*)"));
	always(re.match(WIDE("a b")) == 3);
	always(re.match(WIDE(" ")) == 1);

	re.compile(WIDE("Time of Day (..):(..):(..)"));
	always(re.match(WIDE("Time of Day 12:34:56")) == 20);
}


void test_anchors() {
	regexp re;
	re.compile(WIDE("foobar$"));
	always(re.match(WIDE("foobar")) == 6);
	always(re.match(WIDE("foobar more")) == -1);

	re.compile(WIDE("^foobar$"));
	always(re.match(WIDE("foobar")) == 6);
	always(re.match(WIDE("foobar more")) == -1);
	always(re.match(WIDE(" foobar")) == -1);
	always(re.search(WIDE(" foobar")) == -1);
	always(re.search(WIDE("foobar more")) == -1);

	re.compile(WIDE("^f$"));
	always(re.match(WIDE("f")) == 1);
	
	re.compile(WIDE("^f|^g"));
	always(re.match(WIDE("f")) == 1);
	always(re.match(WIDE("g")) == 1);

	re.compile(WIDE("^f$|^g$"));
	always(re.match(WIDE("f")) == 1);
	always(re.match(WIDE("ff")) == -1);
	always(re.match(WIDE("g")) == 1);
	always(re.match(WIDE("gg")) == -1);

	re.compile(WIDE("^foo$|^bar$"));
	always(re.match(WIDE("foo")) == 3);
	always(re.match(WIDE("bar")) == 3);

	re.compile(WIDE("(^foo$)|(^bar$)"));
	always(re.match(WIDE("foo")) == 3);
	always(re.match(WIDE("bar")) == 3);
	always(re.match(WIDE("foobar")) == -1);
	always(re.match(WIDE(" bar")) == -1);
	always(re.match(WIDE("bar ")) == -1);
}


void test_numbers() {
	regexp re;

	re.compile(WIDE("(a)\\1"));
	always(re.match(WIDE("aa")) == 2);
	
	re.compile(WIDE("(a)(b)\\2\\1"));
	always(re.match(WIDE("abba")) == 4);

	re.compile(WIDE("a\\1"));

	// test null handling in escapes
	re.compile(WIDE("a\\0"));
	CHAR m[3] = { 'a', 0, 0 };
	always(re.match(m, 2) == 2);
	re.compile(WIDE("a\\000"));
	always(re.match(m, 2) == 2);
	re.compile(WIDE("a\\000000"));
	always(re.match(m, 2) == 2);

	CHAR c[] = { 'a', '\\', '0', '0', '3', 0 };
	re.compile(c, 5);
}

#if 0
void test_invalid_patterns(regexp::SyntaxOpt s = regexp::Egrep) {
	int which = 0;
	while ( 1 ) {
		try { 
			regexp re(s);
			switch ( which++ ) {
			case 0:
				// missing closing part of the class
				re.compile(WIDE("[A-"));
				always(1 == 0);
				break;

			case 1:
				// missing )
				re.compile(WIDE("(AB)(A"));
				always(1 == 0);
				break;

			case 2:
				// missing (
				re.compile(WIDE("(AB)A)"));
				always(1 == 0);
				break;
/* TODO
			case 3:
				// \0 is an illegal register
				re.compile(WIDE("[A-Z]a\\0")); // m null
				always(1 == 0);
				break;
*/
			case 4:
				break;

			default:
				return;
			}
		} catch ( ... ) {
		}
	}
	always(0); // we should never get here.
}

#endif

void test_caseless() {
	regexp re;

	re.compile(WIDE("^Time of Day (..):(..):(..)"));
	re.caseless_compares(true);
	always(re.match(WIDE("time of day 12:34:56")) == 20);
}

#if vc5
void test_pat_matching() {
	regexp re(WIDE("([A-Z]+[0123]+[0-9]*|[A-Z]+[a-z]*[0-9]*)"));

	always(re.match(WIDE("AAA1239")) == 7);
	always(re.match(WIDE("A1")) == 2);
	always(re.match(WIDE("111")) == -1);

	re.compile(WIDE("[A-Z]+[a-z]+[0-5]*A+B*C+"));
	always(re.match(WIDE("Aa1ABC")) == 6); // match every token.
	always(re.match(WIDE("AaAC")) == 4); // match required token.
}
#endif

#if vc5
void test_pat_searching() {
	regexp re(WIDE("[A-Z]+"));

	always(re.search(WIDE("look for THIS pattern")) == 9);
	always(re.search(WIDE("look for T, that is it")) == 9);
	always(re.search(WIDE("look for THIS")) == 9);
	always(re.search(WIDE("look for T")) == 9);
	always(re.search(WIDE("THIS is what to find")) == 0);
	always(re.search(WIDE("T")) == 0);
	always(re.search(WIDE(" T")) == 1);
	always(re.search(WIDE("aT")) == 1);

	re.compile(WIDE("[a-z]+"));
	always(re.search(WIDE("LOOK FOR this pattern")) == 9);
}
#endif

void test_partial_m() {
	regexp re;
	re.compile(WIDE("aB+B"));

	always(re.partial_match(WIDE("a")) == 1);
	always(re.partial_match(WIDE("aa")) == 1);
	always(re.partial_match(WIDE("aB")) == 2);

	re.compile(WIDE("[A-Z][A-Z]+"));
	always(re.match(WIDE("A")) == -1);
	always(re.partial_match(WIDE("A")) == 1);
	always(re.partial_match(WIDE("Ab")) == 1);
	always(re.partial_match(WIDE("AA")) == 2);
}

void test_html_matching() {
	const CHAR* html = WIDE("   <!blah> <1>blah</1>\r\
	<B>more bold</B>");

	const CHAR* html2 = WIDE("asd asdf asdf \n\
	<b>bold</b>");

	const CHAR* html3 = WIDE("     \n\
		\r\
	\t\
		<b>bold</b>");

	regexp re1(WIDE("<[A-Za-z0-9]+"));
	always(re1.search(html) == 11);

	regexp re2(WIDE("</[A-Za-z0-9]+>"));
	always(re2.search(html) == 18);

	regexp re3(WIDE("[^\n\r\t <>]"));
	always(re3.search(html2) == 0);
	always(re3.search(html3) == 14);
}

void test_buggy_expr() {
	regexp b1(WIDE("include"));
	always(b1.search(WIDE("this is it include")) == 11);
}

void test_match_counting() {
	regexp re;
	match_vector m;

	debugging_switch = 0;
	re.compile( WIDE("(a){1,}a") );
	re.match( WIDE("aaa") );
	always(re.match( WIDE("aaa"), m ) == 3);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile(WIDE("B{2,2}C"));
	re.match( WIDE("BBC") );

	always(re.match(WIDE("BBC")) == 3);
	always(re.match(WIDE("BC")) == -1);
	always(re.match(WIDE("BBBC")) == -1);

	re.compile(WIDE("B{2}C"));	// same as {2,2}
	always(re.match(WIDE("BBC")) == 3);
	always(re.match(WIDE("BC")) == -1);
	always(re.match(WIDE("BBBC")) == -1);

	re.compile(WIDE("B{,2}C"));
	always(re.match(WIDE("BC")) == 2);
	always(re.match(WIDE("BBC")) == 3);
	always(re.match(WIDE("BBBC")) == -1);

	re.compile(WIDE("B{2,}C"));
	always(re.match(WIDE("BBC")) == 3);
	always(re.match(WIDE("BBBC")) == 4);

	re.compile(WIDE("B{2,3}C"));
	always(re.match(WIDE("BC")) == -1);
	always(re.match(WIDE("BBC")) == 3);
	always(re.match(WIDE("BBBC")) == 4);
	always(re.match(WIDE("BBBBC")) == -1);

	re.compile(WIDE("B{2,3}C"));
	re.caseless_compares(true);
	always(re.match(WIDE("bc")) == -1);
	always(re.match(WIDE("bbc")) == 3);
	always(re.match(WIDE("bbbc")) == 4);
	always(re.match(WIDE("bbbbc")) == -1);

	re.compile(WIDE("B{2}C{3}"));
	always(re.match(WIDE("BBCCC")) == 5);

	re.compile(WIDE("(B{1}C){1}"));
	always(re.match(WIDE("BC")) == 2);

	re.compile(WIDE("(BC)+"));
	always(re.match(WIDE("B")) == -1);
	always(re.match(WIDE("BC")) == 2);
	always(re.match(WIDE("BCBCBC")) == 6);

	re.compile(WIDE("(BC){2}"));
	always(re.match(WIDE("BBC")) == -1);

	re.compile( WIDE("(B+C){2}") );
	always(re.match( WIDE("BBCBBC"), m ) == 6);
	assert_match(0, 0, 6);
	assert_match(1, 3, 3);

	re.compile( WIDE("(B{2}C)+") );
	always(re.match( WIDE("BBCBBC"), m) == 6);
	assert_match(0, 0, 6);
	assert_match(1, 3, 3);

	re.compile( WIDE("(B{2}C){2}") );
	always(re.match( WIDE("BBCBBC"), m) == 6);
	assert_match(0, 0, 6);
	assert_match(1, 3, 3);

	re.compile(WIDE("A((B{2}C)D{1,2}){2,4}"));
	always(re.match( WIDE("ABBCDDBBCDD"), m) == 11);
	assert_match(0, 0, 11);
	assert_match(1, 6, 5);
	assert_match(2, 6, 3);

	re.compile( WIDE("((((a{1,}){1,}){1,}){1,})") );
	re.match( WIDE("aaa"), m);
}



void test_matches() {
	match_vector m;
	regexp re;
	int i = 0;

	// first, test that we are matching the correct number of times
	re.compile(WIDE("(a)"));
	always(re.match(WIDE("a")) == 1 && re.match(WIDE("a"), m) == 1);
	always(m.size() == 2);

	re.compile(WIDE("a"));
	always(re.match(WIDE("a"), m) == 1);
	always(m.size() == 1);

	// now, test the actual m 
	re.compile(WIDE("(a)(b)(c)"));
	re.match(WIDE("abc"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 1);
	assert_match(2, 1, 1);
	assert_match(3, 2, 1);

	re.compile(WIDE("(ab)?(A)+"));
	re.match(WIDE("abAA"), m);
	assert_match(0, 0, 4);
	assert_match(1, 0, 2);
	assert_match(2, 3, 1);

	re.compile(WIDE("((abc)+)"));
	always(re.match(WIDE("abcabc"), m) == 6);
	assert_match(0, 0, 6);
	assert_match(1, 0, 6);
	assert_match(2, 3, 3);

	re.compile(WIDE("((abc)+( )?)(hij)*"));
	re.match(WIDE("abcabc"));
	assert_match(0, 0, 6);
	assert_match(1, 0, 6);
	assert_match(2, 3, 3);

	re.compile(WIDE("((abc)+( )?)(hij)*"));
	re.match(WIDE("abc hijhij"), m);
	always(re.match(WIDE("abc hijhij"), m) == 10);
	always(m.size() == 5);
	assert_match(0, 0, 10);
	assert_match(1, 0, 4);
	assert_match(2, 0, 3);
	assert_match(3, 3, 1);
	assert_match(4, 7, 3);

	re.match(WIDE("z"), m);
	always(m.size() == 0);

	// now the hard part, start checking to | failures. that is, backup
	// up before an already m backreference.

	re.compile(WIDE("(a)b|(a)c"));

	re.match(WIDE("ab"), m);
	assert_match(0, 0, 2);
	assert_match(1, 0, 1);
	
	re.match(WIDE("ac"), m);
	assert_match(0, 0, 2);
	assert_match(1, 0, 0);
	assert_match(2, 0, 1);

	re.compile(WIDE("(a)(ab|ac)"));
	re.match(WIDE("aab"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 1);
	assert_match(2, 1, 2);
	
	re.match(WIDE("aac"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 1);
	assert_match(2, 1, 2);

	re.compile(WIDE("((a)+|(b)+)"));
	re.match(WIDE("aaa"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 3);
	assert_match(2, 2, 1);

	re.match(WIDE("b"), m);
	assert_match(0, 0, 1);
	assert_match(1, 0, 1);
	assert_match(2, 0, 0);
	assert_match(3, 0, 1);

	re.compile(WIDE("(a)+(b)*|(b)+(a)*"));
	re.match(WIDE("aaabbb"), m);
	assert_match(0, 0, 6);
	assert_match(1, 2, 1);
	assert_match(2, 5, 1);

	re.match(WIDE("bbbaaa"), m);
	assert_match(0, 0, 6);
	assert_match(1, 0, 0);
	assert_match(2, 0, 0);
	assert_match(3, 2, 1);
	assert_match(4, 5, 1);

	re.compile(WIDE("(a+)(b*)|(b+)(a*)"));
	re.match(WIDE("aaabbb"), m);
	assert_match(0, 0, 6);
	assert_match(1, 0, 3);
	assert_match(2, 3, 3);
	
	re.match(WIDE("aaa"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 3);

	re.match(WIDE("bbbaaa"), m);
	assert_match(0, 0, 6);
	assert_match(1, 0, 0);
	assert_match(2, 0, 0);
	assert_match(3, 0, 3);
	assert_match(4, 3, 3);	
	
	re.match(WIDE("bbb"), m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 0);
	assert_match(2, 0, 0);
	assert_match(3, 0, 3);
}

// #ifndef _UNICODE

void test_perl_closures() {
#ifdef _UNICODE
	wperl_regexp re;
#else
	perl_regexp re;
#endif

	re.compile(WIDE("AB{0,}"));		// AB*
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("AB")) == 2);
	always(re.match(WIDE("ABB")) == 3);

	re.compile(WIDE("AB{1,}"));		// AB+
	always(re.match(WIDE("A")) == -1);
	always(re.match(WIDE("AB")) == 2);
	always(re.match(WIDE("ABB")) == 3);

	re.compile(WIDE("AB{0,1}"));	// AB?
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("AB")) == 2);
	always(re.match(WIDE("ABB")) == 2);

	re.compile(WIDE("AB?"));
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("AB")) == 2);
	always(re.match(WIDE("ABB")) == 2);

	re.compile(WIDE("AB{0,1}$"));	// AB?$
	always(re.match(WIDE("A")) == 1);
	always(re.match(WIDE("AB")) == 2);
	always(re.match(WIDE("ABB")) == -1);

	re.compile(WIDE("([^ ]+)"));
	always(re.match(WIDE("abcd")) == 4);

	re.compile(WIDE("[^ ]{1,}"));
	always(re.match(WIDE("abcd")) == 4);

#if 0
	// foobar
	re.compile(WIDE("([^ ]|[^A]){1,}a"));
	re.compile( WIDE("(a|b){1,}a") );
#endif

	re.compile( WIDE("(a|b){1}a") );
	always(re.match( WIDE("b") ) == -1);
	always(re.match(WIDE("ba")) == 2);

	const CHAR* telno = WIDE("\\({0,1}([2-9][01][0-9])\\){0,1} *([2-9][0-9]{2})[ -]{0,1}([0-9]{4})");
	re.compile(telno);
	always(re.match(WIDE("(800) 321-4567")) == 14);
}


void test_grep() {
#ifdef _UNICODE
	wgrep_regexp re;
#else
	grep_regexp re;
#endif

	re.compile(WIDE("abc*"));
	always(re.match(WIDE("abc")) == 3);
	re.compile(WIDE("abc+"));
	always(re.match(WIDE("abc+")) == 4);
	re.compile(WIDE("abc?"));
	always(re.match(WIDE("abc?")) == 4);
	re.compile(WIDE("(ab)"));
	always(re.match(WIDE("(ab)")) == 4);
	
	re.compile(WIDE("ab|cd"));
	always(re.match(WIDE("ab|cd")) == 5);

	re.compile(WIDE("\\(ab\\)"));
	always(re.match(WIDE("ab")) == 2);
	re.compile(WIDE("\\(ab\\)\\1"));
	always(re.match(WIDE("abab")) == 4);

	re.compile(WIDE("\\(\\(a\\)\\2\\)\\1"));
	always(re.match(WIDE("aaaa")) == 4);
}


void test_egrep() {
#ifdef _UNICODE
	wegrep_regexp re;
#else
	egrep_regexp re;
#endif

	re.compile(WIDE("(^[^aeiou]*a[^aeiou]*e[^aeiou]*i[^aeiou]*o[^aeiou]*u[^aeiou]*)"));
	always(re.match(WIDE("facetious")) == 9);

	re.compile(WIDE("abc+"));
	always(re.match(WIDE("abc")) == 3);
	re.compile(WIDE("abc?$"));
	always(re.match(WIDE("abc")) == 3 && re.match(WIDE("ab")) == 2 && re.match(WIDE("abcc")) == -1);

	re.compile(WIDE("ab|cd"));
	always(re.match(WIDE("ab")) == 2 && re.match(WIDE("cd")) == 2);
}

void test_awk() {
	test_egrep();

#ifdef _UNICODE
	wawk_regexp re;
#else
	awk_regexp re;
#endif
	re.compile(WIDE("(black|blue)bird"));
	always(re.match(WIDE("blackbird")) == 9 && re.match(WIDE("bluebird")) == 8);

	re.compile(WIDE("^[+-]?([0-9]+[.]?[0-9]*|[.][0-9]+)([Ee][+-]?[0-9]+)?$"));
	always(re.match(WIDE("+23.56")) == 6);
	always(re.match(WIDE("12.45")) == 5);
	always(re.match(WIDE(".23")) == 3);
	always(re.match(WIDE("+.34")) == 4);

	re.compile(WIDE("^\\tabc"));
	always(re.match(WIDE("\tabc")) == 4);
	re.compile(WIDE("^\\f\\t\\nabc"));
	always(re.match(WIDE("\f\t\nabc")) == 6);
	// \b \f \n \r \t \ddd \c
}

void test_perl() {
#ifdef _UNICODE
	wperl_regexp re;
#else
	perl_regexp re;
#endif

	re.compile(WIDE("[abc]{2,3}def\\d"));
	always(re.match(WIDE("abcdef9")) == 7);
	re.compile(WIDE("[abc]{2}\\D."));
	always(re.match(WIDE("abcc")) == 4);

	re.compile(WIDE("\\w+"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("aa")) == 2);
	always(re.match(WIDE(",")) == -1);

	re.compile(WIDE("\\W+"));
	always(re.match(WIDE(",")) == 1);
	always(re.match(WIDE(".")) == 1);
	always(re.match(WIDE("a")) == -1);
	always(re.match(WIDE("A")) == -1);
	always(re.match(WIDE("9")) == -1);
	always(re.match(WIDE(",9")) == 1);
	always(re.match(WIDE(",,,")) == 3);
	always(re.match(WIDE(".,$#@")) == 5);

	re.compile(WIDE("\\s+"));
	always(re.match(WIDE("  ")) == 2);
	always(re.match(WIDE("\t ")) == 2);
	always(re.match(WIDE("a")) == -1);

	re.compile(WIDE("\\S+"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE(" ")) == -1);

	re.compile(WIDE("\\d+"));
	always(re.match(WIDE("0123456789")) == 10);
	always(re.match(WIDE("a123456789")) == -1);

	re.compile(WIDE("\\D+"));
	always(re.match(WIDE("0")) == -1);
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("abc")) == 3);

	re.compile(WIDE("\\d{1,2}"));
	always(re.match(WIDE("01")) == 2);
	always(re.match(WIDE("012")) == 2);

	re.compile(WIDE("\\d{2,}"));
	always(re.match(WIDE("12345")) == 5);

	re.compile(WIDE("\\d{2}"));
	always(re.match(WIDE("01")) == 2);
	always(re.match(WIDE("0123")) == 2);

	re.compile(WIDE("^foo\\bfoo$"));
	// re.dump_code();
	// cerr << "perl ^foo\\bfoo$" << re.match(WIDE("foo foo")) << endl;

	re.compile(WIDE("foo\\Bfoo"));
	
	re.compile(WIDE("^Message-ID:\\s*(.*)")); // mi;
	re.caseless_compares(true);
	always(re.match(WIDE("Message-Id: <199606071315.JAA01859@humm.whoi.edu>")) == 49);

	re.compile(WIDE("^Subject:\\s*(Re:\\s*)*(.*)")); // mi
	always(re.match(WIDE("Subject: IonaSphere No. #18")) == 27);
	always(re.match(WIDE("Subject: Re: Re: Your last bug!")) == 31);

	// /((a{0,5}){0,5}){0,5}/


}

void test_std() {
#ifdef _UNICODE
	std::wstring s(WIDE("this is a STRING"));
#else
	std::string s(WIDE("this is a STRING"));
#endif

	regexp re(WIDE("this is a STRING"));
	always(re.match(s) == 16);

	re.compile(s);
	always(re.match(WIDE("this is a STRING")) == 16);
	always(re.search(WIDE("this is a STRING")) == 0);
	always(re.search(s) == 0);
}


template <class charT>
void test_const_ref(const regexp& re, const charT* s, int n) {
	always(re.match(s) == n);
}

template <class charT>
void test_ref(regexp& re, const charT* s, int n) {
	always(re.match(s) == n);
}

template <class charT>
void test_cctor(regexp re, const charT* s, int n) {
	always(re.match(s) == n);
	re.compile(WIDE("falmouth sucks"));
	always(re.match(WIDE("falmouth sucks")) == 14);
}

void test_ctors() {
	regexp re;
	re.compile(WIDE("[a-z]+123"));
	always(re.match(WIDE("a123")) == 4);

	regexp re_copy(re);
	always(re_copy.match(WIDE("a123")) == 4);
	always(re.match(WIDE("a123")) == 4);

	re_copy.compile(WIDE("123"));
	always(re_copy.match(WIDE("123")) == 3);
	always(re.match(WIDE("a123")) == 4);

	regexp re_assign;
	re_assign = re_copy;
	always(re_assign.match(WIDE("123")) == 3);
	always(re_copy.match(WIDE("123")) == 3);
	always(re.match(WIDE("a123")) == 4);

	re_copy = re;
	always(re_copy.match(WIDE("a123")) == re.match(WIDE("a123")) && re.match(WIDE("a123")) == 4);

	test_ref(re_copy, WIDE("a123"), 4);
	test_cctor(re_copy, WIDE("a123"), 4);
	test_cctor(re, WIDE("a123"), 4);
	always(re_copy.match(WIDE("a123")) == re.match(WIDE("a123")) && re.match(WIDE("a123")) == 4);

	test_ref(re_copy, WIDE("a123"), 4);
	test_const_ref(re_copy, WIDE("a123"), 4);
	test_const_ref(re, WIDE("a123"), 4);
	always(re_copy.match(WIDE("a123")) == re.match(WIDE("a123")) && re.match(WIDE("a123")) == 4);

	re.compile(WIDE("12ab"));
	test_const_ref(re, WIDE("12ab"), 4);
	always(re.match(WIDE("12ab")) == 4);
	test_cctor(re, WIDE("12ab"), 4);
	always(re.match(WIDE("12ab")) == 4);

	const regexp const_re(WIDE("AB45"));
	always(const_re.match(WIDE("AB45")) == 4);
	test_const_ref(const_re, WIDE("AB45"), 4);
	test_cctor(const_re, WIDE("AB45"), 4);
}


void test_backrefs_matching() {
	regexp re;

	re.compile(WIDE("(foo)\\1"));
	always(re.match(WIDE("foofoo")) == 6);
	always(re.match(WIDE("foo")) == -1);
	
	re.compile(WIDE("(a)(b)\\2\\1"));
	always(re.match(WIDE("abba")) == 4);
	re.compile(WIDE("(a)(b)\\1\\2"));
	always(re.match(WIDE("abab")) == 4);

	re.compile(WIDE("(a)\\1?"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("aa")) == 2);

	re.compile(WIDE("(a)(\\1)?"));
	always(re.match(WIDE("a")) == 1);
	always(re.match(WIDE("aa")) == 2);

	re.compile(WIDE("(a+)(\\1b)?"));
	always(re.match(WIDE("aa")) == 2);
	always(re.match(WIDE("aab")) == 2);

	re.compile(WIDE("(a*)(\\1b)?"));
	always(re.match(WIDE("ab")) == 1);

	re.compile(WIDE("(a*)(\\1b)+"));
	always(re.match(WIDE("b")) == 1);
	always(re.match(WIDE("aab")) == 3);

	re.compile(WIDE("(a+)(b+)(\\1\\2)"));
	always(re.match(WIDE("abab")) == 4);

	re.compile(WIDE("(((a+)(b+)c+)d+)"));
	always(re.match(WIDE("abcd")) == 4);
	always(re.match(WIDE("aabbccdd")) == 8);

	re.compile(WIDE("(((a+)(b+)c+)d+)\\1"));
	always(re.match(WIDE("abcdabcd")) == 8);

	re.compile(WIDE("(((a+)(b+)c+)d+)\\1\\4"));
	always(re.match(WIDE("abcdabcdb")) == 9);

	re.compile(WIDE("(((a+)(b+)c+)d+)\\1\\3"));
	always(re.match(WIDE("abcdabcda")) == 9);

	re.compile(WIDE("((a)(b))\\3?\\2"));
	always(re.match(WIDE("aba")) == 3);
	always(re.match(WIDE("abba")) == 4);
}


void test_stingy_greedy() {
	regexp re;

	re.compile(WIDE("a.*?b"));
	always(re.match(WIDE("ab")) == 2);
	always(re.match(WIDE("acb")) == 3);
	always(re.match(WIDE("acbbb")) == 3);

	re.compile(WIDE("a.+?b"));
	always(re.match(WIDE("abb")) == 3);
	always(re.match(WIDE("abbbb")) == 3);
	always(re.match(WIDE("ac")) == -1);

	re.compile(WIDE("a.??b"));
	always(re.match(WIDE("ab")) == 2);
	always(re.match(WIDE("acb")) == 3);
	always(re.match(WIDE("accb")) == -1);

	const CHAR* s = WIDE("The food is under the bar in the barn");
	re.compile(WIDE("foo(.*)bar"));
	always(re.match(s + re.search(s)) == 32); // "food is under the bar in the bar"

	re.compile(WIDE("foo(.*?)bar"));
	always(re.match(s + re.search(s)) == 21); // "food is under the bar"
	
	re.compile(WIDE("foo(.+?)bar"));
	always(re.match(s + re.search(s)) == 21);

	// cerr << re.compile(WIDE("foo(([a-z]*)?)bar")) << endl;
}

void test_lookaheads() {
	// (?=pattern) for the assertion
	// (?!pattern) for the negation
	// for example, /\bfoo(?!bar)\w+/ will match 'foostuff' but not
	// 'foo' or 'foobar'
	// is that the same as /\bfoo([^b][^a][^r])\w+/

	// NOTE: neither lookaheads or backreferences can appear before
	// there in-use context, that means things like /\1\s(\w+)/ and
	// /(?!foo)bar/ are not legal.
	//
	// how about recursive groups /\b(?{begin)\b.*\b(?}end)/b/i;
}

void test_word_boundary() {
}

void test_optimize() {
	regexp re(WIDE("string"));

	re.optimize();
	re.match(WIDE("string"));
	always(re.match(WIDE("string")) == 6);

	re.caseless_compares(true);
	always(re.match(WIDE("STRING")) == 6);

	re.compile(WIDE("AB"));
	re.optimize();
	always(re.match(WIDE("AB")) == 2);

	re.compile(WIDE("A"));
	re.optimize();
	always(re.match(WIDE("A")) == 1);
}

#ifdef _UNICODE

template <class regT>
class any_char_regexp {
public:
	any_char_regexp() : P_impl() {}
	virtual ~any_char_regexp() {}

	int compile(const wchar_t*, size_t slen = -1, int* err_pos = 0) { return 0; }
	int compile(const char* s, size_t slen = -1, int* err_pos = 0) { return 0; }

private:
	void widen_char(const char* s, size_t slen, wchar_t*);
	regT P_impl;
};


// typedef perl_syntax<wchar_t, wint_t> wperl_syntax;
typedef syntax_regexp< perl_syntax<wchar_t, wint_t>, std::wstring > wperl_regexp;
typedef any_char_regexp< wperl_regexp > aperl_regexp;

void foobar() {
	aperl_regexp re;
	re.compile("foo");
	re.compile(WIDE("foo"));
}

#endif


void test_new_backref() {
	regexp re;
	match_vector m;

#ifdef _UNICODE
	const wchar_t* str = WIDE("aaa");
#else
	const char* str = "aaa";
#endif

	re.compile( WIDE("([a])+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("(a)+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("(a)(a)(a)") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 1);
	assert_match(2, 1, 1);
	assert_match(3, 2, 1);

	re.compile( WIDE("a(a)a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("a(a)+") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 2, 1);

	re.compile( WIDE("(a)*") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 2, 1);

	re.compile( WIDE("(a)*a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("(a|b)+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("(b|a)+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);	
	
	re.compile( WIDE("(a|b)*a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);	
		
	re.compile( WIDE("(b|a)*a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);

	re.compile( WIDE("(aa)+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 2);

	re.compile( WIDE("(aa)*a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 2);

	re.compile( WIDE("(aa)?a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 2);

	re.compile( WIDE("((a)(a))+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 2);
	assert_match(2, 0, 1);
	assert_match(3, 1, 1);

	re.compile( WIDE("(((a)a)a)") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 0, 3);
	assert_match(2, 0, 2);
	assert_match(3, 0, 1);

	re.compile( WIDE("([^ ]+)[ ]+\\[(.+)\\][ ]+(\\w+)[ ]+((.*?)[ ]+(HTTP.+)$|(.+)$)") );
	re.match(WIDE("bos1b.delphi.com [Fri Sep 30 21:50:57 1994] GET /gifs/heaps.gif"), m);
	assert_match(0, 0, 63);
	assert_match(1, 0, 16);
	assert_match(2, 18, 24);
	assert_match(3, 44, 3);
	assert_match(4, 48, 15);
	assert_match(5, 0, 0);
	assert_match(6, 0, 0);
	assert_match(7, 48, 15);

	re.match(WIDE("bos1b.delphi.com [Fri Sep 30 21:50:57 1994] GET /gifs/heaps.gif HTTP/1.0"), m);
	assert_match(0, 0, 72);
	assert_match(1, 0, 16);
	assert_match(2, 18, 24);
	assert_match(3, 44, 3);
	assert_match(4, 48, 24);
	assert_match(5, 48, 15);
	assert_match(6, 64, 8);
	assert_match(7, 0, 0);

	re.compile( WIDE("((a)|(a))+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);
	assert_match(2, 1, 1);
	assert_match(3, 0, 0);

	re.compile( WIDE("(a|a)+a") );
	re.match(str, m);
	assert_match(0, 0, 3);
	assert_match(1, 1, 1);
}


void test_search() {
	regexp re;
	re.compile( WIDE("foo") );
	re.optimize();
	always(re.search( WIDE("this is foobar") ) == 8);
	always(re.search( WIDE("fo foo") ) == 3);
}


void test_suite() {
	test_simple();
	test_compiler();
	test_valid_patterns();
	
	test_char_class_parse();
	test_char_class();
	test_n_char_class();
	test_hat_char_class();
	test_nested_char_class();

	test_anchors();
#if 0
	test_invalid_patterns();
#endif
	test_caseless();
#if vc5
	test_pat_matching();
	test_pat_searching();
#endif

	test_partial_m();
	test_html_matching();
	test_buggy_expr();
	test_match_counting();
	test_matches();
	test_numbers();
	test_backrefs_matching();

	test_perl_closures();
	test_grep();
	test_egrep();
	test_awk();
	test_perl();
	test_std();
	
	test_ctors();

	test_word_boundary();
	test_stingy_greedy();
	test_universal_regexp_base();
	test_optimize();

	test_search();
}

// i've been dreading this; but the time has come to test the handling
// of octal and hex constants in strings. 

void test_constants() {
	regexp re;
	re.compile( WIDE("foo") );
}

// test things like '^asfa' and 'asd[^as]'; we are interest in what 
// happends with ^ when it's used in different locations; in particular
// defining what happens when we move things around.

void test_hats() {
	regexp re;
	re.compile( WIDE("^foobar") );
	re.compile( WIDE("foo[^bar]") );
}

void test_extreme() {
	regexp re;
	re.compile( WIDE("\".*\"") );

	match_vector mv;

	re.search( WIDE("The name \"McDonalds's\" is said \"makudonarudo\" in Japanese"), mv);
	always(mv[0].first == 9 && mv[0].second == 36);
}

main() {
	test_search();

	debugging_switch = 0;
	regexp re;
	match_vector m;

	test_suite();

	// these should eventually move the the test suite function.
	test_new_backref();

	test_constants();
	test_hats();

	test_extreme();
	return 0;
}

#endif		// skip all of this
