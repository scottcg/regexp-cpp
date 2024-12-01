#pragma once

#include "concepts.h"
#include "precedence.h"
#include "code.h"
#include "source.h"

namespace re {
    template<class traitsType>
    class re_syntax_base;

    /////////////////////////////////////////////////////////////////////////////////////
    // compile_state contains all the interesting bits necessary to pass around when
    // compiling a regular expression. in more basic terms, i could replace this class
    // by lengthening the function parameter lists, instead i have this class with a
    // bunch of public members (and lengthening parameter lists using vc++ is a dangerous
    // undertaking -- especially in templates).

    template<typename traitsType>
        requires IsReCharTraits<traitsType>
    class re_compile_state {
    public:
        typedef traitsType traits_type;
        typedef typename traitsType::char_type char_type;
        typedef typename traitsType::int_type int_type;
        typedef re_syntax_base<traitsType> syntax_type;
        typedef compiled_code_vector<traitsType> code_vector_type;
        typedef re_input_string<traitsType> source_vector_type;
        typedef std::stack<int> open_backref_stack;

        re_compile_state(const syntax_type &syn, code_vector_type &out, source_vector_type &in)
            : op(0),
              ch(0),
              beginning_context(1),
              cclass_complement(false),
              parenthesis_nesting(0),
              group_nesting(0),
              number_of_backrefs(0),
              next_backref(1),
              syntax(syn),
              input(in),
              output(out) {
        }

    public:
        int_type op; // current converted operation.
        int_type ch; // current input character.
        int beginning_context; // are we at the "start".
        bool cclass_complement; // is the current [] or [^].

        int parenthesis_nesting; // should be 0 when compile is done.
        int group_nesting; // should be 0 when compile is done.
        int number_of_backrefs; // number of () found.

        int next_backref; // next free backref number.
        open_backref_stack backref_stack; // stack for nesting backrefs.
        re_future_jump_stack jump_stack; // left open gotos.
        re_precedence_stack prec_stack; // operator precedence stack.

        const syntax_type &syntax; // what is our syntax object
        source_vector_type &input; // ref to the input character stream.
        code_vector_type &output; // ref to where the output code is going.
    };
}
