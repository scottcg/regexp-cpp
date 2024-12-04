#pragma once

#include "tokens.h"
#include "precedence.h"
#include "compile.h"
#include "syntax_base.h"

namespace re {
    template<class traitsType>
        class syntax_python : public syntax_base<traitsType> {
    public:
        typedef compile_state<traitsType> compile_state_type;

        bool context_independent_ops() const override;

        int translate_plain_op(compile_state_type &) const override;

        int translate_escaped_op(compile_state_type &) const override;

        int precedence(int op) const override;
    };

    template<class traitsT>
    bool syntax_python<traitsT>::context_independent_ops() const { return 1; }

    template<class traitsT>
    int syntax_python<traitsT>::translate_plain_op(compile_state_type &cs) const {
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
    int syntax_python<traitsT>::translate_escaped_op(compile_state_type &) const {
        return TOK_CHAR;
    }

    template<class traitsT>
    int syntax_python<traitsT>::precedence(int op) const {
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