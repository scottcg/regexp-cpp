#pragma once


#include "tokens.h"
#include "precedence.h"
#include "compile.h"

namespace re {
	template<class traitsType>
	class compiled_code_vector;

	template<class traitsType>
	class syntax_base {
	public:
		virtual ~syntax_base() = default;

		typedef traitsType traits_type;
		typedef typename traits_type::char_type char_type;
		typedef typename traits_type::int_type int_type;
		typedef compile_state<traitsType> re_compile_state_type;

		virtual bool context_independent_ops() const { return false; }

		virtual int precedence(int op) const { return 0; }

		virtual int compile(re_compile_state_type &cs) const {
			cs.op = ~TOK_END;
			while (cs.op != TOK_END) {
				if (cs.input.at_end() == true) {
					cs.op = TOK_END;
				} else {
					cs.input.get(cs.ch);
					cs.op = cs.syntax.translate_plain_op(cs);
					if (cs.op == TOK_ESCAPE) {
						cs.input.get(cs.ch);
						cs.op = cs.syntax.translate_escaped_op(cs);
						if (cs.op == TOK_CTRL_CHAR) {
							cs.op = TOK_CHAR;
							if (cs.input.translate_ctrl_char(cs.ch)) {
								return SYNTAX_ERROR;
							}
						}
					}
				}
				// propagate the current character offset to the higher level (precedence)
				// or clean up and pending jumps for lower precedence operators.

				int level = cs.syntax.precedence(cs.op);
				if (level > cs.prec_stack.current()) {
					int c = cs.prec_stack.current();
					for (cs.prec_stack.current(c++); cs.prec_stack.current() < level;
					     cs.prec_stack.current(c++)) {
						cs.prec_stack.start(cs.output.offset());
					}
					cs.prec_stack.start(cs.output.offset());
				} else if (level < cs.prec_stack.current()) {
					cs.prec_stack.current(level);
					while (cs.jump_stack.size()) {
						if (cs.jump_stack.top() >= cs.prec_stack.start()) {
							cs.output.put_address(cs.jump_stack.top(), cs.output.offset());
							cs.jump_stack.pop();
						} else {
							break;
						}
					}
				}

				if (int err = cs.syntax.compile_opcode(cs)) {
					// syntax error someplace
					return err;
				}
				cs.beginning_context = (cs.op == '(' || cs.op == '|');
			}
			return 0;
		}

		virtual int compile_opcode(re_compile_state_type &cs) const {
			switch (cs.op) {
				case TOK_END:
					// cs.output.store(OP_END, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_END));
					break;

				case TOK_CHAR:
					//remove cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_CHAR, cs.ch));
					break;

				case '.':
					//remove cs.output.store(OP_ANY_CHAR, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_ANY_CHAR));
					break;

				case '^':
					if (!cs.beginning_context) {
						if (context_independent_ops()) {
							return ILLEGAL_OPERATOR;
						} else {
							// remove cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
							cs.prec_stack.start(cs.output.store(OP_CHAR, cs.ch));
							break;
						}
					}
				// remove cs.output.store(OP_BEGIN_OF_LINE, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_BEGIN_OF_LINE));
					break;

				case '$':
					if (!(cs.input.at_end() == true || incomplete_eoi(cs))) {
						if (context_independent_ops()) {
							return ILLEGAL_OPERATOR;
						} else {
							// remove cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
							cs.prec_stack.start(cs.output.store(OP_CHAR, cs.ch));
							break;
						}
					}
				// remove cs.output.store(OP_END_OF_LINE, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_END_OF_LINE));
					break;

				case '?':
				case '*':
				case '+':
					if (cs.beginning_context) {
						if (context_independent_ops()) {
							return ILLEGAL_OPERATOR;
						} else {
							// remove cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
							cs.prec_stack.start(cs.output.store(OP_CHAR, cs.ch));
							break;
						}
					}

				// we skip empty expressions for ?, + and * matches
					if (cs.prec_stack.start() == cs.output.offset()) {
						break;
					}

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
						default: break;
					}
					break;

				case '[':
					return cs.output.store_class(cs);

				case TOK_REGISTER:
					if (cs.ch == '0' || !(cs.ch > '0' && cs.ch <= '9')) {
						return ILLEGAL_BACKREFERENCE;
					}
					cs.output.store(OP_BACKREF, cs.ch - '0');
					break;

				case TOK_BACKREF:
					// remove cs.output.store(OP_BACKREF, cs.ch, cs.prec_stack);
					cs.prec_stack.start(cs.output.store(OP_BACKREF, cs.ch));
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

				case '|':
					return cs.output.store_alternate(cs);

				case TOK_ESCAPE:
				default:
					return ILLEGAL_OPERATOR;
			}
			return 0;
		}

		virtual bool incomplete_eoi(re_compile_state_type &cs) const { return false; }

		virtual int translate_plain_op(re_compile_state_type &cs) const { return 0; }

		virtual int translate_escaped_op(re_compile_state_type &cs) const { return 0; }

		virtual int translate_char_class_escaped_op(re_compile_state_type &cs) const {
			const int flag = (cs.cclass_complement == true) ? 1 : 0;
			switch (cs.ch) {
				case 'w':
					cs.output.store(OP_WORD, flag);
					break;

				case 's':
					cs.output.store(OP_SPACE, flag);
					break;

				case 'd':
					cs.output.store(OP_DIGIT, flag);
					break;

				default:
					break;
			}
			return 0;
		}
	};
}
