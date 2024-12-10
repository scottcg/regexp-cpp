#pragma once

#include "tokens.h"
#include "precedence.h"
#include "compile.h"
#include "syntax_base.h"
#include "syntax_generic.h"


namespace re {
    ///////////////////////////////////////////////////////////////////////////////////////////
    // egrep. the most interesting part of this is the lack of registers.
    //
    //  c		any non-special character c matches itself
    //  \c		turn off any special meaning of character c
    //  ^		beginning of line
    //  $		end of line
    //  .		any single character
    //  [...]	any one of character in ...; ranges (a-z) are legal
    //  [^...]	any single character not in ...; ranges are legal
    //  r*		zero or more occurrences of r
    //  r+		one or more occurrences of r
    //  r?		zero or one occurrences of r
    //  r1r2	r1 followed by r2
    //  r1|r2	r1 or r2
    //  (r)		regular expression r. () are used for grouping.
    //
    //	no regular expression matches a newline (\n).

    template<class traitsType>
    class syntax_egrep : public syntax_generic<traitsType> {
    public:
        typedef compile_state<traitsType> re_compile_state_type;

    public:
        bool context_independent_ops() const override;

        bool incomplete_eoi(re_compile_state_type &cs) const override;

        int translate_plain_op(re_compile_state_type &cs) const override;

        int translate_escaped_op(re_compile_state_type &cs) const override;
    };

    template<class traitsT>
    bool syntax_egrep<traitsT>::context_independent_ops() const { return false; }

    template<class traitsT>
    bool syntax_egrep<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
        // check for '|' or ')' (note: ')' is not a register).
        assert(cs.input.at_end() == false);
        typename traitsT::int_type ch = 0;
        cs.input.get(ch);
        cs.input.unget();

        return (ch == '|' || ch == ')') ? true : false;
    }

    template<class traitsT>
    int syntax_egrep<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
        switch (cs.ch) {
            case '(':
            case ')':
            case '+':
            case '?':
            case '|':
                return cs.ch;
            default:
                return syntax_generic<traitsT>::translate_plain_op(cs);
        }
    }

    template<class traitsT>
    int syntax_egrep<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
        switch (cs.ch) {
            case 'n':
            case 'f':
            case 'b':
            case 'r':
            case 't':
                return TOK_CTRL_CHAR;
            default:
                return syntax_generic<traitsT>::translate_escaped_op(cs);
        }
    }

}