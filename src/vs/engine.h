#pragma once

#include <vector>
#include <iostream>

#include "concepts.h"
#include "traits.h"
#include "ctext.h"
#include "compile.h"
#include "syntax.h"

namespace re {
	using ::std::ostream;
	using ::std::string;
	using ::std::vector;

	template class re_input_string< re_char_traits<char> >;
	//template class re_input_string< re_char_traits<wchar_t> >;

	template class re_ctext< re_char_traits<char> >;
	//template class re_ctext< re_char_traits<wchar_t> >;

	template <class T> class re_syntax_base;

	template class re_code_vec< re_char_traits<char> >;
	//template class re_code_vec< re_char_traits<wchar_t> >;

	template class generic_syntax< re_char_traits<char> >;
	//template class generic_syntax< re_char_traits<wchar_t> >;

	template class perl_syntax< re_char_traits<char> >;
	//template class perl_syntax< re_char_traits<wchar_t> >;

	typedef perl_syntax< re_char_traits<char> > perl_syntax_type;
	//typedef perl_syntax< re_char_traits<wchar_t> > wperl_syntax_type;

	typedef ::std::pair<int, int>			re_match_type;
	typedef vector<re_match_type>	re_match_vector;

	/////////////////////////////////////////////////////////////////////////////////
	// in the pair, the first is the starting position, the second is the length of
	// the match. if either of the pair is -1 then the backreference is invalid.
	typedef ::std::pair<int, int> executing_backref;

	/////////////////////////////////////////////////////////////////////////////////
	// re_engine<syntaxType> is hidden in here because i don't want to
	// publicize any of the implementation in the the interface class (basic_regexp). 
	// i really like the technique of splitting the interface/impl in c++, i this
	// is (imho) the best way to do that.
	//
	// the member functions of basic_regexp<synT, char_type, int_type>::impl are like what
	// one would see in the private.

	enum { ANCHOR_NONE = 0, ANCHOR_LINE = 1, ANCHOR_BUFFER = 2 };

	template <class syntaxType> class re_engine {
	public:
		typedef syntaxType							syntax_type;

		typedef typename syntax_type::traits_type			traits_type;
		typedef typename syntaxType::char_type				char_type;
		typedef typename syntaxType::int_type				int_type;
		typedef executing_backref					backref_type;
		typedef std::vector< executing_backref >	backrefs_type;

		typedef re_input_string<traits_type>		source_vector_type;
		typedef re_code_vec<traits_type>			code_vector_type;
		typedef re_compile_state<traits_type>		compile_state_type;
		typedef re_ctext<traits_type>				ctext_type;

	public:
		short						anchor;
		int							syntax_error_state;
		bool						caseless_cmps;
		bool						lower_caseless_cmps;
		int							using_backrefs;
		size_t						maximum_closure_stack;
		code_vector_type			code;
		syntax_type					syntax;

		static const re_match_vector	default_matches;

	public:
		re_engine();
		re_engine(const re_engine& that);
		const re_engine &operator =(const re_engine &that);
		
	public:
		int exec_compile(const char_type* s, size_t slen = -1, int* err = nullptr);
		int exec_match(ctext_type& text, bool partial_matches = false,
					re_match_vector& m = const_cast<re_match_vector &>(default_matches)) const;
		int exec_optimize();
		int exec_search(ctext_type& text, int range = 0,
					re_match_vector& m = const_cast<re_match_vector &>(default_matches)) const;
		void dump_code() const;
	};


	template <class syntaxType>
	const re_match_vector re_engine<syntaxType>::default_matches;

	template <class syntaxType>
	re_engine<syntaxType>::re_engine() : code() {
		anchor = ANCHOR_NONE;
		syntax_error_state = 1;			// default, no compiled expression.
		caseless_cmps = 0;
		lower_caseless_cmps = 0;
		using_backrefs = 0;
		maximum_closure_stack = 4096;
	}

	template <class syntaxType>
	re_engine<syntaxType>::re_engine(const re_engine<syntaxType>& that)
			: code(that.code) {
		anchor = that.anchor;
		syntax_error_state = 1;			// default, no compiled expression.
		caseless_cmps = that.caseless_cmps;
		lower_caseless_cmps = that.lower_caseless_cmps;
		using_backrefs = that.using_backrefs;
	}

