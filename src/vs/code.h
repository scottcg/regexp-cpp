
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

	template <class traitsT> class compiled_code_vector {
	public:
		typedef traitsT						traits_type;
		typedef typename traits_type::char_type		char_type;
		typedef typename traits_type::int_type		int_type;
		typedef typename traits_type::char_type		code_type;
		typedef re_compile_state<traitsT> compile_state_type;

	public:
		compiled_code_vector(int n = INC_SIZE)  : _size(n) {
			_offset = 0;
			_code_vector = 0;
			if ( _size > 0 ) {
				_code_vector = new code_type [ _size ];
				initialize();
			}
		}

		compiled_code_vector(const compiled_code_vector& rhs) {
			_offset = 0;
			_code_vector = 0;
			_size = 0;
			operator = (rhs);
		}

		compiled_code_vector& operator = (const compiled_code_vector& rhs) {
			if ( this != &rhs ) {
				_offset = rhs._offset;
				delete [] _code_vector;
				_code_vector = 0;
				if ( (_size = rhs._size) > 0 ) {
					_code_vector = new code_type [ _size ];
					memcpy(_code_vector, rhs._code_vector, sizeof(code_type) * _size);
				}
			}
			return *this;
		}

		~compiled_code_vector() {
			delete [] _code_vector;
		}


		const code_type operator [] (int i) const {
			return _code_vector[i];
		}

		code_type& operator [] (int i) {
			if ( i > _offset ) _offset = i;
			return _code_vector[i];
		}

	public:
		void initialize() {	// can be called to re-initialize the object.
			_offset = 0;
			if ( _code_vector ) {
				memset(_code_vector, 0, sizeof(code_type) * _size);
			}
		}		
		void alloc(int n) { 	// check vector size, enlarge if necessary.
			if ( (_offset + n) > _size ) {
				n = INC_SIZE + (_offset + n);
				code_type* p = new code_type [n];
				if ( _code_vector ) {
					memcpy(p, _code_vector, sizeof(code_type) * _size);
					delete [] _code_vector;
				}
				_size = n;
				_code_vector = p;
			}
		}

	public:
		void store(code_type t) {
			alloc(1);
			assert(_offset < _size);
			_code_vector[_offset++] = t;
		}

		void  store(code_type op, code_type flag) {
			alloc(2);
			_code_vector[_offset++] = op;
			_code_vector[_offset++] = flag;
		}

		void store(code_type opcode, re_precedence_stack& prec_stack) {
			prec_stack.start(_offset);
			alloc(1);
			store(opcode);
		}

		void store(code_type opcode, code_type ch, re_precedence_stack& prec_stack) {
			prec_stack.start(_offset);
			alloc(2);
			store(opcode);
			store(ch);
		}

		void put_address(int off, int addr) {
			alloc(2); // just in case
			int dsp = addr - off - 2;
			_code_vector[off] = static_cast<code_type>(dsp & 0xFF);
			_code_vector[off + 1] = static_cast<code_type>((dsp >> 8) & 0xFF);
		}

		void store_jump(int opcode_pos, int type, int to_addr) {
			alloc(3);
			// move stuff down the stream, we are going to insert at pos
			for ( int a = _offset - 1; a >= opcode_pos; a-- ) {
				_code_vector[a + 3] = _code_vector[a];
			}
			_code_vector[opcode_pos] = static_cast<code_type>(type);
			put_address(opcode_pos + 1, to_addr);
			_offset += 3;
		}

		int offset() const {
			return _offset;
		}

		const code_type* code() const {
			return _code_vector;
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
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, _offset + 4);
			store(OP_FORWARD);
			cs.prec_stack.start(offset());
			store(OP_POP_FAILURE);
			return 0;
		}

		int store_class(compile_state_type& cs) {				// used for "[A]" and "[^A]"
			// do all kinds of complicated stuff to initialize the precedence stack and to set
			// the [] to the highest precedence we can get (so that any future jumps we insert
			// when processing the character class will be completed when we finish).
			int start_offset = _offset;
			cs.prec_stack.start(_offset);
			cs.prec_stack.current(NUM_LEVELS - 1);
			cs.prec_stack.start(_offset);

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
			for ( int a = _offset - 1; a >= pos; a-- ) {
				_code_vector[a + skip] = _code_vector[a];
			}
			_code_vector[pos++] = static_cast<code_type>(OP_CLOSURE);
			put_address(pos, addr);
			pos += 2;
			put_number(pos, mi);
			pos += 2;
			put_number(pos, mx);
			_offset += skip; // 4 + 3

			store_jump(_offset, OP_CLOSURE_INC, cs.prec_stack.start() + 3);
			put_number(_offset, mi);
			_offset += 2;
			put_number(_offset, mx);
			_offset += 2;

			cs.prec_stack.start(_offset);
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

			store_closure_count(cs, cs.prec_stack.start(), _offset + 10, minimum, maximum);

			return 0;
		}

		
	private:
		void put_number(int pos, int n) {
			alloc(2);	// just in case
			_code_vector[pos++] = static_cast<code_type>(n & 0xFF);
			_code_vector[pos] = static_cast<code_type>((n >> 8) & 0xFF);
		}

		code_type*		_code_vector;
		int				_size;
		int				_offset;
		int				_character_class_org;
	};
}