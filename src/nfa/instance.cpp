// $Header: /regexp/instance.cpp 1     2/22/97 6:03p Scg $

#include "regexp.cpp"
#include "syntax.cpp"

#pragma warning ( once: 4661 )

#ifndef _UNICODE

template syntax_base<std::string>;

template generic_syntax<std::string>;
template grep_syntax<std::string>;
template egrep_syntax<std::string>;
template awk_syntax<std::string>;
template perl_syntax<std::string>;

// template basic_regexp< std::string, char, int>;
//template basic_regexp<std::string>;
template basic_regexp< syntax_base<std::string> >;

// template syntax_regexp< perl_syntax<char, int>, std::string >;
// template syntax_regexp< grep_syntax<char, int>, std::string >;
// template syntax_regexp< egrep_syntax<char, int>, std::string >;
// template syntax_regexp< awk_syntax<char, int>, std::string >;
// template syntax_regexp< generic_syntax<char, int>, std::string >;

#else

template syntax_base<wchar_t, wint_t>;

template generic_syntax<wchar_t, wint_t>;
template grep_syntax<wchar_t, wint_t>;
template egrep_syntax<wchar_t, wint_t>;
template awk_syntax<wchar_t, wint_t>;
template perl_syntax<wchar_t, wint_t>;

template basic_regexp< std::wstring, wchar_t, wint_t>;

template syntax_regexp< perl_syntax<wchar_t, wint_t>, std::wstring >;
template syntax_regexp< grep_syntax<wchar_t, wint_t>, std::wstring >;
template syntax_regexp< egrep_syntax<wchar_t, wint_t>, std::wstring >;
template syntax_regexp< awk_syntax<wchar_t, wint_t>, std::wstring >;
template syntax_regexp< generic_syntax<wchar_t, wint_t>, std::wstring >;

#endif
