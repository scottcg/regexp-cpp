// $Header: /regexp/code.h 1     2/22/97 6:02p Scg $

#ifndef INCLUDE_CODE_H
#define INCLUDE_CODE_H

///////////////////////////////////////////////////////////////////////////////////////////////
// number of precedence levels in use, if you change this number, be sure to add 1 so that
// ::store_cclass will have the highest precedence for it's own use (it's using NUM_LEVELS - 1).
const int NUM_LEVELS = 6;

///////////////////////////////////////////////////////////////////////////////////////////////
// the precedence_vector and precedence_stack work together to provide and n precedence levels
// and (almost) unlimited amount of nesting. the nesting occurs when via the precedence stack
// (push/pop) and the precedence_vector stores the current offset of the output code via
// a store current precedence. did you get all of that?

template <class intT, int sz> class precedence_vector : public std::vector<intT> {
public:
	typedef intT int_type;
	precedence_vector(intT init = 0);
};

template <class intT, int sz> 
precedence_vector<intT, sz>::precedence_vector(intT init)
	: std::vector<intT>(sz, init) {}

typedef precedence_vector<int, NUM_LEVELS> precedence_element;
// vc5 typedef std::vector<precedence_element> precedence_element_vec;
// typedef precedence_element precedence_element_vec;

///////////////////////////////////////////////////////////////////////////////////////////////
// the actual precedence stack; adds a couple of handy members to keep track of the positions
// in the precedence_element.

class precedence_stack : public std::stack< precedence_element > {
public:
	typedef precedence_element::int_type int_type;

public:
	precedence_stack();

	// current precedence value.
	int current() const;
	void current(int l);

	// where we are int the input stream; index in array indicates precedence
	int_type start() const;
	void start(int_type offset);

private:
	int		P_current;
};

inline precedence_stack::precedence_stack()
	: std::stack< precedence_element >(), P_current(0) {
	push(precedence_element());
}

inline int precedence_stack::current() const { return P_current; }

inline void precedence_stack::current(int l) { P_current = l; }

inline precedence_stack::int_type precedence_stack::start() const
{ return (top()).at(P_current);}

inline void precedence_stack::start(int_type offset)
{ top().operator[](P_current) = offset; }

///////////////////////////////////////////////////////////////////////////////////////////////
// trivial stack used to keep track of the offsets in the code vector where jumps
// should occur when changing to lower precedence operators. after a successful 
// compilation of a regular expression the future jump stack should be empty.

typedef std::stack<int> future_jump_stack;


///////////////////////////////////////////////////////////////////////////////////////////////
// the basic_code_vector is like a smart vector, it's used to store the compiled
// regular expression. it provides some simple member functions to store operations
// (opcodes) and some member that supply a little amount of safety when using the
// class (as opposed to using a simple c/c++ array). the reason that this is a template
// is that when i first wrote this package it only dealt with char's; i figured that 
// someday i'd add wide character support and i wasn't sure what type the code vector should be.

const int INC_SIZE = 16;	// starting/increment size for the code array.

//template <class stringType> class foo
//{
//public:
//	typedef typename stringType::char_type fo;
//};

