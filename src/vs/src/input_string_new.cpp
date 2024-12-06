#pragma once

#include <span>
#include <cassert>
#include <locale>

namespace  re {
    template<typename TraitsType>
    class input_string_new {
    public:
        using char_type = typename TraitsType::char_type;
        using int_type = typename TraitsType::int_type;

        explicit input_string_new(const char_type *s, std::size_t len = static_cast<std::size_t>(-1))
            : _span(s, (len == static_cast<std::size_t>(-1)) ? TraitsType::length(s) : len), _offset(0) {
        }

        // Access current character
        char_type operator[](std::size_t i) const {
            return _span[i];
        }

        // Length of the string
        std::size_t length() const {
            return _span.size();
        }

        // Get and advance
        bool get(int_type &value) {
            if (at_end()) return false;
            value = _span[_offset++];
            return true;
        }

        // Peek at the next character
        int_type peek() const {
            assert(!at_end());
            return _span[_offset];
        }

        // Move backwards
        void unget() {
            assert(!at_begin());
            --_offset;
        }

        // Advance by a given amount
        void advance(std::size_t skip) {
            assert((_offset + skip) <= _span.size());
            _offset += skip;
        }

        // Check if at the beginning or end
        bool at_begin() const {
            return _offset == 0;
        }

        bool at_end() const {
            return _offset == _span.size();
        }

        // Current offset
        std::size_t offset() const {
            return _offset;
        }

    private:
        std::span<const char_type> _span;
        std::size_t _offset;
    };
}