	template <class syntaxType>
	const re_engine<syntaxType>&
	re_engine<syntaxType>::operator = (const re_engine<syntaxType>& that) {
		if ( this != &that ) {
			code = that.code;
			anchor = that.anchor;
			syntax_error_state = that.syntax_error_state;
			caseless_cmps = that.caseless_cmps;
			lower_caseless_cmps = that.lower_caseless_cmps;
			using_backrefs = that.using_backrefs;
		}
		return *this;
	}

	/////////////////////////////////////////////////////////////////////////////
	// compile a regular expression.
	// always will delete old code, by reinitializing the code object. will always
	// reset to the anchor to 0.
	// 
	// does not reinitialize case less comparison flags.
	//
	// returns 0 if no error
	// != 0 on error and sets *err_pos to the character to the position of error.
	//

	template <class syntaxType>
	int re_engine<syntaxType>::exec_compile(const char_type* s, size_t slen, int* err_pos) {
		anchor = 0;			// allow for repeat calls to ::exec_compile
		code.initialize();

		if ( err_pos ) {
			*err_pos = 0;	// be optimistic, set err_pos to 0.
		}

		if ( s == nullptr ) {
			return 0;		// garbage in, nothing out.
		}

		traits_type::check(s, slen);					// calculate slen if user passed default

		source_vector_type source(s, slen);
		compile_state_type cs(syntax, code, source);	// let syntax obj do the work.

		syntax_error_state = cs.syntax.compile(cs);		// perform the compilation
		if ( syntax_error_state ) {
			code.initialize();							// code isn't useful.
			if ( err_pos ) {
				*err_pos = cs.input.offset();			// where the error occurred.
			}
			return syntax_error_state;					// return pos in string where error occurred
		}
		assert(cs.jump_stack.size() == 0);
		using_backrefs = cs.number_of_backrefs;		// remember number of backrefs.
		return 0;										// no error
	}


	/////////////////////////////////////////////////////////////////////////////
	// optimize a compiled regular expression.
	// this member function is constantly changing, but, right now, this will take
	// simple character sequences and turn it into a single string operation.
	// 
	// we try to avoid resizing the code memory, since everything we are doing
	// should shorten the number of code operations. but, for now, i'm creating
	// a new code object and assigning it to the existing code object.
	//

	template <class syntaxType>
	int re_engine<syntaxType>::exec_optimize() {
		if ( syntax_error_state ) {
			return -3;
		}

		bool continue_work = true;
		typename code_vector_type::code_type char_count = 0;
		int i = 0;
		while ( continue_work && code[i] != OP_END ) {
			if ( code[i++] == OP_CHAR ) {
				++i;			// skip the operator
				char_count++;
			} else {
				continue_work = false;
			}
		}

		if ( continue_work == false || char_count < 2 ) { // don't bother if just a single char
			return 0;
		}

		int old_cursor = 0;
		if ( code[old_cursor] != OP_END ) { // just a plain string
			code_vector_type new_code(char_count + 3);
			// now, strip all OP_CHARs and turn the code into OP_STRING
			int new_cursor = 0;
			new_code[new_cursor++] = OP_STRING;
			new_code[new_cursor++] = char_count;
			while ( code[old_cursor] != OP_END ) {
				if ( code[old_cursor] == OP_CHAR ) {
					++old_cursor;
				} else {
					new_code[new_cursor++] = code[old_cursor++];
				}
			}
			new_code[new_cursor] = OP_END;
			code = new_code;
		}
		return 1;
	}

	/////////////////////////////////////////////////////////////////////////////
	// match stack object; these are used to store/save the counting values
	// when using the {} style operator; this is a template because some day
	// i'm going to switch to unicode -- and i want as much type checking that
	// i can get.
	//
	// the auto cctor is used; that's why cctor's aren't privatized.

