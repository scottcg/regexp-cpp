#pragma once

#include <cassert>
#include <locale>


namespace re {
	/////////////////////////////////////////////////////////////////////////
	// base for a class that is used to help process and input array of chars
	// (or wchar_t's). rather than deal with a simple c/c++ array  i'm using
	// this class so that i can catch some pointer/access errors here.
	//
	// in addition, i've included some stuff to extract some common stuff from
	// the input array, like numbers control characters. this is a template
	// class since the input can be char/wchar_t.

	template <class traitsType> class re_input_string {
	public:
		typedef traitsType				traits_type;
		typedef typename traitsType::char_type	char_type;
		typedef typename traitsType::int_type	int_type;

	public:
		re_input_string(const char_type* s, size_t l = -1) {
			m_str = s;
			if ( l == static_cast<size_t>(-1)  ) {
				l = (s) ? traits_type::length(s) : 0;
			}
			m_len = l;
			m_offset  = 0;
		}

		char_type operator [](int i) const { return m_str[i]; }
		size_t length() const						{ return m_len; }

	public:
		int get(int_type& v) {					// get next char; return -1 if eof
			if ( m_offset >= m_len ) {
				return -1;
			}
			v = m_str[m_offset++];
			return 0;
		}
		int_type peek() const {					// just look at next char
			assert(m_offset < m_len);
			return m_str[m_offset];
		}
		void unget() {							// backup one character
			assert(m_offset != 0);
			m_offset--;
		}
		void unget(int_type& v) {				// backup and set prior char to v
			m_offset--;
			v = m_str[m_offset];
		}
		void advance(int skip) {				// skip some characters (offset() += skip)
			assert(skip >= 0);
			m_offset += skip;
		}

		int get_number(int_type& ch) {			// get a "number" (integer)
			assert(traits_type::isdigit(m_str[m_offset - 1]));
			get(ch);
			if ( !traits_type::isdigit(m_str[m_offset -2]) ) {
				return -1;
			}
			return m_str[m_offset - 2] - '0';
		}

		int peek_number(int_type& n, int max_digits = 6) const { // get an integer, with a max digits
			assert(max_digits < 7);
			char_type temp_array[7]; // absolute max +null.
			int nfound = 1;
			temp_array[nfound - 1] = m_str[m_offset - 1];
			while ( (max_digits - nfound) ) {
				if ( traits_type::isdigit(m_str[m_offset - 1 + nfound]) ) {
					temp_array[nfound] = m_str[m_offset -1 + nfound];
					nfound++;
				} else {
					break;
				}
			}
			temp_array[nfound] = 0;
			int int_value = traits_type::cstr_to_decimal_int(temp_array);
			n = int_value;
			return nfound;
		}
		int offset() const		{ return m_offset; }
		bool at_begin() const	{ return (m_offset == 0) ? true : false; }
		bool at_end() const		{ return (m_offset == m_len) ? true : false; }

		int translate_ctrl_char(int_type& ch);	// this needs to go.
	private:
		int hexidecimal_to_decimal(int ch);		// convert a hex to decimal
		int get_hexidecimal_digit(int_type& h);	// these should now be functions.

	private:
		const char_type*	m_str;
		int					m_len;
		int					m_offset;

	private:
		// re_input_string(const re_input_string&);
		// re_input_string& operator = (const re_input_string&) const;
	};

	template <class traitsType>
	int re_input_string<traitsType>::translate_ctrl_char(int_type& ch) {
		switch ( ch ) {
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

	template <class traitsType>
	int re_input_string<traitsType>::get_hexidecimal_digit(int_type& h) {
		int_type c;
		int value1;
		get(c);
		value1 = hexidecimal_to_decimal(c);
		if ( value1 == -1 ) {
			return -1;
		}

		get(c);
		const int value2 = hexidecimal_to_decimal(c);
		if ( value2 == -1 ) {
			return -1;
		}
		h = (int_type)(value1 * 16 + value2);
		return 0;
	}

	template <class traitsType>
	int re_input_string<traitsType>::hexidecimal_to_decimal(int ch) {
		if ( ch >= '0' && ch <= '9' )
			return ch - '0';
		if ( ch >= 'a' && ch <= 'f')
			return ch - 'a' + 10;
		if ( ch >= 'A' && ch <= 'F' )
			return ch - 'A' + 10;
		return -1;
	}
}