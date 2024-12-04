#pragma once

#include <locale>
#include <cstring>
#include <cwchar>
#include <string>

template<class T>
struct re_char_traits : std::char_traits<T> {
    typedef T char_type;
    typedef typename std::char_traits<T>::int_type int_type;
	typedef std::basic_string<T> string_type;
};

// Specialization for char (inherits from std::char_traits<char>)
template<>
struct re_char_traits<char> : std::char_traits<char> {
    using traits_type = std::char_traits<char>;
	typedef std::basic_string<char> string_type;

    static size_t length(const char_type *x) { return traits_type::length(x); }

    static int isalpha(const int_type c) { return std::isalpha(c); }
    static int isalnum(const int_type c) { return std::isalnum(c); }
    static int isspace(const int_type c) { return std::isspace(c); }
    static int isdigit(const int_type c) { return std::isdigit(c); }
    static int islower(const int_type c) { return std::islower(c); }
    static int toupper(const int_type c) { return std::toupper(c); }
    static int tolower(const int_type c) { return std::tolower(c); }

    static int cstr_to_decimal_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::isdigit(ch = *s++)) {
            i = (i * 10) + (ch - '0');
        }
        return i;
    }

    static int cstr_to_octal_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::isdigit(ch = *s++)) {
            i = (i * 8) + (ch - '0');
        }
        return i;
    }

    static int cstr_to_hex_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::isdigit(ch = *s++)) {
            i = (i * 16) + (ch - '0');
        }
        return i;
    }

    static int strncmp(const char_type* s1, const char_type* s2, const size_t n) {
        return std::char_traits<char_type>::compare(s1, s2, n);
    }

    static int istrncmp(const char_type* s1, const char_type* s2, size_t n) {
        while (n-- != 0) {
            int_type c1 = std::tolower(*s1++);
            int_type c2 = std::tolower(*s2++);
            if (c1 != c2) {
                return c1 - c2;
            }
            if (c1 == '\0') {
                break;
            }
        }
        return 0;
    }

    static int has_chars(const char_type* s1, const char_type* s2) {
        const char_type* t = std::strpbrk(s1, s2);
        return (t) ? (t - s1) : -1;
    }

    static int check(const char_type* s, size_t& n) {
        if (n == static_cast<size_t>(-1)) {
            n = (s) ? length(s) : 0;
        }
        return n;
    }
};

template<>
struct re_char_traits<wchar_t> : public std::char_traits<wchar_t> {
    using traits_type = std::char_traits<wchar_t>;
	typedef std::basic_string<wchar_t, std::char_traits<wchar_t>> string_type;

    static size_t length(const char_type *s) { return traits_type::length(s); }
    static int isalpha(const int_type c) { return std::iswalpha(c); }
    static int isalnum(const int_type c) { return std::iswalnum(c); }
    static int isspace(const int_type c) { return std::iswspace(c); }
    static int isdigit(const int_type c) { return std::iswdigit(c); }
    static int islower(const int_type c) { return std::iswlower(c); }
    static int toupper(const int_type c) { return std::towupper(c); }
    static int tolower(const int_type c) { return std::towlower(c); }

    static int cstr_to_decimal_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::iswdigit(ch = *s++)) {
            i = (i * 10) + (ch - L'0');
        }
        return i;
    }

    static int cstr_to_octal_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::iswdigit(ch = *s++)) {
            i = (i * 8) + (ch - L'0');
        }
        return i;
    }

    static int cstr_to_hex_int(const char_type* s) {
        int i = 0;
        char_type ch = 0;
        while (s && *s != 0 && std::iswdigit(ch = *s++)) {
            i = (i * 16) + (ch - L'0');
        }
        return i;
    }

    static int strncmp(const char_type* s1, const char_type* s2, const size_t n) {
        return std::wcsncmp(s1, s2, n);
    }

    static int istrncmp(const char_type* s1, const char_type* s2, size_t n) {
        while (n-- != 0) {
            int_type c1 = std::towlower(*s1++);
            int_type c2 = std::towlower(*s2++);
            if (c1 != c2) {
                return c1 - c2;
            }
            if (c1 == L'\0') {
                break;
            }
        }
        return 0;
    }

    static int has_chars(const char_type* s1, const char_type* s2) {
        const char_type* t = std::wcspbrk(s1, s2);
        return (t) ? (t - s1) : -1;
    }

    static int check(const char_type* s, size_t& n) {
        if (n == static_cast<size_t>(-1)) {
            n = (s) ? length(s) : 0;
        }
        return n;
    }
};
