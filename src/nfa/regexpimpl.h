// $Header: /regexp/regexpimpl.h 1     2/22/97 6:03p Scg $

#ifndef INCLUDE_REGEXPP_H
#define INCLUDE_REGEXPP_H


/////////////////////////////////////////////////////////////////////////////////
// in the pair, the first is the starting position, the second is the length of
// the match. if either of the pair is -1 then the backreference is invalid.
typedef std::pair<int, int> executing_backref;

#include "ctext.h"

/////////////////////////////////////////////////////////////////////////////////
// basic_regexp_impl<strT> is hidden in here because i don't want to
// publicize any of the implementation in the the interface class (basic_regexp). 
// i really like the technique of splitting the interface/impl in c++, i this
// is (imho) the best way to do that.
//
// the member functions of basic_regexp<synT, charT, intT>::impl are like what
// one would see in the private.

enum { ANCHOR_NONE = 0, ANCHOR_LINE = 1, ANCHOR_BUFFER = 2 };

template <class strT> class basic_regexp_impl {
public:
	typedef strT								string_type;
	typedef typename strT::char_traits::char_type		char_type;
	typedef typename strT::char_traits::int_type			int_type;

	typedef typename syntax_base<strT>					syntax_type;
	typedef executing_backref					backref_type;
	typedef typename std::vector< executing_backref >	backrefs_type;
	typedef typename basic_code_vector<strT>				code_vector_type;
	typedef typename compile_state_base<strT>			compile_state_type;

public:
	short						anchor;
	int							syntax_error_state;
	bool						caseless_compares;
	bool						lower_caseless_compares;
	int							using_backrefs;
	size_t						maximum_closure_stack;
	code_vector_type			code;
	
	static const match_vector	default_matches;

public:
	basic_regexp_impl();
	basic_regexp_impl(const basic_regexp_impl& that);
	const basic_regexp_impl& operator = (const basic_regexp_impl& that);
	
public:
	int exec_compile(const syntax_type& syntax, const char_type* s, size_t slen = -1, int* err = 0);
	int exec_match(ctext<strT>& text, bool partial_matches = false, match_vector& m = (match_vector&)default_matches) const;
	int exec_optimize();
	int exec_search(ctext<strT>& text, int range = 0, match_vector& m = (match_vector&)default_matches) const;
};


template <class strT>
const match_vector basic_regexp_impl<strT>::default_matches;

template <class strT>
basic_regexp_impl<strT>::basic_regexp_impl() : code() {
	anchor = ANCHOR_NONE;
	syntax_error_state = 1;			// default, no compiled expression.
	caseless_compares = 0;
	lower_caseless_compares = 0;
	using_backrefs = 0;
	maximum_closure_stack = 4096;
}

template <class strT>
basic_regexp_impl<strT>::basic_regexp_impl(const basic_regexp_impl<strT>& that)
		: code(that.code) {
	anchor = that.anchor;
	syntax_error_state = 1;			// default, no compiled expression.
	caseless_compares = that.caseless_compares;
	lower_caseless_compares = that.lower_caseless_compares;
	using_backrefs = that.using_backrefs;
}

