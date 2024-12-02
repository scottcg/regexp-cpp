#pragma once

#include "tokens.h"
#include "precedence.h"
#include "compile.h"
#include "syntax_base.h"
#include "syntax_generic.h"

namespace re {
    ///////////////////////////////////////////////////////////////////////////////////////////
    // grep contains tagged regular expressions.
    //
    //  c		any non-special character c matches itself
    //  \c		turn off any special meaning of character c
    //  ^		beginning of line
    //  $		end of line
    //  .		any single character
    //  [...]	any one of character in ...; ranges (a-z) are legal
    //  [^...]	any single character not in ...; ranges are legal
    //  \n		with the n'th \( \) matched
    //  r*		zero or more occurrences of r
    //  r1r2	r1 followed by r2
    //  \(...\)	tagged regular expression
    //
    //	no regular expression matches a newline (\n).

    template<class traitsType>
    class syntax_grep : public syntax_generic<traitsType> {
    public:
        typedef compile_state<traitsType> re_compile_state_type;

        bool context_independent_ops() const override;

        bool incomplete_eoi(re_compile_state_type &cs) const override;

        int translate_escaped_op(re_compile_state_type &cs) const override;
    };

    /////////////////////////////////////////////////////////////////////////////////////

    template<class traitsT>
    bool syntax_grep<traitsT>::context_independent_ops() const { return false; }

    // grep allows threes different escaped operators, all have to do
    // with backreferences (or tagged expressions in grep lingo).
    //	\( start of backref
    //	\) end of backref
    //	\digit refer to a backref -- should > 0 && < number of tags
    // anything else is just a character match.

    template<class traitsT>
    int syntax_grep<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
        static_assert(std::is_same<traitsT, re_char_traits<char>>::value, "traitsT is not re_char_traits<char>");

        if (traitsT::isdigit(static_cast<typename traitsT::int_type>(cs.ch))) {
            typename traitsT::int_type value = 0;
            int n_digits = cs.input.peek_number(value);

            if (cs.number_of_backrefs && value <= cs.number_of_backrefs && value > 0) {
                cs.input.advance(n_digits - 1);
                cs.ch = value;
                return TOK_BACKREF;
            }
            return SYNTAX_ERROR;
        }

        switch (cs.ch) {
            case '(':
            case ')':
                return cs.ch;
            default:
                return TOK_CHAR;
        }
    }

    // check for "\)" as in expressions like "\(^foo$\)"; i need to check to
    // make sure that this is legal grep syntax.

    template<class traitsT>
    bool syntax_grep<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
        assert(cs.input.at_end() == false);
        typename traitsT::int_type ch = 0;
        cs.input.get(ch);
        if (ch == '\\' && cs.input.at_end() == false) {
            cs.input.get(ch);
            cs.input.unget();
            cs.input.unget();
            if (ch == ')') {
                return true;
            }
        } else {
            cs.input.unget();
        }
        return false;
    }
}