template <class strT> class basic_code_vector {
public:
	typedef typename strT::char_traits::char_type		char_type;
	typedef typename strT::char_traits::int_type			int_type;
	typedef typename strT::char_traits::char_type	code_type; // TODO is this right? should it be int_type?
	typedef typename compile_state_base<strT>			compile_state_type;
	// typedef typename compile_state_base<charT, intT>	compile_state_type;

public:
	basic_code_vector(int n = INC_SIZE);
	basic_code_vector(const basic_code_vector&);
	basic_code_vector& operator = (const basic_code_vector&);

	~basic_code_vector();

	const code_type operator [] (int i) const { return P_vector[i]; }
	code_type& operator [] (int i) {
		if ( i > P_offset ) P_offset = i;
		return P_vector[i];
	}

public:
	void initialize();		// can be called to re-initialize the object.
	void alloc(int n);		// check vector size, enlarge if necessary.

	// these are all safe to call; they will call alloc() before updating vector. each
	// with store something in the code vector and possibly update the precedence stack
	// to reflect where we are storing something.
	void store(code_type t);
	void store(code_type opcode, code_type flag);
	void store(code_type opcode, precedence_stack&);
	void store(code_type opcode, code_type ch, precedence_stack&);
	void store_jump(int pos, int type, int addr);

public:
	int store_alternate(compile_state_type&);		// used for "A|B"
	int store_concatenate(compile_state_type&);		// used for "[^AB]"
	int store_class_alternate(compile_state_type&);	// used for "[AB"
	int store_class(compile_state_type&);			// used for "[A]" and "[^A]"
	int store_closure(compile_state_type&);			// used for "{n,m}"
	void store_closure_count(compile_state_type&, int pos, int addr, int minimum, int maximum);
	
	int offset() const;
	const code_type* code() const {					// pointer to code; should get rid of this.
		return P_vector;
	}

	void put_address(int off, int addr);			// store a jump to address.

private:
	void put_number(int pos, int n);

	code_type*		P_vector;
	int				P_size;
	int				P_offset;
	int				P_cclass_org;
};


template <class strT>
basic_code_vector<strT>::basic_code_vector(int n) : P_size(n) {
	P_offset = 0;
	P_vector = 0;
	if ( P_size > 0 ) {
		P_vector = new code_type [ P_size ];
		initialize();
	}
}

template <class strT>
basic_code_vector<strT>::basic_code_vector(const basic_code_vector& that) {
	P_offset = 0;
	P_vector = 0;
	P_size = 0;
	operator = (that);
}

template <class strT>
basic_code_vector<strT>&
basic_code_vector<strT>::operator = (const basic_code_vector& that) {
	if ( this != &that ) {
		P_offset = that.P_offset;
		delete [] P_vector;
		P_vector = 0;
		if ( (P_size = that.P_size) > 0 ) {
			P_vector = new code_type [ P_size ];
			memcpy(P_vector, that.P_vector, sizeof(code_type) * P_size);
		}
	}
	return *this;
}

template <class strT>
basic_code_vector<strT>::~basic_code_vector() {
	delete [] P_vector;
}

//template <class strT>
//const basic_code_vector<strT>::code_type basic_code_vector<strT>::operator [] (int i) const {
//	return P_vector[i];
//}

//template <class strT>
//basic_code_vector<strT>::code_type& basic_code_vector<strT>::operator [] (int i) {
//	if ( i > P_offset ) P_offset = i;
//	return P_vector[i];
//}

template <class strT>
void basic_code_vector<strT>::initialize() {
	P_offset = 0;
	if ( P_vector ) {
		memset(P_vector, 0, sizeof(code_type) * P_size);
	}
}

template <class strT>
void basic_code_vector<strT>::alloc(int n) {
	if ( (P_offset + n) > P_size ) {
		n = INC_SIZE + (P_offset + n);
		code_type* p = new code_type [n];
		if ( P_vector ) {
			memcpy(p, P_vector, sizeof(code_type) * P_size);
			delete [] P_vector;
		}
		P_size = n;
		P_vector = p;
	}
}

template <class strT>
void basic_code_vector<strT>::store(code_type t) {
	alloc(1);
	assert(P_offset < P_size);
	P_vector[P_offset++] = t;
}

template <class strT>
void basic_code_vector<strT>::store(code_type op, code_type flag) {
	alloc(2);
	P_vector[P_offset++] = op;
	P_vector[P_offset++] = flag;
}

template <class strT>
void basic_code_vector<strT>::store(code_type opcode, precedence_stack& prec_stack) {
	prec_stack.start(P_offset);
	alloc(1);
	store(opcode);
}

