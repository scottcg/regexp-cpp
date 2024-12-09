#pragma once

#include "tokens.h"
#include "precedence.h"
#include "compile.h"
#include "syntax_base.h"

namespace re {
    // generic syntax contains the syntax code for the "common" regular expressions.
    //  c			non-metacharacter c
    //  \c			escape sequence or literal character c
    //  ^			beginning of string
    //  $			end of string
    //  .			any character
    //  [ab...]		any character in ab...
    //  [^ab...]	any character not in ab...
    //  [a-b]		any character in the range a-b
    //  [^a-b]		any character not in the range a-b
    //  r*			zero or more occurrences of r
    //
    template<class traitsType>
    class syntax_generic : public syntax_base<traitsType> {
    public:
        typedef compile_state<traitsType> re_compile_state_type;

        bool context_independent_ops() const override;

        int translate_plain_op(re_compile_state_type &) const override;

        int translate_escaped_op(re_compile_state_type &) const override;

        int precedence(int op) const override;
    };


    template<class traitsT>
    bool syntax_generic<traitsT>::context_independent_ops() const { return true; }

    template<class traitsT>
    int syntax_generic<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
        switch (cs.ch) {
            case '\\':
                return TOK_ESCAPE;
            case '[':
            case ']':
            case '^':
            case '$':
            case '.':
            case '*':
                return cs.ch;
            default:
                return TOK_CHAR;
        }
    }

    template<class traitsT>
    int syntax_generic<traitsT>::translate_escaped_op(re_compile_state_type &) const {
        return TOK_CHAR;
    }

    template<class traitsT>
    int syntax_generic<traitsT>::precedence(const int op) const {
        switch (op) {
            case TOK_END:
                return 0;
            case ')':
                return 1;
            case '|':
                return 2;
            case '^':
            case '$':
                return 3;
            default:
                return 4;
        }
    }
}