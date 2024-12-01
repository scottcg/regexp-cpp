#pragma once

#include <vector>
#include <stack>
#include "concepts.h"

namespace re {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // number of precedence levels in use, if you change this number, be sure to add 1 so that
    // ::store_cclass will have the highest precedence for it's own use (it's using NUM_LEVELS - 1).
    constexpr int NUM_LEVELS = 6;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the re_precedence_vec and re_precedence_stack work together to provide and n precedence levels
    // and (almost) unlimited amount of nesting. the nesting occurs when via the precedence stack
    // (push/pop) and the re_precedence_vec stores the current offset of the output code via
    // a store current precedence. did you get all of that?

    template<int sz>
    class re_precedence_vec : public std::vector<int> {
    public:
        explicit re_precedence_vec(const int init = 0) : std::vector<int>(sz, init) {
        }
    };

    typedef re_precedence_vec<NUM_LEVELS> re_precedence_element;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the actual precedence stack; adds a couple of handy members to keep track of the positions
    // in the re_precedence_element.

    class re_precedence_stack : public std::stack<re_precedence_element> {
    public:
        re_precedence_stack() : m_current(0) {
            push(re_precedence_element());
        }

        // current precedence value.
        int current() const { return m_current; }
        void current(int l) { m_current = l; }

        // where we are int the input stream; index in array indicates precedence
        int start() const { return top().at(m_current); }
        void start(const int offset) { top().at(m_current) = offset; }

    private:
        int m_current;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // trivial stack used to keep track of the offsets in the code vector where jumps
    // should occur when changing to lower precedence operators. after a successful
    // compilation of a regular expression the future jump stack should be empty.

    typedef std::stack<int> re_future_jump_stack;
    typedef re_precedence_stack re_precedence_stack;
}
