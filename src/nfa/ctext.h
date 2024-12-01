#pragma once

/////////////////////////////////////////////////////////////////////////////
// a small template clas to handle the peculiarities of having two strings passed to the
// match/search functions. this class tries to hide that fact and presents something
// that looks like a single "text string".

template <class strType>
class ctext {
public:
	typedef typename strType::char_traits::char_type char_type;
	typedef typename strType::char_traits::char_type int_type;	// not a mistake.

public:
	ctext(const char_type* s1, size_t l1, size_t p = 0, size_t pstop = -1,
			const char_type* s2 = 0, size_t l2 = -1);

	char_type operator [] (int i) const { return text[i]; }
	operator const char_type* () const { return text; }

	int absolute_position() const;

	const char_type* text_ptr() const { return text; }
	void text_ptr(const char_type* p) const { text = p; }
	const char_type* partend_ptr() const { return partend; }
	void partend_ptr(const char_type* p) { return partend; }

public:
	void current(int& ch);
	int next(int_type& ch);
	void unget(int_type& ch);

	int skip(size_t n);
	int length() const;
	int start() const;
	
	void start(size_t s);

	bool at_begin() const;
	bool at_end() const;

	bool buffer_begin() const;
	bool buffer_end() const;
	bool begin_of_strs() const;
	bool end_of_strs() const;

	int match_end(const char_type* p = 0) const;

	bool word_test() const;
	bool word_begin() const;
	bool word_end() const;
	bool word_boundary() const;
	bool not_word_boundary() const;

	bool compare(int begin, int end, int_type& ch);

private:
	const char_type*	str1;
	const char_type*	str2;
	size_t			len1;
	size_t			len2;
	size_t			pos_stop;
	size_t			pos;

	const char_type*	text;
	const char_type*	textend;
	const char_type*	partend;
	const char_type*	part2_end;
};

template <class strT>
ctext<strT>::ctext(const char_type* s1, size_t l1, size_t p, size_t pstop,
				   const char_type* s2, size_t l2) {
	str1 = s1;
	str2 = s2;
	len1 = l1;
	len2 = l2;
	pos_stop = pstop;
	pos = p;

	check_string(str1, len1);
	check_string(str2, len2);

	if ( pos_stop == -1 ) {
		pos_stop = len1 + len2;
	}

	start(pos);

	// double check the parameters to see if we have maintained our sanity.
	assert(pos >= 0 && len1 >= 0);
	assert(len2 >= 0 && pos_stop >= 0);
	assert(pos_stop <= len1 + len2);
	assert(pos <= pos_stop);
}

template <class strT>
inline void ctext<strT>::current(int& ch) { ch = text[-1]; }

template <class strT>
inline int ctext<strT>::next(int_type& ch) {
	if ( text == partend ) {
		if ( text == textend ) return 1;
		text = str2;
		partend = part2_end;
	}
	ch = *text++;
	return 0;
}

template <class strT>
inline void ctext<strT>::unget(int_type& ch) {
#if notdone
	if ( text == str2 ) {
		if ( pos_stop <= len1 ) {
			partend = s1 + pos_stop;
		} else {
			partend = s1 + l1;
		}
		text = s1;
	}
#endif
	ch = *--text;
}

template <class strT>
inline int ctext<strT>::skip(size_t n) {
	while ( text != partend && n-- ) {
		text++;
	}
	return n;
}

template <class strT>
inline int ctext<strT>::length() const { return len1 + len2; }

template <class strT>
inline int ctext<strT>::start() const { return pos; }

template <class strT>
inline void ctext<strT>::start(size_t s) {
	pos = s;
	if ( pos <= len1 ) {
	  text = str1 + pos;
	  if ( pos_stop <= len1 ) {
		  textend = partend = str1 + pos_stop;
	  } else {
		  textend = partend = str1 + len1;
	  }
	  part2_end = str2 + pos_stop - len1;
	} else {
		text = str2 + pos - len1;
		textend = partend = part2_end = str2 + pos_stop - len1;
	}
}

