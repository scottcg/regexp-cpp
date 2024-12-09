
#pragma once

#include <cassert>

namespace re {
	/////////////////////////////////////////////////////////////////////////////
	// a small template class that is used to combine two strings into a single
	// virtual string. includes just the basics which i need for my regular
	// expression engine. one can optionally include a new starting position and
	// length for the resulting virtual string.
	// objects of this class also have a cursor that can be used to walk the
	// re_ctext characters, in a bidirectional manner, and members that allow one
	// to capture the current pointer position and return to that position later.
	//
	// i don't currently have prefix ++/-- because vc5 is broken. additionally
	// some of the member functions of this class must be defined within in the
	// class because the compiler will not compile the code otherwise.

	template<class traitsType>
	class re_ctext {
	public:
		typedef traitsType traits_type;
		typedef typename traitsType::char_type char_type;
		typedef typename traitsType::int_type int_type;

		explicit re_ctext(const char_type *s1, size_t l1 = -1, const char_type *s2 = 0, size_t l2 = -1,
		                  size_t n = -1, size_t len = -1);

		char_type operator ++(int);

		char_type operator --(int);

		char_type operator [](int i) const;

		explicit operator const char_type *() const {
			if (m_text_end == m_str1_end && m_text < m_str1) return 0;
			if (m_text == m_text_end) return 0;
			return m_text;
		}

		explicit operator char_type() const {
			// vc5 must have this inline
			if (m_text_end == m_str1_end && m_text < m_str1) return 0;
			if (m_text == m_text_end) return 0;
			assert(m_text != nullptr);
			return *m_text;
		}

		// returns the current position in the text; this allows you to save the
		// current pointer position and return it later.
		const char_type *text() const {
			return m_text;
		}

		// return to position specified by p; it's smart enough to return to a
		// consistent state. the value of p must be within either of the char arrays
		// passed to the constructor.
		void text(const char_type *p) {
			assert(m_str1 != nullptr);
			if (p >= (m_str1 - 1) && p <= m_str1_end) {
				m_text = p;
				m_text_end = m_str1_end;
			} else {
				assert(m_str2 != nullptr && m_str2_end != nullptr);
				assert(p >= m_str2 && p <= m_str2_end);
				m_text = p;
				m_text_end = m_str2_end;
			}
		}

		void reset();

		int next(int_type &ch);

		void current(int &ch) const;

		void unget(int_type &ch);

		int position() const;

		int advance(size_t n);

		bool has_substring(int begin, int end, int_type &lastch) {
			const char_type *b = 0;
			const char_type *e = 0;
			while (b != e) {
				if (next(lastch)) {
					return false;
				}
				// check my location in string
				if (lastch != *b++) {
					return false;
				}
			}
			return true;
		}

		int length() const;

		bool at_begin() const;

		bool at_end() const;

	private:
		const char_type *m_str1; // where first string starts (adjusted)
		const char_type *m_str2; // where second string starts (adjusted)
		const char_type *m_text; // address in either m_str1 or m_str2
		const char_type *m_text_end; // address where m_text must stop
		const char_type *m_str1_end; // address where m_str1 stops
		const char_type *m_str2_end; // address where m_str2 stops

		size_t m_len1; // length of first string (adjusted); >= 0
		size_t m_len2; // length of second string (adjusted); >= 0
	};


	template<class traitsType>
	re_ctext<traitsType>::re_ctext(const char_type *s1, size_t l1, const char_type *s2, size_t l2,
	                               size_t n, size_t len) {
		assert(s1 != nullptr);

		if (l1 == static_cast<size_t>(-1)) {
			l1 = (s1) ? traits_type::length(s1) : 0;
		}

		if (l2 == static_cast<size_t>(-1)) {
			l2 = (s2) ? traits_type::length(s2) : 0;
		}

		// adjust starting position to n; account for possibility of two strings.
		if (n != static_cast<size_t>(-1) && n > 0) {
			if (n < l1) {
				s1 += n;
				l1 -= n;
			} else if (s2 && (n < (l1 + l2))) {
				s1 = s2 + l2 - ((l1 + l2) - n);
				l1 = (l1 + l2) - n;
				s2 = 0;
				l2 = 0;
			} else if (n == l1) {
				s1 = s2 = 0;
				l1 = l2 = 0;
			}
		}

		// now adjust the "length" of the input string(s).
		if (len != static_cast<size_t>(-1) && len > 0) {
			if (len < (l1 + l2)) {
				if (len <= l1) {
					l1 = len;
					s2 = 0;
					l2 = 0;
				} else if (s2) {
					l2 = (len - l1);
				} else {
					s1 = s2 = 0;
					l1 = l2 = 0;
				}
			} else if (len != (l1 + l2)) {
				s1 = s2 = 0;
				l1 = l2 = 0;
			}
		}

		m_text = m_str1 = s1;
		m_str2 = s2;
		m_len1 = l1;
		m_len2 = l2;
		m_text_end = m_str1_end = m_str1 + m_len1;
		m_str2_end = m_str2 + m_len2;
	}

