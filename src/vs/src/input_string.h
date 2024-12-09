#pragma once

#include <cassert>
#include <locale>

namespace re {
	template<class traitsType>
	class input_string {
	public:
		typedef traitsType traits_type;
		typedef typename traitsType::char_type char_type;
		typedef typename traitsType::int_type int_type;

		explicit input_string(const char_type *s, size_t l = -1) {
			_str = s;
			if (l == static_cast<size_t>(-1)) {
				l = (s) ? traits_type::length(s) : 0;
			}
			_len = l;
			_offset = 0;
		}

		char_type operator [](int i) const { return _str[i]; }
		int_type length() const { return _len; }

		int get(int_type &v) {
			// get next char; return -1 if eof
			if (_offset >= _len) {
				return -1;
			}
			v = _str[_offset++];
			return 0;
		}

		int_type peek() const {
			// just look at next char
			assert(_offset < _len);
			return _str[_offset];
		}

		void unget() {
			// backup one character
			assert(_offset != 0);
			_offset--;
		}

		void unget(int_type &v) {
			// backup and set prior char to v
			_offset--;
			v = _str[_offset];
		}

		void advance(const int skip) {
			// skip some characters (offset() += skip)
			assert(skip >= 0);
			_offset += skip;
		}

		int get_number(int_type &ch) {
			// get a "number" (integer)
			assert(traits_type::isdigit(_str[_offset - 1]));
			get(ch);
			if (!traits_type::isdigit(_str[_offset - 2])) {
				return -1;
			}
			return _str[_offset - 2] - '0';
		}

		int_type peek_number(int_type &n, size_t max_digits = 6) const {
			// get an integer, with a max digits
			assert(max_digits < 7);
			std::string temp_string;
			while (temp_string.size() < static_cast<size_t>(max_digits) && traits_type::isdigit(
				       _str[_offset + temp_string.size()])) {
				temp_string += _str[_offset + temp_string.size()];
			}
			n = traits_type::cstr_to_decimal_int(temp_string.c_str());
			return temp_string.size();
		}

		int offset() const { return _offset; }
		bool at_begin() const { return _offset == 0; }
		bool at_end() const { return _offset == _len; }

		int translate_ctrl_char(int_type &ch); // this needs to go.
	private:
		int get_hexadecimal_digit(int_type &h); // these should now be functions.

		const char_type *_str;
		int_type _len;
		int _offset;
	};

	template<class traitsType>
	int input_string<traitsType>::translate_ctrl_char(int_type &ch) {
		switch (ch) {
			case 'a':
			case 'A': // ^G bell
				ch = 7;
				break;

			case 'b':
			case 'B':
				ch = '\b';
				break;

			case 'c':
			case 'C': // control character \cL == ^L
				get(ch);
				if (ch < '@' || ch > '_') {
					return -1;
				}
				ch = toupper(ch) - '@';
				break;

			case 'f':
			case 'F':
				ch = '\f';
				break;

			case 'n':
			case 'N':
				ch = '\n';
				break;

			case 'r':
			case 'R':
				ch = '\r';
				break;

			case 't':
			case 'T':
				ch = '\t';
				break;

			case 'v':
			case 'V':
				ch = '\v';
				break;

			case 'x':
			case 'X':
				return get_hexadecimal_digit(ch); // hex code
				break;

			case '0': // \000 octal or a register.
				ch = 0;
				break;

			default:
				break;
		}
		return 0;
	}

	template<class traitsType>
	int input_string<traitsType>::get_hexadecimal_digit(int_type &h) {
		int_type c;
		get(c);
		const int value1 = traits_type::hexadecimal_to_decimal(c);
		if (value1 == -1) {
			return -1;
		}

		get(c);
		const int value2 = traits_type::hexadecimal_to_decimal(c);
		if (value2 == -1) {
			return -1;
		}
		h = static_cast<int_type>(value1 * 16 + value2);
		return 0;
	}
}
