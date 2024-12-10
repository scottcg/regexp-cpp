#pragma once

#include "traits.h"
#include "tokens.h"

namespace re {
    template<class traitsT>
    struct instruction {
        typedef traitsT traits_type;
        typedef typename traits_type::char_type char_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::char_type code_type;

        opcodes op; // Opcode (e.g., OP_CHAR, OP_RANGE_CHAR, etc.)
        int_type arg1; // First argument (e.g., character, range start, jump target, etc.)
        int_type arg2; // Second argument (e.g., range end, repetition max, etc.)

        explicit instruction(const opcodes opcode)
            : instruction(opcode, 0, 0) {
        } // Delegates to 3-argument constructor

        instruction(const opcodes opcode, const int_type argument1)
            : instruction(opcode, argument1, 0) {
        } // Delegates to 3-argument constructor

        instruction(const opcodes opcode, const int_type argument1, const int_type argument2)
            : op(opcode), arg1(argument1), arg2(argument2) {
        }
    };
}