template <class strT>
const basic_regexp_impl<strT>&
basic_regexp_impl<strT>::operator = (const basic_regexp_impl<strT>& that) {
	if ( this != &that ) {
		code = that.code;
		anchor = that.anchor;
		syntax_error_state = that.syntax_error_state;
		caseless_compares = that.caseless_compares;
		lower_caseless_compares = that.lower_caseless_compares;
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

template <class strT>
int basic_regexp_impl<strT>::exec_compile(const syntax_type& syntax, const char_type* s,
										  size_t slen, int* err_pos) {
	anchor = 0;									// allow for repeat calls to ::exec_compile
	code.initialize();

	if ( err_pos ) {
		*err_pos = 0;							// be optimistic, set err_pos to 0.
	}

	if ( s == 0 ) {								// garbage in, nothing out.
		return 0;
	}

	typedef source_help_vector<string_type> source_vector_type;

    check_string(s, slen);						// calculate slen if user passed default
	source_vector_type source(s, slen);
	
	compile_state_type cs(syntax, code, source);// let syntax obj do the work.

	syntax_error_state = cs.syntax.compile(cs);
	if ( syntax_error_state ) {
		code.initialize();						// code isn't useful.
		if ( err_pos ) {
			*err_pos = cs.input.offset();		// where the error occurred.
		}
		return syntax_error_state;
	} else {
		assert(cs.jump_stack.size() == 0);
		using_backrefs = cs.number_of_backrefs; // remember number of backrefs.
	}
	return 0;									// no error
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

template <class strT>
int basic_regexp_impl<strT>::exec_optimize() {
	if ( syntax_error_state ) {
		return -3;
	}

	bool continue_work = true;
	code_vector_type::code_type char_count = 0;
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
	if ( continue_work == true && code[old_cursor] != OP_END ) { // just a plain string
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

template <class charT, class intT> class closure {
public:
	typedef charT		char_type;
	typedef intT		int_type;

public:
	closure(const int_type* c = 0, const char_type* t = 0, const char_type* p = 0,
			int mi = -1, int mx = -1)
		: code(c), text(t), partend(p), minimum(mi), maximum(mx), matched(0)
	{}

	inline int failure() const {
		return (maximum == -1 && minimum == -1);
	}

	inline int closed() const {
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
	
	inline int can_continue() const {
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
	inline bool operator < (const closure&) const { return false; }
	inline bool operator == (const closure&) const { return false; }

public:
	const int_type*		code;
	const char_type*	text;
	const char_type*	partend;
	int					minimum;
	int					maximum;
	int					matched;

private:
	closure(const closure&);
	closure& operator = (const closure&);
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

template <class intT>
inline int decode_address_and_advance(const intT*& cp) {
	assert(cp != 0);
	return int(short((*cp++) | (*cp++ << 8)));
}


typedef std::pair<int, int> runtime_backref;

inline std::ostream& operator << (std::ostream& out, const runtime_backref& p) {
	return out << '(' << p.first << ',' << p.second << ')';
}

template <class strT>
int basic_regexp_impl<strT>::exec_match(ctext<strT>& text, bool partial_matches,
										match_vector& matches) const {
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

	typedef closure< char_type, code_vector_type::code_type > match_closure;
	typedef std::stack< match_closure > matching_stack;
	matching_stack ms;

	typedef std::vector< runtime_backref > runtime_backref_stack;
	typedef std::vector< runtime_backref_stack > runtime_backref_stack_vector;

	// can use (below): runtime_backref_stack bs(0); and the stacks will be added when necessary.
	runtime_backref_stack_vector bs(using_backrefs, runtime_backref_stack(0));

	const code_vector_type::code_type* code_ptr = code.code();
	const charT* last_text = text.text_ptr();

	bool continue_matching = true;
	while ( continue_matching ) {
		bool fail = false;
		intT ch = 0;
		intT temporary = 0;
		// remember our last match for use if partial_match'ing.
		last_text = text.text_ptr();

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
					matches.push_back(match_type(0, text.match_end() - text.start()));
					for ( int i = 0; i < using_backrefs; i++ ) {
						matches.push_back(match_type(0, 0));
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
			return ( text.match_end() - text.start() ); // length of match.

		case OP_BEGIN_OF_LINE:
			if ( text.at_begin() || text[-1] == '\n' ) continue; // text[-1] always valid
			break;

		case OP_END_OF_LINE:
			if ( text.at_end() ) continue;
			break;

		case OP_ANY_CHAR:
			if ( text.next(ch) ) break;
			if ( ch == '\n' ) break;
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
			if ( caseless_compares ) {
				if ( to_a_upper(ch) != to_a_upper(*code_ptr++) ) break;
			} else if ( lower_caseless_compares ) {
				if ( ch == *code_ptr++ || ch == to_a_upper(*code_ptr++) ) break;
			} else {
				if ( ch != (charT)*code_ptr++ ) break;
			}
			continue;

		case OP_STRING: {
				size_t n = *(code_ptr++);
				if ( caseless_compares ) {
					if ( istring_n_compare(code_ptr, text.text_ptr(), n) == 0 ) {
						text.skip(n);
						code_ptr += n;
						continue;
					}
				} else {
					if ( string_n_compare(code_ptr, text.text_ptr(), n) == 0 ) {
						text.skip(n);
						code_ptr += n;
						continue;
					}
				}
				code_ptr += n;
				break;
			}
		case OP_NOT_CHAR:
			if ( text.next(ch) ) break;
			if ( ch != static_cast<charT>(*code_ptr++) ) continue;
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
				while ( int(bs.size()) < ref ) {
					bs.push_back(runtime_backref_stack());
				}
				runtime_backref_stack& s = bs[ref - 1];
				s.push_back(runtime_backref(text.absolute_position(), -1));
			}
			continue;

		case OP_BACKREF_END: {
				int ref = *code_ptr++;

				assert(ref <= bs.size());
				runtime_backref_stack& s = bs[ref - 1];
				assert(s.size() > 0);

				if ( s.back().second == -1 ) {
					s.back().second = text.absolute_position();
				} else {
					s.push_back(runtime_backref(s.back().first, text.absolute_position()));
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

				if ( text.compare(s.back().first, s.back().second, ch) == false ) {
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
				const code_vector_type::code_type* cp = code_ptr + 1;
				int addr1 = decode_address_and_advance(cp);
				if ( maximum_closure_stack < ms.size() ) return -2;
				ms.push(match_closure(code_ptr + addr1 + 3));
				code_ptr += addr0;
			}
			continue;

		case OP_PUSH_FAILURE2: {
				int addr = decode_address_and_advance(code_ptr);
				if ( maximum_closure_stack < ms.size() ) return -2;
				ms.push(match_closure(code_ptr + addr));
			}
			continue;

		case OP_PUSH_FAILURE: {
				int addr = decode_address_and_advance(code_ptr);	// offset to goto
				if ( maximum_closure_stack < ms.size() ) return -2;
				ms.push(match_closure(code_ptr + addr, text.text_ptr(), text.partend_ptr()));
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
				ms.push(match_closure(code_ptr + addr, text.text_ptr(), text.partend_ptr(), mi, mx));
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
				
				match_closure m;
				m.matched = n_matches;
				m.minimum = mi;
				m.maximum = mx;


				if ( !m.can_continue() ) {
					(*it).second = 0;
					continue;
				}
				(*it).second = n_matches;

				m.text = text.text_ptr();
				m.partend = text.partend_ptr();
				m.code = code_ptr;
				code_ptr += addr;
				ms.push(m);
			}
			continue;

		case OP_BEGIN_OF_BUFFER:
			if ( text.buffer_begin() ) continue;
			break;
		
		case OP_END_OF_BUFFER:
			if ( text.buffer_end() ) continue;
			break;

		case OP_BEGIN_OF_WORD:
			if ( text.end_of_strs() ) {
				break;
			} else if ( text.at_begin() || is_a_alnum(static_cast<intT>(text[-1])) == 0 )  { 
				continue;
			}
			break;

		case OP_END_OF_WORD:
			if ( text.at_begin() || is_a_alnum(text[-1]) == 0 ) {
				break;
			} else if ( is_a_alnum(*text) != 0 ) {
				break;
			} else if ( text.end_of_strs() ) {
				continue;
			}
			continue;
		
		case OP_WORD_BOUNDARY:	// match at begin/end of strings (buffer) ok.
			if ( *code_ptr ) {	// complement
				if ( text.begin_of_strs() || text.word_test() ) break;
				continue;
			} else {
				if ( text.begin_of_strs() || text.word_test() ) continue;
				break;
			}

		case OP_DIGIT:
			if ( text.next(ch) ) break;
			if ( (*code_ptr++ ? !is_a_digit(ch) : is_a_digit(ch)) ) {
				continue;
			}
			break;

		case OP_SPACE:
			if ( text.next(ch) ) break;
			if ( (*code_ptr++ ? !is_a_space(ch) : is_a_space(ch)) ) {
				continue;
			}
			break;

		case OP_WORD:
			if ( text.next(ch) ) break;
			if ( (*code_ptr++ ? !is_a_alnum(ch) : is_a_alnum(ch)) ) {
				continue;
			}
			break;

		default:
			return -1;	// should i throw something? because we've got illegal code.
		} // end switch

		// we should only get here when we break out of the above
		// switch, that is, a break above implys a failure.
		fail = true;
		while ( fail == true ) {
			if ( ms.empty() == false ) {
				match_closure m = ms.top();
				ms.pop();
				
				text.text_ptr(m.text);
				if ( m.text == 0 || !m.closed() ) {
					continue;
				}
				text.partend_ptr(m.partend);
				code_ptr = m.code;
				fail = false;

				// now cleanup the backreference stack.
				if ( bs.size() ) {	// we have some backreferences
					runtime_backref_stack_vector::iterator it = bs.begin();
					int p = text.absolute_position();
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
		return (text.match_end(last_text) - text.start());
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

template <class strT>
int basic_regexp_impl<strT>::exec_search(ctext<strT>& text, int range,
										 match_vector& matches) const {

	if ( syntax_error_state ) {
		return -3;
	}

	// sanity check the parameters, for some of the parameters, we will
	// calculate values (via inferred defaults).
	if ( code.offset() == 0 ) {
		return -1;
	}

	if ( range == 0 ) {
		range = text.length();
	}
	int dir = (range < 0) ? -1 : 1;
	if ( range < 0 ) {
		range = -range;
	}

	const charT* study_map = 0;

	if ( anchor == ANCHOR_BUFFER ) {
		if ( text.start() != ANCHOR_NONE ) {
			return -1;
		} else {
			range = 0;
		}
	}

	// after cleaning up the parameters, check that we haven't violated any assumptions that follow.
	// assert(pos + range >= 0 && pos + range <= len1 + (len2 == -1 ? 0 : len2));

	const code_vector_type::code_type* code_ptr = code.code();

	for ( int pos = text.start(); range >= 0; range--, pos += dir ) {
		text.start(pos);
		const charT* partstart = 0;
		const charT* partend = 0;

		if ( lower_caseless_compares == false && caseless_compares == false && *code_ptr == OP_STRING ) {
			size_t n_chars = *(code_ptr + 1);
			int n = has_chars(text, (code_ptr + 2));
			if ( n > 0 ) {
				// n--;
				pos += (dir * n);
				range -= n;
				text.start(pos);
			} else if ( n < 0 ) {
				return -1;
			}
		}
#if 0
		// if code[0] == OP_ANCHOR_LINE then we need to some thing interesting; since
		// we aren't going to search the entire string
		if ( anchor == ANCHOR_LINE ) {
			if ( pos > 0 && (pos <= len1 ? str1[pos - 1] : str2[pos - len1 - 1]) != '\n' ) {
				continue;
			}
		}
#endif
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
	return -1;
}

#endif