	template <class char_type, class int_type> class re_closure {
	public:
		//typedef char_type		char_type;
		//typedef int_type		int_type;

	public:
		re_closure(const int_type* c = 0, const char_type* t = 0, int mi = -1, int mx = -1)
			: code(c), text(t), minimum(mi), maximum(mx), matched(0)
		{}

		int failure() const {
			return (maximum == -1 && minimum == -1);
		}

		int closed() const {
			if ( maximum == -1 ) {	// we are not match counting.
				return 1;
			} else if ( minimum == maximum && matched == minimum ) {
				return 1;
			} else if ( (minimum == 0 && matched <= maximum)
					|| (maximum == 0 && matched >= minimum) ) {
				return 1;
			} else if ( (minimum != 0 && maximum != 0)
					&& ( matched >= minimum && matched <= maximum) ) {
				return 1;
			}
			return 0;
		}
		
		int can_continue() const {
			assert(maximum != -1 && minimum != -1);	// don't call this unless counting.
			if ( minimum == maximum && matched < minimum ) {
				return 1;
			} else if ( (minimum != 0 && maximum != 0) && matched < maximum ) {
				return 1;
			} else if ( (minimum == 0 && matched < maximum) || maximum == 0 ) {
				return 1;
			}
			return 0;
		}
		// vc5
		bool operator < (const re_closure&) const { return false; }
		bool operator == (const re_closure&) const { return false; }

	public:
		const int_type*		code;
		const char_type*	text;
		int					minimum;
		int					maximum;
		int					matched;
	};


	/////////////////////////////////////////////////////////////////////////////
	// matching function
	//
	// this member will search for a match in the catenation (or just the str1)
	// of two strings starting at pos (within str1 or str1 + str2).
	// pos is the starting position in str1 + str2 (i.e. it can be > length
	// of str1). pos_stop is the stop position of (i.e. max length of search). 
	// part indicates if partial matches are acceptable.
	//
	// matching can be influenced by the external "ignoreCase" flag, so that
	// exact character matches can be modified to ignore case differences, that
	// is "C" == "c".
	//
	// parameters:
	//  if you pass -1 for either of the sizes then ::strlen will be called to
	//		calculate the string length.
	//	if you pass 0 for part then no partial matches will be taken.
	//	if you pass -1 for pos_stop is will be set to len1 + len2.
	//
	// returns:
	//   >0 to indicate the length in characters for a succesful match
	//   -1 to indicate an unsuccessful match
	//   -2 to indicate an error, either stack overflow or a bogus re that passed
	//		the compiler.
	//
	//   it should never return a 0; although, i haven't completely proved that it's
	//	 not possible.
	//

	inline int decode_address_and_advance(const char*& cp) {
		short r = *cp++;
		r |= *cp++ << 8;
		return static_cast<int>(r);
	}

	inline int decode_address_and_advance(const wchar_t*& cp) {
		wint_t r = *cp++;
		r |= *cp++ << 8;
		return static_cast<int>(r);
	}


	typedef ::std::pair<int, int> runtime_backref;

	inline ::std::ostream& operator << (::std::ostream& out, const runtime_backref& p) {
		return out << '(' << p.first << ',' << p.second << ')';
	}

