#pragma once

#include <string_view>
#include <cstddef>
#include "traits.h"

namespace re {

    template<class traitsType>
    class cursor_string_view {
    public:
        typedef traitsType traits_type;
        typedef typename traitsType::char_type char_type;
        typedef typename traitsType::int_type int_type;
        typedef std::size_t size_type;
        typedef typename traits_type::string_view_type string_view_type;

        explicit cursor_string_view(std::string_view view)
            : view_(view), cursor_(0) {}

        // Cursor-based navigation
        char_type next() {
            if (at_end()) return '\0';
            return view_[cursor_++];
        }

        char_type prev() {
            if (at_begin()) return '\0';
            return view_[--cursor_];
        }

        void advance(std::size_t n) {
            cursor_ = std::min(cursor_ + n, view_.size());
        }

        void reset() { cursor_ = 0; }

        // Accessors
        char_type current() const {
            return at_end() ? '\0' : view_[cursor_];
        }

        size_t position() const { return cursor_; }

        bool at_begin() const { return cursor_ == 0; }
        bool at_end() const { return cursor_ >= view_.size(); }

        string_view_type remaining() const {
            return view_.substr(cursor_);
        }

        size_t length() const { return view_.size(); }

    public: // transition
        // Index operator (added for compatibility)
        char_type operator[](int i) const {
            if (i < 0 || static_cast<std::size_t>(i) >= view_.size()) {
                return '\0';  // Out of bounds
            }
            return view_[i];
        }

    private:
        string_view_type view_;
        size_t cursor_;
    };

} // namespace re