#pragma once

#include "tokens.h"
#include "precedence.h"
#include "syntax_base.h"

namespace re {
///////////////////////////////////////////////////////////////////////////////////////////
    // perl extensions to awk; actually extensions to awk with some grep thrown in just to
    // muddy things up. the grep part is registers, but perl registers don't need escaping.
    //
    //  these are some additional escapes, most of these look like they came
    //  from emacs.
    //	\d matches [0-9]
    //	\D matches [^0-9]
    //	\b matches on a word boundary (between \w and \W)
    //	\B matches on a non-word boundary
    //	\s [ \t\r\n\f]
    //	\S [^ \t\r\n\f]
    //	\w [0-9a-zA-Z]
    //	\W [^0-9a-zA-Z]
    //
    //	{n,m} must occur at least n times but no more than m times.
    //	{n,} must occur at least n times
    //	{n} must match exactly n times
    //	just a note:
    //		* same as {0,}
    //		+ same as {1,}
    //		? same as {0,1}
    //
    //	\r\f\b\n inherited from awk.
    //	\000 is an octal number, unless the number would match a prior
    //		register.
    //	\xNN as in \x7f matches that character (hex matching)
    //	\cD matches control character \cL matches ^L
    //
    //  the really complicated part here is that one can use escapes inside
    //  sets: so, \d\w\s\n\r\t\f\b are interpolated within a set...

    template<class traitsType>
    class syntax_perl : public syntax_egrep<traitsType> {
    public:
        typedef compile_state<traitsType> re_compile_state_type;
    	typedef traitsType traits_type;

    public:
        int precedence(int op) const override {
            return syntax_generic<traitsType>::precedence(op);
        }

        bool context_independent_ops() const override;

        bool incomplete_eoi(re_compile_state_type &cs) const override;

        int compile_opcode(re_compile_state_type &cs) const override;

        int translate_plain_op(re_compile_state_type &cs) const override;

        int translate_escaped_op(re_compile_state_type &cs) const override;

        int translate_char_class_escaped_op(re_compile_state_type &cs) const override;
    };

    template<class traitsT>
	bool syntax_perl<traitsT>::context_independent_ops() const { return false; }

