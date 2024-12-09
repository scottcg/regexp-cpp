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
			if (_text == _text_end) return 0;
			return _text;
		}

		explicit operator char_type() const {
			if (_text == _text_end) return 0;
			assert(_text != nullptr);
			return *_text;
		}

		const char_type *text() const {
			return _text;
		}

		void text(const char_type *p) {
			assert(_str1 != nullptr);
			assert(p >= _str1 && p <= _str1_end);
			_text = p;
		}

		void reset();

		int next(int_type &ch);

		void current(int &ch) const;

		void unget(int_type &ch);

		int position() const;

		int advance(size_t n);

		bool has_substring(int begin, int end, int_type &lastch) {
			assert(true); //  "Not implemented");
			return false;
		}

		int length() const;

		bool at_begin() const;

		bool at_end() const;

	private:
		const char_type *_str1;
		const char_type *_text;
		const char_type *_text_end;
		const char_type *_str1_end;

		size_t _len1;
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

		_text = _str1 = s1;
		_len1 = l1;
		_text_end = _str1_end = _str1 + _len1;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator ++(int) {
		if (_text == _text_end) {
			return 0;
		}
		return *_text++;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator --(int) {
		if (_text < _str1) return 0;
		return *--_text;
	}

	template<class traitsType>
	void ctext<traitsType>::reset() {
		_text = _str1;
	}

	template<class traitsType>
	void ctext<traitsType>::current(int &ch) const {
		ch = _text[-1];
	}

	template<class traitsType>
	int ctext<traitsType>::next(int_type &ch) {
		if (_text == _text_end) return 0;
		ch = *_text++;
		return 1;
	}

	template<class traitsType>
	void ctext<traitsType>::unget(int_type &ch) {
		ch = (_text == _str1) ? 0 : *--_text;
	}

	template<class traitsType>
	int ctext<traitsType>::advance(size_t n) {
		while (_text != _str1_end && n--) {
			++_text;
		}
		return n;
	}

	template<class traitsType>
	int ctext<traitsType>::length() const {
		return _len1;
	}

	template<class traitsType>
	bool ctext<traitsType>::at_begin() const {
		return (_text == _str1);
	}

	template<class traitsType>
	bool ctext<traitsType>::at_end() const {
		return (_text == _str1_end);
	}

	template<class traitsType>
	int ctext<traitsType>::position() const {
		if (_text < _str1) return 0;
		return _text - _str1;
	}

	template<class traitsType>
	typename ctext<traitsType>::char_type ctext<traitsType>::operator [](const int i) const {
		auto assize_t = static_cast<size_t>(i);
		assert(assize_t <= _len1);
		return (assize_t == _len1) ? 0 : _str1[assize_t];
	}
}