	template<class traitsType>
	typename re_ctext<traitsType>::char_type re_ctext<traitsType>::operator ++(int) {
		if (m_text == m_text_end) {
			if (m_str2 && m_text_end != m_str2_end) {
				m_text = m_str2;
				m_text_end = m_str2_end;
			} else {
				return 0;
			}
		}
		if (m_text_end == m_str1_end) {
			if (m_text < m_str1) {
				m_text++;
				return 0;
			}
		}
		return *m_text++;
	}

	template<class traitsType>
	typename re_ctext<traitsType>::char_type re_ctext<traitsType>::operator --(int) {
		if (m_text_end == m_str1_end) {
			if (m_text < m_str1) return 0;
			return *m_text--;
		} else if (m_str2 && m_text_end == m_str2_end) {
			if (m_text < m_str2) {
				m_text = m_str1_end - 1;
				m_text_end = m_str1_end;
			}
			return *m_text--;
		}
		return 0;
	}

	template<class traitsType>
	void re_ctext<traitsType>::reset() {
		m_text = m_str1;
		m_text_end = m_str1_end;
	}

	template<class traitsType>
	void re_ctext<traitsType>::current(int &ch) const {
		ch = m_text[-1];
	}

	template<class traitsType>
	int re_ctext<traitsType>::next(int_type &ch) {
		if (m_text == m_text_end) {
			if (!m_str2 || m_text == m_str2_end) return 0; // Return 0 when end is reached
			m_text = m_str2;
			m_text_end = m_str2_end;
		}
		ch = *m_text++;
		return 1; // Return 1 when a character is successfully read
	}

	template<class traitsType>
	void re_ctext<traitsType>::unget(int_type &ch) {
		if (m_str2 && m_text_end == m_str2_end) {
			if (m_text == m_str2) {
				m_text_end = m_str1_end;
				m_text = m_str1_end - 1;
			}
		}
		ch = (m_text == m_str1) ? 0 : *--m_text;
	}

	template<class traitsType>
	int re_ctext<traitsType>::advance(size_t n) {
		while (m_text != m_str1_end && n--) {
			++m_text;
		}
		return n;
	}

	template<class traitsType>
	int re_ctext<traitsType>::length() const {
		return m_len1 + m_len2;
	}

	template<class traitsType>
	bool re_ctext<traitsType>::at_begin() const {
		return ((m_text == m_str1) ? true : false);
	}

	template<class traitsType>
	bool re_ctext<traitsType>::at_end() const {
		return (m_text == m_str1_end || m_text == m_str2_end) ? true : false;
	}

	template<class traitsType>
	int re_ctext<traitsType>::position() const {
		if (m_text_end == m_str2_end) {
			// we are on the second string
			return (m_text - m_str2) + m_len1;
		} else {
			if (m_text < m_str1) return 0;
			return m_text - m_str1;
		}
	}

	template<class traitsType>
	typename re_ctext<traitsType>::char_type re_ctext<traitsType>::operator [](int i) const {
		auto assize_t = static_cast<size_t>(i);
		assert(assize_t <= (m_len1 + m_len2)); // <= because we get to null.
		if (m_str2 && assize_t >= m_len1) {
			return ((i - m_len1) == m_len2) ? 0 : m_str2[i - m_len1];
		}
		return (assize_t == m_len1) ? 0 : m_str1[assize_t];
	}
}
