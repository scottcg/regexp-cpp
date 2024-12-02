
#pragma once
/*
todo: potentially other syntaxes
lex, sed, python i kinda like lex since it has that neat precedence with
^this|that$ == ^(this|that)$
*/

#include "tokens.h"
#include "precedence.h"
#include "compile.h"

namespace re {
	template<class traitsType> class compiled_code_vector;

	template<class traitsType>
	class re_syntax_base {
	public:
		virtual ~re_syntax_base() = default;

		typedef traitsType traits_type;
		typedef typename traits_type::char_type char_type;
		typedef typename traits_type::int_type int_type;
		typedef re_compile_state<traitsType> re_compile_state_type;

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

				int err = cs.syntax.compile_opcode(cs); // finally, compile a single opcode.
				if (err) {
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

		virtual int translate_cclass_escaped_op(re_compile_state_type &cs) const {
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


	///////////////////////////////////////////////////////////////////////////////////////////
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
	class generic_syntax : public re_syntax_base<traitsType> {
	public:
		typedef re_compile_state<traitsType> re_compile_state_type;

		bool context_independent_ops() const override;

		int translate_plain_op(re_compile_state_type &) const override;

		int translate_escaped_op(re_compile_state_type &) const override;

		int precedence(int op) const override;
	};

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
	class grep_syntax : public generic_syntax<traitsType> {
	public:
		typedef re_compile_state<traitsType> re_compile_state_type;

		bool context_independent_ops() const override;

		bool incomplete_eoi(re_compile_state_type &cs) const override;

		int translate_escaped_op(re_compile_state_type &cs) const override;
	};


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
	class egrep_syntax : public generic_syntax<traitsType> {
	public:
		typedef re_compile_state<traitsType> re_compile_state_type;

	public:
		bool context_independent_ops() const override;

		bool incomplete_eoi(re_compile_state_type &cs) const override;

		int translate_plain_op(re_compile_state_type &cs) const override;

		int translate_escaped_op(re_compile_state_type &cs) const override;
	};

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
	class perl_syntax : public egrep_syntax<traitsType> {
	public:
		typedef re_compile_state<traitsType> re_compile_state_type;

	public:
		int precedence(int op) const override {
			return generic_syntax<traitsType>::precedence(op);
		}

		bool context_independent_ops() const override;

		bool incomplete_eoi(re_compile_state_type &cs) const override;

		int compile_opcode(re_compile_state_type &cs) const override;

		int translate_plain_op(re_compile_state_type &cs) const override;

		int translate_escaped_op(re_compile_state_type &cs) const override;

		int translate_cclass_escaped_op(re_compile_state_type &cs) const override;
	};


	/////////////////////////////////////////////////////////////////////////////////////

	template<class traitsT>
	bool generic_syntax<traitsT>::context_independent_ops() const { return 1; }

	template<class traitsT>
	int generic_syntax<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
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
	int generic_syntax<traitsT>::translate_escaped_op(re_compile_state_type &) const {
		return TOK_CHAR;
	}

	template<class traitsT>
	int generic_syntax<traitsT>::precedence(int op) const {
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

	/////////////////////////////////////////////////////////////////////////////////////

	template<class traitsT>
	bool grep_syntax<traitsT>::context_independent_ops() const { return false; }

	// grep allows threes different escaped operators, all have to do
	// with backreferences (or tagged expressions in grep lingo).
	//	\( start of backref
	//	\) end of backref
	//	\digit refer to a backref -- should > 0 && < number of tags
	// anything else is just a character match.

	template<class traitsT>
	int grep_syntax<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
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
	bool grep_syntax<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
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

	/////////////////////////////////////////////////////////////////////////////////////


	template<class traitsT>
	bool egrep_syntax<traitsT>::context_independent_ops() const { return false; }

	template<class traitsT>
	bool egrep_syntax<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
		// check for '|' or ')' (note: ')' is not a register).
		assert(cs.input.at_end() == false);
		typename traitsT::int_type ch = 0;
		cs.input.get(ch);
		cs.input.unget();

		return (ch == '|' || ch == ')') ? true : false;
		return false;
	}

	template<class traitsT>
	int egrep_syntax<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
		switch (cs.ch) {
			case '(':
			case ')':
			case '+':
			case '?':
			case '|':
				return cs.ch;
			default:
				return generic_syntax<traitsT>::translate_plain_op(cs);
		}
	}

	template<class traitsT>
	int egrep_syntax<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
		switch (cs.ch) {
			case 'n':
			case 'f':
			case 'b':
			case 'r':
			case 't':
				return TOK_CTRL_CHAR;
			default:
				return generic_syntax<traitsT>::translate_escaped_op(cs);
		}
		return TOK_CHAR;
	}

	/////////////////////////////////////////////////////////////////////////////////////

	template<class traitsT>
	bool perl_syntax<traitsT>::context_independent_ops() const { return false; }

	template<class traitsT>
	int perl_syntax<traitsT>::translate_plain_op(re_compile_state_type &cs) const {
		switch (cs.ch) {
			case '{':
			case '}':
				return cs.ch;

			default:
				return egrep_syntax<traitsT>::translate_plain_op(cs);
		}
	}

	// perl has a bunch of escaped ops; the most complicated is the
	// backreferences, since depending upon the context a what looks like
	// a backref could be a constant (like \33 == \033).
	// the rest of the escaped ops are really short cuts for something that
	// could be specified by hand (like \d = [0-9]).

	template<class traitsT>
	int perl_syntax<traitsT>::translate_escaped_op(re_compile_state_type &cs) const {
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
				return egrep_syntax<traitsT>::translate_escaped_op(cs);
		}
	}

	template<class traitsT>
	bool perl_syntax<traitsT>::incomplete_eoi(re_compile_state_type &cs) const {
		assert(cs.input.at_end() == false);
		typename traitsT::int_type ch = 0;
		cs.input.get(ch);
		cs.input.unget();

		return (ch == '|' || ch == ')') ? true : false;
	}

	template<class traitsT>
	int perl_syntax<traitsT>::compile_opcode(re_compile_state_type &cs) const {
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
				return egrep_syntax<traitsT>::compile_opcode(cs);
		}
		return 0;
	}

	template<class traitsT>
	int perl_syntax<traitsT>::translate_cclass_escaped_op(re_compile_state_type &cs) const {
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
				return egrep_syntax<traitsT>::translate_cclass_escaped_op(cs);
		}
		return 0;
	}
}
