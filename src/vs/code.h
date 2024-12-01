
#pragma once

#include "tokens.h"
#include "precedence.h"

namespace re {
	///////////////////////////////////////////////////////////////////////////////////////////////
	// the re_code_vec is like a smart vector, it's used to store the compiled
	// regular expression. it provides some simple member functions to store operations
	// (opcodes) and some member that supply a little amount of safety when using the
	// class (as opposed to using a simple c/c++ array). the reason that this is a template
	// is that when i first wrote this package it only dealt with char's; i figured that 
	// someday i'd add wide character support and i wasn't sure what type the code vector should be.

	constexpr int INC_SIZE = 16;	// starting/increment size for the code array.

	template <class traitsT> class re_code_vec {
	public:
		typedef traitsT						traits_type;
		typedef typename traits_type::char_type		char_type;
		typedef typename traits_type::int_type		int_type;
		typedef typename traits_type::char_type		code_type;
		typedef re_compile_state<traitsT> compile_state_type;

	public:
		re_code_vec(int n = INC_SIZE)  : m_size(n) {
			m_offset = 0;
			m_vector = 0;
			if ( m_size > 0 ) {
				m_vector = new code_type [ m_size ];
				initialize();
			}
		}

		re_code_vec(const re_code_vec& rhs) {
			m_offset = 0;
			m_vector = 0;
			m_size = 0;
			operator = (rhs);
		}

		re_code_vec& operator = (const re_code_vec& rhs) {
			if ( this != &rhs ) {
				m_offset = rhs.m_offset;
				delete [] m_vector;
				m_vector = 0;
				if ( (m_size = rhs.m_size) > 0 ) {
					m_vector = new code_type [ m_size ];
					memcpy(m_vector, rhs.m_vector, sizeof(code_type) * m_size);
				}
			}
			return *this;
		}

		~re_code_vec() {
			delete [] m_vector;
		}


		const code_type operator [] (int i) const {
			return m_vector[i];
		}

		code_type& operator [] (int i) {
			if ( i > m_offset ) m_offset = i;
			return m_vector[i];
		}

	public:
		void initialize() {	// can be called to re-initialize the object.
			m_offset = 0;
			if ( m_vector ) {
				memset(m_vector, 0, sizeof(code_type) * m_size);
			}
		}		
		void alloc(int n) { 	// check vector size, enlarge if necessary.
			if ( (m_offset + n) > m_size ) {
				n = INC_SIZE + (m_offset + n);
				code_type* p = new code_type [n];
				if ( m_vector ) {
					memcpy(p, m_vector, sizeof(code_type) * m_size);
					delete [] m_vector;
				}
				m_size = n;
				m_vector = p;
			}
		}

	public:
		void store(code_type t) {
			alloc(1);
			assert(m_offset < m_size);
			m_vector[m_offset++] = t;
		}

		void  store(code_type op, code_type flag) {
			alloc(2);
			m_vector[m_offset++] = op;
			m_vector[m_offset++] = flag;
		}

		void store(code_type opcode, re_precedence_stack& prec_stack) {
			prec_stack.start(m_offset);
			alloc(1);
			store(opcode);
		}

		void store(code_type opcode, code_type ch, re_precedence_stack& prec_stack) {
			prec_stack.start(m_offset);
			alloc(2);
			store(opcode);
			store(ch);
		}

		void put_address(int off, int addr) {
			alloc(2); // just in case
			int dsp = addr - off - 2;
			m_vector[off] = (code_type)(dsp & 0xFF);
			m_vector[off + 1] = (code_type)((dsp >> 8) & 0xFF);
		}

		void store_jump(int opcode_pos, int type, int to_addr) {
			alloc(3);
			// move stuff down the stream, we are going to insert at pos
			for ( int a = m_offset - 1; a >= opcode_pos; a-- ) {
				m_vector[a + 3] = m_vector[a];
			}
			m_vector[opcode_pos] = (code_type)(type);
			put_address(opcode_pos + 1, to_addr);
			m_offset += 3;
		}

		int offset() const {
			return m_offset;
		}

		const code_type* code() const {
			return m_vector;
		}

		int store_alternate(compile_state_type& cs) {			// used for "A|B"
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);
			store(OP_GOTO);
			cs.jump_stack.push(offset());
			store(0);
			store(0);
			cs.prec_stack.start(offset());

			return 0;
		}

		int store_class_alternate(compile_state_type& cs) {		// used for "[AB"
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);