	template <class syntaxType>
	int re_engine<syntaxType>::exec_match(ctext_type& text,
										bool partial_matches, re_match_vector& matches) const {
		if ( syntax_error_state ) {
			return -3;
		}

		// if want matches, we delete everything regardless of any future matches
		if ( &matches != &default_matches ) {
			matches.erase(matches.begin(), matches.end());
		}

		// check to see if we have compiled expression, and something to match
		if ( code.offset() == 0 || text.length() == 0 ) {
			return -1;
		}

		typedef std::vector < std::pair<int, int> > match_count_vector;
		match_count_vector mv(0);

		typedef re_closure< char_type, typename code_vector_type::code_type > re_match_closure;
		typedef std::stack< re_match_closure > matching_stack;
		matching_stack ms;

		typedef std::vector< runtime_backref > runtime_backref_stack;
		typedef std::vector< runtime_backref_stack > runtime_backref_stack_vector;

		// can use (below): runtime_backref_stack bs(0); and the stacks will be added when necessary.
		runtime_backref_stack_vector bs(using_backrefs, runtime_backref_stack(0));

		const typename code_vector_type::code_type* code_ptr = code.code();
		const char_type* last_text = text.text();

		bool continue_matching = true;
		while ( continue_matching ) {
			bool fail = false;
			int_type ch = 0;
			int_type temporary = 0;
			// remember our last match for use if partial_match'ing.
			last_text = text.text();

			assert(code_ptr != 0);
			switch ( *code_ptr++ ) {
			case OP_NOOP:
				continue;

			case OP_BACKUP:
				text.unget(ch);
				continue;

			case OP_FORWARD:
				if ( text.next(ch) ) break;
				continue;

			case OP_END: {
					// we always put the entire matched length in backref 0, since that backref
					// isn't used.
					if ( &matches != &default_matches ) {
						matches.push_back(re_match_type(0, text.position()));
						for ( int i = 0; i < using_backrefs; i++ ) {
							matches.push_back(re_match_type(0, 0));
						}
					}
					if ( bs.size() ) {
						for ( int i = 0; i < bs.size(); i++ ) {
							runtime_backref_stack& s = bs[i];
							while ( s.size() ) {
								if ( s.back().second == -1 ) {
									s.pop_back();
									continue;
								}
								
								if ( &matches != &default_matches ) {
									if ( (s.back().second - s.back().first) > 0 ) {
										matches[i + 1].first = s.back().first;
										matches[i + 1].second = s.back().second - s.back().first;
									}
								}

								break;
							}
						}
					}
				}
				return ( text.position() ); // length of match. todo: account for +pos

			case OP_BEGIN_OF_LINE:
				if ( text.at_begin() || text[-1] == '\n' ) continue; // text[-1] always valid
				break;

			case OP_END_OF_LINE:
				if ( text.at_end() ) continue;
				break;

			case OP_ANY_CHAR:
				if ( text.next(ch) ) break;
				if ( ch == '\n' ) break;
	#if opt_greedy_line
				if ( *(code_ptr + 1) == OP_GOTO && *(code_ptr + 2) == OP_END ) {
					// eat entire line
				}
	#endif
	#if opt_greedy_peek
				if ( *(code_ptr + 1) == OP_GOTO && *(code_ptr + 2) == OP_CHAR ) {
					// search ahead for char.
				}
	#endif
				continue;
			
			case OP_BIN_CHAR:
				if ( text.next(ch) ) break;
				if ( ch == *code_ptr++ ) continue;
				break;

			case OP_NOT_BIN_CHAR:
				if ( text.next(ch) ) break;
				if ( ch != *code_ptr++ ) continue;
				break;

			case OP_CHAR:
				if ( text.next(ch) ) break;
				if ( caseless_cmps ) {
					if ( traits_type::toupper(ch) != traits_type::toupper(*code_ptr++) ) break;
				} else if ( lower_caseless_cmps ) {
					if ( ch == *code_ptr++ || ch == traits_type::toupper(*code_ptr++) ) break;
				} else {
					if ( ch != static_cast<char_type>(*code_ptr++) ) break;
				}
				continue;

			case OP_STRING: {
					size_t n = *(code_ptr++);
					if ( caseless_cmps ) {
						if ( traits_type::istrncmp(code_ptr, text.text(), n) == 0 ) {
							text.advance(n);
							code_ptr += n;
							continue;
						}
					} else {
						if ( traits_type::strncmp(code_ptr, text.text(), n) == 0 ) {
							text.advance(n);
							code_ptr += n;
							continue;
						}
					}
					code_ptr += n;
					break;
				}
			case OP_NOT_CHAR:
				if ( text.next(ch) ) break;
				if ( ch != static_cast<char_type>(*code_ptr++) ) continue;
				break;

			case OP_RANGE_CHAR:
				if ( text.next(ch) ) break;
				code_ptr += 2;
				if ( ch >= *(code_ptr - 2) && ch <= *(code_ptr - 1) ) continue;
				break;

			case OP_NOT_RANGE_CHAR:
				if ( text.next(ch) ) break;
				code_ptr += 2;
				if ( !(ch >= *(code_ptr - 2) && ch <= *(code_ptr - 1)) ) continue;
				break;

			case OP_BACKREF_BEGIN: {
					int ref = *code_ptr++;
					while ( static_cast<int>(bs.size()) < ref ) {
						bs.push_back(runtime_backref_stack());
					}
					runtime_backref_stack& s = bs[ref - 1];
					s.push_back(runtime_backref(text.position(), -1));
				}
				continue;

			case OP_BACKREF_END: {
					int ref = *code_ptr++;

					assert(ref <= bs.size());
					runtime_backref_stack& s = bs[ref - 1];
					assert(s.size() > 0);

					if ( s.back().second == -1 ) {
						s.back().second = text.position();
					} else {
						s.push_back(runtime_backref(s.back().first, text.position()));
					}
				}
				continue;

			case OP_BACKREF: {
					int ref = *code_ptr++;
					assert(ref <= bs.size());
					if ( ref > bs.size() || bs[ref - 1].size() == 0) {
						break;
					}
					runtime_backref_stack& s = bs[ref - 1];

				// scg alt: if ( text.compare(s.back().first, s.back().second, ch) == false ) {
					if ( text.has_substring(s.back().first, s.back().second, ch) == false ) {
						break;
					}
				}
				continue;

			case OP_GOTO:
				code_ptr += decode_address_and_advance(code_ptr);
				continue;

			case OP_FAKE_FAILURE_GOTO: {
					int addr0 = decode_address_and_advance(code_ptr);
					assert(*code_ptr == OP_PUSH_FAILURE);
					const typename code_vector_type::code_type* cp = code_ptr + 1;
					int addr1 = decode_address_and_advance(cp);
					if ( maximum_closure_stack < ms.size() ) return -2;
					ms.push(re_match_closure(code_ptr + addr1 + 3));
					code_ptr += addr0;
				}
				continue;

			case OP_PUSH_FAILURE2: {
					int addr = decode_address_and_advance(code_ptr);
					if ( maximum_closure_stack < ms.size() ) return -2;
					ms.push(re_match_closure(code_ptr + addr));
				}
				continue;

			case OP_PUSH_FAILURE: {
					int addr = decode_address_and_advance(code_ptr);	// offset to goto
					if ( maximum_closure_stack < ms.size() ) return -2;
					ms.push(re_match_closure(code_ptr + addr, text.text()));
				}
				continue;
				
			case OP_POP_FAILURE:
				if ( ms.top().failure() ) ms.pop();
				continue;

			case OP_POP_FAILURE_GOTO:
				if ( ms.top().failure() ) ms.pop();
				code_ptr += decode_address_and_advance(code_ptr);
				continue;

			case OP_CLOSURE: {
					int addr = decode_address_and_advance(code_ptr);	// offset to goto
					int mi = decode_address_and_advance(code_ptr);		// minimum
					int mx = decode_address_and_advance(code_ptr);		// maximum
					if ( maximum_closure_stack < ms.size() ) {
						return -2;
					}
					ms.push(re_match_closure(code_ptr + addr, text.text(), mi, mx));
				}
				continue;

			case OP_CLOSURE_INC: {
					int my_addr = code_ptr - code.code();				// index into map (unique)
					int addr = decode_address_and_advance(code_ptr);	// offset to goto
					int mi = decode_address_and_advance(code_ptr);		// minimum
					int mx = decode_address_and_advance(code_ptr);		// maximum

					// we store the number of successful matches in a "map", and then store
					// it again in the failure stack. we do this because other failure 
					// points might be pushed onto the stack before we get a chance to
					// do this increment -- and we save copies in the stack to test failures
					// later (outside this switch).

					// the map really isn't a stl::map because that class isn't thread safe,
					// this implementation uses a rb_tree with a bunch of static objects.

					int n_matches = 1;

					match_count_vector::iterator it = mv.begin();
					if ( mv.size() ) {
						while ( it != mv.end() ) {
							if ( (*it).first == my_addr ) {
								n_matches = (*it).second + 1;
								break;
							} else {
								it++;
							}
						}
					}

					if ( it == mv.end() ) {
						mv.push_back(std::pair<int, int>(my_addr, 0));
						it = mv.end() - 1;
					}
					
					re_match_closure m;
					m.matched = n_matches;
					m.minimum = mi;
					m.maximum = mx;


					if ( !m.can_continue() ) {
						(*it).second = 0;
						continue;
					}
					(*it).second = n_matches;

					m.text = text.text();
					m.code = code_ptr;
					code_ptr += addr;
					ms.push(m);
				}
				continue;

	#if !notdone
			case OP_BEGIN_OF_BUFFER:
				if ( text.buffer_begin() ) continue;
				break;
			
			case OP_END_OF_BUFFER:
				if ( text.buffer_end() ) continue;
				break;
	#endif

			case OP_BEGIN_OF_WORD:
				if ( text.at_end() ) {
					break;
				} else if ( text.at_begin() || traits_type::isalnum(static_cast<int_type>(text[-1])) == 0 )  { 
					continue;
				}
				break;

			case OP_END_OF_WORD:
				if ( text.at_begin() || traits_type::isalnum(text[-1]) == 0 ) {
					break;
				} else if ( traits_type::isalnum(*text) != 0 ) {
					break;
				} else if ( text.at_end() ) {
					continue;
				}
				continue;
	#if !notdone
			case OP_WORD_BOUNDARY:	// match at begin/end of strings (buffer) ok.
				if ( *code_ptr ) {	// complement
					if ( text.at_begin() || text.word_test() ) break;
					continue;
				} else {
					if ( text.at_begin() || text.word_test() ) continue;
					break;
				}
	#endif

			case OP_DIGIT:
				if ( text.next(ch) ) break;
				if ( (*code_ptr++ ? !traits_type::isdigit(ch) : traits_type::isdigit(ch)) ) {
					continue;
				}
				break;

			case OP_SPACE:
				if ( text.next(ch) ) break;
				if ( (*code_ptr++ ? !traits_type::isspace(ch) : traits_type::isspace(ch)) ) {
					continue;
				}
				break;

			case OP_WORD:
				if ( text.next(ch) ) break;
				if ( (*code_ptr++ ? !traits_type::isalnum(ch) : traits_type::isalnum(ch)) ) {
					continue;
				}
				break;

			default:
				return -2;	// should i throw something? because we've got illegal code.
			} // end switch

			// we should only get here when we break out of the above
			// switch, that is, a break above implys a failure.
			fail = true;
			while ( fail == true ) {
				if ( ms.empty() == false ) {
					re_match_closure m = ms.top();
					ms.pop();
					
	#if oldway
					text.text(m.text);
					if ( m.text == 0 || !m.closed() ) {
						continue;
					}
	#endif
					if ( m.text == 0 ) continue;
					text.text(m.text);
					if ( !m.closed() ) continue;
					code_ptr = m.code;
					fail = false;

					// now cleanup the backreference stack.
					if ( bs.size() ) {	// we have some backreferences
						runtime_backref_stack_vector::iterator it = bs.begin();
						int p = text.position();
						while ( it != bs.end() ) {
							while ( (*it).size() ) {
								if ( ((*it).back().first > p || (*it).back().second > p) ) {
									if ( (*it).size() == 1 ) {
										(*it).back().second = -1;
										break;
									} else {
										(*it).pop_back();
									}
								} else {
									break;
								}
							}
							it++;
						}
					}
					break;
				} else {
					continue_matching = false;	// this is basically "return -1;"
					break;
				}
			} // while ( fail ) 

		} // while ( continue_matching )

		// if we get here then the match failed, we can now check for partial matches.
		if ( partial_matches && last_text ) {
	#if oldway
			return (text.match_end(last_text) - text.start());
	#endif
			text.text(last_text);
			return (text.position());
		}
		return -1; // match not found.
	}


