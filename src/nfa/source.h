
#ifndef INCLUDE_SOURCE_HELP_H
#define INCLUDE_SOURCE_HELP_H

///////////////////////////////////////////////////////////////////////////////////////////////
// base for a class that is used to help process and input array of chars (or wchar_t's).
// rather than deal with a simple c/c++ array  i'm using this class so that i can catch some
// pointer/access errors here.
//
// in addition, i've included some stuff to extract some common stuff from the input array,
// like numbers control characters. this is a template class since the input can be char/wchar_t.
//

template <class strT> class source_help_vector {
public:
	typedef strT								string_type;
	typedef typename strT::char_traits::char_type		char_type;
	typedef typename strT::char_traits::int_type			int_type;

public:
	source_help_vector(const char_type* a, size_t l);

	const char_type operator [] (int i) const {
		return m_vector[i];
	}

public:
	int get(int_type& v);					// get next char; return -1 if eof
	int_type peek() const {					// just look at next char
		assert(m_offset < m_length);
		return m_vector[m_offset];
	}
	void unget();							// backup one character
	void unget(int_type& v);				// backup and set prior char to v
	void advance(int skip);					// skip some characters (offset() += skip)

	int get_number(int_type& ch);			// get a "number" (integer)
	int peek_number(int_type& n, int max_dig = 6) const; // get an integer, with a max digits
	int offset() const;						// where we are in input char array
	bool at_end() const;					// offset() >= length of char array

	int translate_ctrl_char(int_type& ch);	// this needs to go.

private:
	int hexidecimal_to_decimal(int ch);		// convert a hex to decimal
	int get_hexidecimal_digit(int_type& h);	// these should now be functions.
	
	const char_type*	m_vector;
	int					m_length;
	int					m_offset;

private:
	source_help_vector(const source_help_vector&);
	source_help_vector& operator = (const source_help_vector&) const;
};


template <class strT>
source_help_vector<strT>::source_help_vector(const char_type* a, size_t l) {
		m_vector = a;
		m_length = l;
		m_offset  = 0;
}

//template <class strT>
//const source_help_vector<strT>::char_type source_help_vector<strT>::operator [] (int i) const {
//	return m_vector[i];
//}

template <class strT>
int source_help_vector<strT>::get(int_type& v) {
	if ( m_offset >= m_length ) { 
		return -1;
	}
	v = m_vector[m_offset++];
	return 0;
}

//template <class strT>
//source_help_vector<strT>::int_type source_help_vector<strT>::peek() const {
//	assert(m_offset < m_length);
//	return m_vector[m_offset];
//}

template <class strT>
void source_help_vector<strT>::unget() {
	assert(m_offset != 0);
	m_offset--;
}

template <class strT>
void source_help_vector<strT>::unget(int_type& v) {
	m_offset--;
	v = m_vector[m_offset];
}

template <class strT>
void source_help_vector<strT>::advance(int skip) {
	assert(skip >= 0);
	m_offset += skip;
}

template <class strT>
int source_help_vector<strT>::offset() const {
	return m_offset;
}

template <class strT>
bool source_help_vector<strT>::at_end() const {
	return (m_offset >= m_length) ? true : false;
}

// called with m_vector[-1] pointing to a number, and will consume upto max_digits more
// characters leaving m_vector[m_offset - 1] pointing to what's next. for now max_digits can 
template <class strT>
int source_help_vector<strT>::get_number(int_type& ch) {
	assert(m_vector[m_offset - 1] >= '0' && m_vector[m_offset - 1] <= '9');
	get(ch);
	if ( m_vector[m_offset - 2] < '0' || m_vector[m_offset - 2] > '9' ) {
		return -1;
	}
	return m_vector[m_offset - 2] - '0';
}

template <class strT>
int source_help_vector<strT>::peek_number(int_type& value, int max_digits) const {
	assert(max_digits < 7);
	char_type temp_array[7]; // absolute max +null.
	int nfound = 1;
	temp_array[nfound - 1] = m_vector[m_offset - 1];
	while ( (max_digits - nfound) ) {
		if ( isdigit(m_vector[m_offset - 1 + nfound]) ) {
			temp_array[nfound] = m_vector[m_offset -1 + nfound];
			nfound++;
		} else {
			break;
		}
	}
	temp_array[nfound] = 0;
	int int_value = cstr_to_decimal_int(temp_array);
	value = int_value;
	return nfound;
}


template <class strT>
int source_help_vector<strT>::translate_ctrl_char(int_type& ch) {
	switch (ch) {
	case 'a': case 'A':		// ^G bell
		ch = 7;
		break;

	case 'b': case 'B':
		ch = '\b';
		break;

	case 'c': case 'C':		// control character \cL == ^L
		get(ch);
		if ( ch < '@' || ch > '_' ) {
			return -1;
		}
		ch = toupper(ch) - '@';
		break;

	case 'f': case 'F':
		ch = '\f';
		break;

	case 'n': case 'N':
		ch = '\n';
		break;

	case 'r': case 'R':
		ch = '\r';
		break;

	case 't': case 'T':
		ch = '\t';
		break;

	case 'v': case 'V':
		ch = '\v';
		break;

	case 'x': case 'X':
		return get_hexidecimal_digit(ch);// hex code
		break;

	case '0':		// \000 octal or a register.
		ch = 0;
		break;

	default:
		break;
	}
	return 0;
}

template <class strT>
int source_help_vector<strT>::get_hexidecimal_digit(int_type& h) {
	int_type c;
	int value1;
	get(c);
	value1 = hexidecimal_to_decimal(c);
	if ( value1 == -1 ) {
		return -1;
	}

	get(c);
	int value2 = hexidecimal_to_decimal(c);
	if ( value2 == -1 ) {
		return -1;
	}
	h = (int_type)(value1 * 16 + value2);
	return 0;
}

template <class strT>
int source_help_vector<strT>::hexidecimal_to_decimal(int ch) {
	if ( ch >= '0' && ch <= '9' )
		return ch - '0';
	if ( ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if ( ch >= 'A' && ch <= 'F' )
		return ch - 'A' + 10;
	return -1;
}

// define some handy typedef's so that we don't have to deal with
// gigantic template<...> style definitions.


#ifdef _UNICODE
typedef source_help_vector<std::wstring> wchar_source_vector;
#else
typedef source_help_vector<std::string> char_source_vector;
#endif

#endif