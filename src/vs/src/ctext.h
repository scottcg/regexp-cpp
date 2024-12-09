#pragma once

#include <cassert>

namespace re {
	template<class traitsType>
	class ctext {
	public:
		typedef traitsType traits_type;
		typedef typename traitsType::char_type char_type;
		typedef typename traitsType::int_type int_type;

		explicit ctext(const char_type *s1, size_t l1 = -1, size_t n = -1, size_t len = -1);

		char_type operator ++(int);

		char_type operator --(int);

		char_type operator [](int i) const;

		explicit operator const char_type *() const {
			if (m_text == m_text_end) return 0;
			return m_text;
		}

		explicit operator char_type() const {
			if (m_text == m_text_end) return 0;
			assert(m_text != nullptr);
			return *m_text;
		}

		const char_type *text() const {
			return m_text;
		}

		void text(const char_type *p) {
			assert(m_str1 != nullptr);
			assert(p >= m_str1 && p <= m_str1_end);
			m_text = p;
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
		const char_type *m_str1;
		const char_type *m_text;
		const char_type *m_text_end;
		const char_type *m_str1_end;

		size_t m_len1;
	};

	template<class traitsType>
	ctext<traitsType>::ctext(const char_type *s1, size_t l1, size_t n, size_t len) {
		assert(s1 != nullptr);

		if (l1 == static_cast<size_t>(-1)) {
			l1 = (s1) ? traits_type::length(s1) : 0;
		}

		if (n != static_cast<size_t>(-1) && n > 0) {
			if (n < l1) {
				s1 += n;
				l1 -= n;
			} else {
				s1 = nullptr;
				l1 = 0;
			}
		}

		if (len != static_cast<size_t>(-1) && len > 0) {
			if (len < l1) {
				l1 = len;
			} else {
				s1 = nullptr;
				l1 = 0;
			}
		}

		m_text = m_str1 = s1;
		m_len1 = l1;
		m_text_end = m_str1_end = m_str1 + m_len1;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator ++(int) {
		if (m_text == m_text_end) {
			return 0;
		}
		return *m_text++;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator --(int) {
		if (m_text < m_str1) return 0;
		return *--m_text;
	}

	template<class traitsType>
	void ctext<traitsType>::reset() {
		m_text = m_str1;
	}

	template<class traitsType>
	void ctext<traitsType>::current(int &ch) const {
		ch = m_text[-1];
	}

	template<class traitsType>
	int ctext<traitsType>::next(int_type &ch) {
		if (m_text == m_text_end) return 0;
		ch = *m_text++;
		return 1;
	}

	template<class traitsType>
	void ctext<traitsType>::unget(int_type &ch) {
		ch = (m_text == m_str1) ? 0 : *--m_text;
	}

	template<class traitsType>
	int ctext<traitsType>::advance(size_t n) {
		while (m_text != m_str1_end && n--) {
			++m_text;
		}
		return n;
	}

	template<class traitsType>
	int ctext<traitsType>::length() const {
		return m_len1;
	}

	template<class traitsType>
	bool ctext<traitsType>::at_begin() const {
		return (m_text == m_str1);
	}

	template<class traitsType>
	bool ctext<traitsType>::at_end() const {
		return (m_text == m_str1_end);
	}

	template<class traitsType>
	int ctext<traitsType>::position() const {
		if (m_text < m_str1) return 0;
		return m_text - m_str1;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator [](const int i) const {
		auto assize_t = static_cast<size_t>(i);
		assert(assize_t <= m_len1);
		return (assize_t == m_len1) ? 0 : m_str1[assize_t];
	}
}