	/////////////////////////////////////////////////////////////////////////////
	// searching method
	//
	// this member will search the catenation (or just the str1) of two strings
	// starting at pos (within str1 or str1 + str2). range is the number
	// of characters in the strings to attempt a match (this can be negative to 
	// indicate that you'd like to search backwards). pos_stop is ending character
	// position (stop position).
	//
	// parameters:
	//  if you pass -1 for either of the sizes then i'll calculate the string length.
	//	if you pass 0 for range i'll set to len1 + len2.
	//	if you pass -1 for pos_stop i'll set to len1 + len2.
	//  if you pass non-default matches, i'll pass them on to exec_match
	//
	// returns:
	//  >=0 to indicate the starting character position for a successful search
	//   -1 to indicate an unsuccessful search
	//   -2 to indicate an error, either stack overflow or a bogus re that passed
	//		the compiler.
	//

	template <class syntaxType>
	int re_engine<syntaxType>::exec_search(ctext_type& text,
										int range, re_match_vector& matches) const {
		if ( syntax_error_state ) {
			return -3;
		}

		if ( code.offset() == 0 ) {
			return -1;
		}

		if ( range == 0 ) {
			range = text.length();
		}
#if !notdone
		int dir = (range < 0) ? -1 : 1;
#endif
		if ( range < 0 ) {
			range = -range;
		}

		// after cleaning up the parameters, check that we haven't violated any assumptions that follow.
		// assert(pos + range >= 0 && pos + range <= len1 + (len2 == -1 ? 0 : len2));

		const typename code_vector_type::code_type* code_ptr = code.code();

	#if !notdone
		for ( int pos = text.start(); range >= 0; range--, pos += dir ) {
			text.start(pos);
			const char_type* partstart = 0;

			if ( lower_caseless_cmps == false && caseless_cmps == false && *code_ptr == OP_STRING ) {
				size_t n_chars = *(code_ptr + 1);
				int n = traits_type::has_chars(text, (code_ptr + 2));
				if ( n > 0 ) {
					// n--;
					pos += (dir * n);
					range -= n;
					text.start(pos);
				} else if ( n < 0 ) {
					return -1;
				}
			}

			// assume that we've either found something in a study_map or (we don't have a study_map) we
			// just rely on match to do the work at every character position. this is terribly slow
			// without a study_map. if ret == -1 then keep searching the strings.
			int ret = exec_match(text, 0, matches);

			if ( ret >= 0 ) {
				if ( &matches != &default_matches ) {
					matches[0].first = pos;
				}

				return pos;
			} else if ( ret < -1 ) {
				return ret;
			}
		}
	#endif // notdone
		return -1;
	}