template <class strT>
inline bool ctext<strT>::at_begin() const { return ((text == str1) ? true : false); }

template <class strT>
inline bool ctext<strT>::at_end() const {
	if ( text == str2 + len2 || (text == str1 + len1 ?
			(len2 == 0 || *str2 == '\n') : *text == '\n')) {
		return true;
	} 
	return false;
}

template <class strT>
inline bool ctext<strT>::buffer_begin() const
{ return (text == str1) ? true : false; }

template <class strT>
inline bool ctext<strT>::buffer_end() const {
	if ( len2 == 0 ) {
		if ( text == str1 + len1 ) {
			return true;
		} else if ( text == str2 + len2 ) {
			return true;
		}
	}
	return false;
}

template <class strT>
inline bool ctext<strT>::word_test() const {
	int_type temp = 0;
	if ( text == str1 + len1 ) {
		temp = *str2;
	} else {
		temp = *text;
	}
	if ( is_a_alnum(text[-1]) || is_a_alnum(temp)  ) {
		return true;
	}
	return false;
}

template <class strT>
inline bool ctext<strT>::begin_of_strs() const {
	if ( text == str1 || text == str2 + len2 || (len2 == 0 && text == str1 + len1) ) {
		return true;
	}
	return false;
}

template <class strT>
inline bool ctext<strT>::end_of_strs() const {
	if ( (text == str2 + len2) || (len2 == 0 && text == str1 + len1) ) {
		return true;
	}
	return false;
}

template <class strT>
inline int ctext<strT>::match_end(const char_type* p) const {
	if ( p == 0 ) p = text;
	if ( partend != part2_end ) {
		return p - str1;
	} else {
		return p - str2 + len1;
	}
}

template <class strT>
inline bool ctext<strT>::word_begin() const { return false; }

template <class strT>
inline bool ctext<strT>::word_end() const { return false; }

template <class strT>
inline bool ctext<strT>::word_boundary() const { return false; }

template <class strT>
inline bool ctext<strT>::not_word_boundary() const { return false; }

template <class strT>
inline int ctext<strT>::absolute_position() const {
	return text - str1; // not done for two strings.
}

//template <class strT>
//inline const ctext<strT>::char_type* ctext<strT>::text_ptr() const { return text; }

//template <class strT>
//inline void ctext<strT>::text_ptr(const char_type* p) const { text = p; }

//template <class strT>
//inline const ctext<strT>::char_type* ctext<strT>::partend_ptr() const { return partend; }

//template <class strT>
//inline void ctext<strT>::partend_ptr(const char_type* p) { partend = p; }

//template <class strT>
//inline ctext<strT>::char_type ctext<strT>::operator [] (int i) const { return text[i]; }

//template <class strT>
//inline ctext<strT>::operator const ctext<strT>::char_type* () const { return text; }

template <class strT>
inline bool ctext<strT>::compare(int begin, int end, int_type& ch) {
	if ( begin < 0 || end < 0 ) {
		return false;
	}
	// this isn't done yet; it's set up to handle only one string; the top code is what
	// was here before, and needs to be updated to handle two strings.
#if 0
	const char_type* ref_textend = bref.end_text;
	const char_type* ref_partend = 0;

	if ( bref.start_partend == bref.end_partend ) {
		ref_partend = ref_textend;
	} else {
		ref_partend = str1 + len1;
	}
#else
	const char_type* ref_textend = str1 + end;
	const char_type* ref_partend = ref_textend;
#endif

	const char_type* ref_text = str1 + begin;
	while ( ref_text != ref_partend ) {
		if ( next(ch) ) {
			return false;
		}
		if ( ref_text == ref_partend ) {
			ref_text = str2;
		}
		int regch = (char_type)*ref_text++;
		if ( regch != ch ) {
			return false;
		}
	}
	return true;
}