template <class strT>
void basic_code_vector<strT>::store(code_type opcode, code_type ch, precedence_stack& prec_stack) {
	prec_stack.start(P_offset);
	alloc(2);
	store(opcode);
	store(ch);
}

template <class strT>
void basic_code_vector<strT>::put_number(int pos, int n) {
	alloc(2);	// just in case
	P_vector[pos++] = (code_type)(n & 0xFF);
	P_vector[pos] = (code_type)((n >> 8) & 0xFF);
}

template <class strT>
void basic_code_vector<strT>::put_address(int off, int addr) {
	alloc(2); // just in case
	int dsp = addr - off - 2;
	P_vector[off] = (code_type)(dsp & 0xFF);
	P_vector[off + 1] = (code_type)((dsp >> 8) & 0xFF);
}

template <class strT>
void basic_code_vector<strT>::store_jump(int opcode_pos, int type, int to_addr) {
	alloc(3);
	// move stuff down the stream, we are going to insert at pos
	for ( int a = P_offset - 1; a >= opcode_pos; a-- ) {
		P_vector[a + 3] = P_vector[a];
	}
	P_vector[opcode_pos] = (code_type)(type);
	put_address(opcode_pos + 1, to_addr);
	P_offset += 3;
}

template <class strT>
int basic_code_vector<strT>::offset() const {
	return P_offset;
}

//template <class strT>
//const basic_code_vector<strT>::code_type* basic_code_vector<strT>::code() const {
//	return P_vector;
//}

template <class strT>
int basic_code_vector<strT>::store_alternate(compile_state_type& cs) {
	store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);
	store(OP_GOTO);
	cs.jump_stack.push(offset());
	store(0);
	store(0);
	cs.prec_stack.start(offset());

	return 0;
}

template <class strT>
int basic_code_vector<strT>::store_class_alternate(compile_state_type& cs) {
	store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);

	// similar to above.
	store(OP_POP_FAILURE_GOTO);
	cs.jump_stack.push(offset());
	store(0);
	store(0);
	cs.prec_stack.start(offset());
	return 0;
}

template <class strT>
int basic_code_vector<strT>::store_concatenate(compile_state_type& cs) {
	// store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, P_offset + 6);
	store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, P_offset + 4);
	store(OP_FORWARD);
	cs.prec_stack.start(offset());
	store(OP_POP_FAILURE);
	return 0;
}

template <class strT>
int basic_code_vector<strT>::store_class(compile_state_type& cs) {
	// do all kinds of complicated stuff to initialize the precedence stack and to set
	// the [] to the highest precedence we can get (so that any future jumps we insert
	// when processing the character class will be completed when we finish).
	int start_offset = P_offset;
	cs.prec_stack.start(P_offset);
	cs.prec_stack.current(NUM_LEVELS - 1);
	cs.prec_stack.start(P_offset);

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


template <class strT>
void basic_code_vector<strT>::store_closure_count(compile_state_type& cs,
														 int pos, int addr,
														 int mi, int mx) {
	const int skip = 7;
	alloc(skip);
	for ( int a = P_offset - 1; a >= pos; a-- ) {
		P_vector[a + skip] = P_vector[a];
	}
	P_vector[pos++] = (code_type)(OP_CLOSURE);
	put_address(pos, addr);
	pos += 2;
	put_number(pos, mi);
	pos += 2;
	put_number(pos, mx);
	P_offset += skip; // 4 + 3

	store_jump(P_offset, OP_CLOSURE_INC, cs.prec_stack.start() + 3);
	put_number(P_offset, mi);
	P_offset += 2;
	put_number(P_offset, mx);
	P_offset += 2;

	cs.prec_stack.start(P_offset);
}


template <class strT>
int basic_code_vector<strT>::store_closure(compile_state_type& cs) {
	typedef source_help_vector<charT, intT> source_vector_type;
	source_vector_type::int_type ch = 0;
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

	store_closure_count(cs, cs.prec_stack.start(), P_offset + 10, minimum, maximum);

	return 0;
}

#endif