			// similar to above.
			store(OP_POP_FAILURE_GOTO);
			cs.jump_stack.push(offset());
			store(0);
			store(0);
			cs.prec_stack.start(offset());
			return 0;
		}

		int store_concatenate(compile_state_type& cs) {			// used for "[^AB]"
			// store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, m_offset + 6);
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, m_offset + 4);
			store(OP_FORWARD);
			cs.prec_stack.start(offset());
			store(OP_POP_FAILURE);
			return 0;
		}

		int store_class(compile_state_type& cs) {				// used for "[A]" and "[^A]"
			// do all kinds of complicated stuff to initialize the precedence stack and to set
			// the [] to the highest precedence we can get (so that any future jumps we insert
			// when processing the character class will be completed when we finish).
			int start_offset = m_offset;
			cs.prec_stack.start(m_offset);
			cs.prec_stack.current(NUM_LEVELS - 1);
			cs.prec_stack.start(m_offset);

			if ( cs.input.get(cs.ch) ) {			// consume the '['.
				return -1;
			}

			cs.cclass_complement = false;			// assume we aren't going to see a '^'
			if ( cs.ch == '^' ) {
				cs.cclass_complement = true;
				if ( cs.input.get(cs.ch) ) {		// consume the '^'
					return -1;
				}
			}

			bool first_time_thru = true;
			do {
				if ( first_time_thru == false && (cs.cclass_complement == false) ) {
					store_class_alternate(cs);
				} else {
					first_time_thru = false;
				}

				if ( cs.ch == '\\' ) {
					cs.input.get(cs.ch);
					if ( cs.syntax.translate_cclass_escaped_op(cs) ) {
						return SYNTAX_ERROR;
					}
				} else if ( cs.ch == '-' && cs.input.peek() != ']' ) {
					store((cs.cclass_complement == false) ? OP_CHAR : OP_NOT_CHAR, '-');
				} else {
					if ( cs.input.peek() == '-' ) {
						code_type first_ch = cs.ch;
						if ( cs.input.get(cs.ch) ) {
							return -1;
						}
						if ( cs.input.peek() == ']' ) {
							cs.input.unget(cs.ch); // put the '-' back
							store(((cs.cclass_complement == false) ? OP_CHAR : OP_NOT_CHAR), first_ch);
						} else {
							if ( cs.input.get(cs.ch) ) {
								return -1;
							}
							store((cs.cclass_complement == false) ? OP_RANGE_CHAR : OP_NOT_RANGE_CHAR);
							store(first_ch);
							store(cs.ch);
						}
					} else {
						store(((cs.cclass_complement == false) ? OP_CHAR : OP_NOT_CHAR), cs.ch);
					}
				}
				
				if ( cs.cclass_complement == true ) {
					store(OP_BACKUP);
				}

				if ( cs.input.get(cs.ch) ) {
					return -1;
				}
			} while ( cs.ch != ']' );

			if ( cs.cclass_complement == true ) {
				store_concatenate(cs);
			}

			// let's put start back to the start of cclass, possibly different because of the
			// insertion of a jumps. since we (cclass) are operating at the highest precedence
			// all of the future jumps will be popped after we return.
			cs.prec_stack.start(start_offset);
			return 0;
		}


		void store_closure_count(compile_state_type& cs, int pos, int addr, int mi, int mx) {
			constexpr int skip = 7;
			alloc(skip);
			for ( int a = m_offset - 1; a >= pos; a-- ) {
				m_vector[a + skip] = m_vector[a];
			}
			m_vector[pos++] = (code_type)(OP_CLOSURE);
			put_address(pos, addr);
			pos += 2;
			put_number(pos, mi);
			pos += 2;
			put_number(pos, mx);
			m_offset += skip; // 4 + 3

			store_jump(m_offset, OP_CLOSURE_INC, cs.prec_stack.start() + 3);
			put_number(m_offset, mi);
			m_offset += 2;
			put_number(m_offset, mx);
			m_offset += 2;

			cs.prec_stack.start(m_offset);
		}


		int store_closure(compile_state_type& cs) {						// used for "{n,m}"
			int_type ch = 0;
			if ( cs.input.get(ch) ) {
				return -1;
			}

			int minimum = -1, maximum = -1;
			if ( ch == ',' ) {				// {,max} no more than max {0,max}
				if ( cs.input.get(ch) ) {	// skip the ','
					return -1;
				}
				minimum = 0;
				maximum = cs.input.get_number(ch);
			} else {
				minimum = cs.input.get_number(ch);
				maximum = 0;
				if ( ch == ',' ) {			// {min,} at least min times {min,0}
					if ( cs.input.get(ch) ) {
						return -1;
					}
					if ( ch != '}' ) {		// {min,max} at least min no more than max
						maximum = cs.input.get_number(ch);
					}
				} else {					// {min} at exactly min (and max)
					maximum = minimum;
				}
			}
			if ( !(minimum >= 0 && maximum >= 0) ) {
				return -1;
			}

			if ( ch != '}' ) {
				return -1;
			}

			store_closure_count(cs, cs.prec_stack.start(), m_offset + 10, minimum, maximum);

			return 0;
		}

		
	private:
		void put_number(int pos, int n) {
			alloc(2);	// just in case
			m_vector[pos++] = (code_type)(n & 0xFF);
			m_vector[pos] = (code_type)((n >> 8) & 0xFF);
		}

		code_type*		m_vector;
		int				m_size;
		int				m_offset;
		int				m_cclass_org;
	};
}