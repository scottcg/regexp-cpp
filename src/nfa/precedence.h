#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////
// number of precedence levels in use, if you change this number, be sure to add 1 so that
// ::store_cclass will have the highest precedence for it's own use (it's using NUM_LEVELS - 1).
const int NUM_LEVELS = 6;

///////////////////////////////////////////////////////////////////////////////////////////////
// the precedence_vector and precedence_stack work together to provide and n precedence levels
// and (almost) unlimited amount of nesting. the nesting occurs when via the precedence stack
// (push/pop) and the precedence_vector stores the current offset of the output code via
// a store current precedence. did you get all of that?

template<class intT, int sz> class precedence_vector : public std::vector<intT> {
public:
	typedef intT int_type;
	precedence_vector(intT init = 0);
};

template<class intT, int sz>
precedence_vector<intT, sz>::precedence_vector(intT init)
	: std::vector<intT>(sz, init) {}

typedef precedence_vector<int, NUM_LEVELS> precedence_element;

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
typedef precedence_stack precedence_stack;