	template<class traitsT>
	int syntax_perl<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
		switch (cs.ch) {
			case '{':
			case '}':
				return cs.ch;

			default:
				return syntax_egrep<traitsT>::translate_plain_op(cs);
		}
	}

	// perl has a bunch of escaped ops; the most complicated is the
	// backreferences, since depending upon the context a what looks like
	// a backref could be a constant (like \33 == \033).
	// the rest of the escaped ops are really short cuts for something that
	// could be specified by hand (like \d = [0-9]).

	template<class traitsT>
	int syntax_perl<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
		if (traitsT::isdigit(cs.ch)) {
			typename traitsT::int_type value = 0;
			int n_digits = cs.input.peek_number(value);

			if (value != 0 && value <= cs.number_of_backrefs) {
				cs.ch = value;
				return TOK_BACKREF;
			} else {
				// should be a constant, like an octal number? /0xx or /0xxxxx
				cs.input.advance(n_digits - 1);
				cs.ch = value;
				return TOK_CHAR;
			}
		}

		switch (cs.ch) {
			case 'b':
			case 'B':
			case 'd':
			case 'D':
			case 's':
			case 'S':
			case 'w':
			case 'W':
				return cs.ch;

			case 'c':
			case 'x':
				return TOK_CTRL_CHAR;

			default:
				return syntax_egrep<traitsT>::translate_escaped_op(cs);
		}
	}

	template<class traitsT>
	bool syntax_perl<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
		assert(cs.input.at_end() == false);
		typename traitsT::int_type ch = 0;
		cs.input.get(ch);
		cs.input.unget();

		return (ch == '|' || ch == ')') ? true : false;
	}

	template<class traitsT>
	int syntax_perl<traitsT>::compile_opcode(re_compile_state_type &cs) const {
		switch (cs.op) {
			case '?':
			case '*':
			case '+':
				if (cs.beginning_context) {
					if (context_independent_ops()) {
						return ILLEGAL_OPERATOR;
					} else {
						// cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
						cs.prec_stack.start(cs.output.store(OP_CHAR, cs.ch));
						break;
					}
				}

			// we skip empty expressions for ?, + and * matches
				if (cs.prec_stack.start() == cs.output.offset()) {
					break;
				}

				if (!cs.input.at_end() && cs.input.peek() == '?') {
					// stingy versions
					typename traitsT::int_type t;
					cs.input.get(t); // consume the '?'

					switch (cs.op) {
						case '?':
							cs.output.store_jump(cs.prec_stack.start(), OP_GOTO,
							                     cs.output.offset() + 3);
							cs.output.store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE,
							                     cs.prec_stack.start() + 6);
							break;

						case '*':
							cs.output.store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE,
							                     cs.output.offset() + 6);
							cs.output.store_jump(cs.output.offset(), OP_PUSH_FAILURE,
							                     cs.prec_stack.start());
							cs.output.store_jump(cs.prec_stack.start(), OP_FAKE_FAILURE_GOTO,
							                     cs.output.offset());
							break;

						case '+':
							cs.output.store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE,
							                     cs.output.offset() + 6);
							cs.output.store_jump(cs.output.offset(), OP_PUSH_FAILURE,
							                     cs.prec_stack.start());
							cs.output.store_jump(cs.prec_stack.start(), OP_FAKE_FAILURE_GOTO,
							                     cs.prec_stack.start() + 6);
							break;
					}
				} else {
					// greedy versions
					switch (cs.op) {
						case '?':
							cs.output.store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE,
							                     cs.output.offset() + 3);
							break;

						case '+':
						case '*':
							cs.output.store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE,
							                     cs.output.offset() + 6);
							cs.output.store_jump(cs.output.offset(), OP_GOTO,
							                     cs.prec_stack.start());

							if (cs.op == '+') {
								// jump over initial failure_jump
								cs.output.store_jump(cs.prec_stack.start(), OP_FAKE_FAILURE_GOTO,
								                     cs.prec_stack.start() + 6);
							}
							break;
					}
				}
				break;

			case '(':
				++cs.number_of_backrefs;
				++cs.parenthesis_nesting;

				cs.prec_stack.start(cs.output.offset());

				cs.output.store(OP_BACKREF_BEGIN, cs.next_backref);
				cs.backref_stack.push(cs.next_backref++);

				cs.prec_stack.push(re_precedence_element());
				cs.prec_stack.current(0);
				cs.prec_stack.start(cs.output.offset());
				break;

			case ')':
				if (cs.parenthesis_nesting <= 0) {
					return MISMATCHED_PARENTHESIS;
				}
				--cs.parenthesis_nesting;

				assert(cs.prec_stack.size() > 1);
				cs.prec_stack.pop();
				cs.prec_stack.current(precedence('('));

				cs.output.store(OP_BACKREF_END, cs.backref_stack.top());
				cs.backref_stack.pop();
				break;

			case TOK_EXT_REGISTER:
				if (cs.input.get_number(cs.ch) || cs.ch <= 0 || cs.ch >= MAX_BACKREFS) {
					return BACKREFERENCE_OVERFLOW;
				}
				//remove cs.output.store(OP_BACKREF, cs.ch, cs.prec_stack);
			cs.prec_stack.start(cs.output.store(OP_BACKREF, cs.ch));
				break;

			case '{':
				return cs.output.store_closure(cs);
				break;

			case 'b':
				//cs.output.store(OP_WORD_BOUNDARY, 0, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_WORD_BOUNDARY, 0));
				break;

			case 'B':
				//cs.output.store(OP_WORD_BOUNDARY, 1, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_WORD_BOUNDARY, 1));
				break;

			case 'd':
				cs.output.store(OP_DIGIT, 0);
				break;

			case 'D':
				cs.output.store(OP_DIGIT, 1);
				break;

			case 's':
				cs.output.store(OP_SPACE, 0);
				break;

			case 'S':
				cs.output.store(OP_SPACE, 1);
				break;

			case 'w':
				cs.output.store(OP_WORD, 0);
				break;

			case 'W':
				cs.output.store(OP_WORD, 1);
				break;

			default:
				return syntax_egrep<traitsT>::compile_opcode(cs);
		}
		return 0;
	}

	template<class traitsT>
	int syntax_perl<traitsT>::translate_char_class_escaped_op(re_compile_state_type &cs) const {
		if (traitsT::isdigit(cs.ch)) {
			typename traitsT::int_type value = 0;
			int n_digits = cs.input.peek_number(value);

			if (value != 0 && value <= cs.number_of_backrefs) {
				// backref? ok here?
				cs.output.store(OP_BIN_CHAR, value);
			} else {
				// should be a constant, like an octal number? /0xx or /0xxxxx
				cs.input.advance(n_digits - 1);
				cs.output.store(OP_BIN_CHAR, value);
			}
			return 0;
		}

		const int flag = (cs.cclass_complement == true) ? 1 : 0;
		switch (cs.ch) {
			case 'd':
				cs.output.store(OP_DIGIT, flag);
				break;

			case 's':
				cs.output.store(OP_SPACE, flag);
				break;

			case 'w':
				cs.output.store(OP_WORD, flag);
				break;

			case 'n':
				cs.output.store(OP_BIN_CHAR, '\n');
				break;

			case 'r':
				cs.output.store(OP_BIN_CHAR, '\r');
				break;

			case 't':
				cs.output.store(OP_BIN_CHAR, '\t');
				break;

			case 'f':
				cs.output.store(OP_BIN_CHAR, '\f');
				break;

			case 'b':
				cs.output.store(OP_BIN_CHAR, '\b');
				break;

			default:
				return syntax_egrep<traitsT>::translate_char_class_escaped_op(cs);
		}
		return 0;
	}
}