	#define DUMP_CODE 1
	#ifndef DUMP_CODE 
	template <class syntaxType> void re_engine<syntaxType>::dump_code() const {
	}
	#else
	#include <iostream>

	template <class syntaxType> void re_engine<syntaxType>::dump_code() const {
		std::ostream& out = std::cerr;
		const char_type* cp = code.code();
		int pos = 0;
		while ( (pos = (cp - code.code())) < code.offset() ) {
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
				out << "OP_WORD (" << static_cast<int>(*cp++) << ")\n";
				break;

			case OP_WORD_BOUNDARY:
				out << "OP_WORD_BOUNDARY (" << static_cast<int>(*cp++) << ")\n";
				break;
			
			case OP_END_OF_LINE:
				out << "OP_END_OF_LINE" << std::endl;
				break;

			case OP_CHAR:
				out << "OP_CHAR (" << static_cast<char>(*cp++) << ")\n";
				break;

			case OP_NOT_CHAR:
				out << "OP_NOT_CHAR (" << static_cast<char>(*cp++) << ")\n";
				break;

			case OP_ANY_CHAR:
				out << "ANY_CHAR" << std::endl;
				break;
				
			case OP_RANGE_CHAR:
				out << "OP_RANGE_CHAR (" << static_cast<char>(*cp++);
				out << ',' << static_cast<char>(*cp++) << ")\n";
				break;

			case OP_NOT_RANGE_CHAR:
				out << "OP_NOT_RANGE_CHAR (" << static_cast<char>(*cp++);
				out << ',' << static_cast<char>(*cp++) << ")\n";
				break;

			case OP_BACKREF_BEGIN:
				out << "OP_BACKREF_BEGIN (" << static_cast<int>(*cp++) << ")\n";
				break;

			case OP_BACKREF_END:
				out << "OP_BACKREF_END (" << static_cast<int>(*cp++) << ")\n";
				break;
			
			case OP_BACKREF:
				out << "OP_BACKREF (" << static_cast<int>(*cp++) << ")\n";
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
				out << "OP_CLOSURE_TEST" << std::endl;
				break;

			case OP_DIGIT:
				out << "OP_DIGIT (" << static_cast<int>(*cp++) << ")" << std::endl;
				break;

			case OP_SPACE:
				out << "OP_SPACE (" << static_cast<int>(*cp++) << ")\n";
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
	#endif	// DUMP_CODE

}