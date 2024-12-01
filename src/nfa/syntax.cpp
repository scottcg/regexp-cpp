// $Header: /regexp/syntax.cpp 1     2/22/97 6:04p Scg $

#include <assert.h>
#include <string>
// #include <stdio.h>
#include <memory.h>

#include <vector>
#include <stack>

#include "syntax.h"
#include "regexp.h"

/////////////////////////////////////////////////////////////////////////////////////
// syntax_base implementation
//

template <class strT>
syntax_base<strT>::syntax_base() {}

template <class strT>
syntax_base<strT>::~syntax_base() {}

template <class strT>
bool syntax_base<strT>::context_independent_ops() const {
	return true;
}

template <class strT>
int syntax_base<strT>::precedence(int op) const {
	return 0;
}

template <class strT>
int syntax_base<strT>::translate_plain_op(compile_state_type& cs) const {
	return 0;
}

template <class strT>
int syntax_base<strT>::translate_escaped_op(compile_state_type& cs) const {
	return 0;
}

template <class strT>
int syntax_base<strT>::compile(compile_state_type& cs) const {
	cs.op = !TOK_END;
    while ( cs.op != TOK_END ) {
		if ( cs.input.at_end() == true ) {
			cs.op = TOK_END;
        } else {
			cs.input.get(cs.ch);
			cs.op = cs.syntax.translate_plain_op(cs);
			if ( cs.op == TOK_ESCAPE ) {
				cs.input.get(cs.ch);
				cs.op = cs.syntax.translate_escaped_op(cs);
				if ( cs.op == TOK_CTRL_CHAR ) {
					cs.op = TOK_CHAR;
					if ( cs.input.translate_ctrl_char(cs.ch) ) {
						return SYNTAX_ERROR;
					}
				}
			}
        }

		// propagate the current character offset to the higher level (precedence)
		// or clean up and pending jumps for lower precedence operators.

		int level = cs.syntax.precedence(cs.op);
		if ( level > cs.prec_stack.current() ) {
			int c = cs.prec_stack.current();
			for ( cs.prec_stack.current(c++); cs.prec_stack.current() < level;
					cs.prec_stack.current(c++) ) {
				cs.prec_stack.start(cs.output.offset());
			}
			cs.prec_stack.start(cs.output.offset());
		} else if ( level < cs.prec_stack.current() ) {
			cs.prec_stack.current(level);
			while ( cs.jump_stack.size() ) {
				if ( cs.jump_stack.top() >= cs.prec_stack.start() ) {
					cs.output.put_address(cs.jump_stack.top(), cs.output.offset());
					cs.jump_stack.pop();
				} else {
					break;
				}
			}
		}

		// finally, we are ready to compile a single operation.
        int err = cs.syntax.compile_opcode(cs);
		if ( err ) {	// syntax error someplace
			return err;
		}
        cs.beginning_context = (cs.op == '(' || cs.op == '|');
	}
	return 0;
}

template <class strT>
int syntax_base<strT>::compile_opcode(compile_state_type& cs) const {
	switch ( cs.op ) {
	case TOK_END:
	  cs.output.store(OP_END, cs.prec_stack);
	  break;

	case TOK_CHAR:
	  cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
	  break;

	case '.':
	  cs.output.store(OP_ANY_CHAR, cs.prec_stack);
	  break;

	case '^':
	  if ( !cs.beginning_context ) {
		  if ( context_independent_ops() ) {
			  return ILLEGAL_OPERATOR;
		  } else {
			  cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
			  break;
		  }
	  }
	  cs.output.store(OP_BEGIN_OF_LINE, cs.prec_stack);
	  break;

	case '$':
	  if ( !(cs.input.at_end() == true || incomplete_eoi(cs)) ) {
		  if ( context_independent_ops() ) {
			  return ILLEGAL_OPERATOR;
		  } else {
			  cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
			  break;
		  }
	  }
	  cs.output.store(OP_END_OF_LINE, cs.prec_stack);
	  break;

	case '?':
	case '*':
	case '+':		  
	  if ( cs.beginning_context ) {
		  if ( context_independent_ops() ) {
			  return ILLEGAL_OPERATOR;
		  } else {
			  cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
			  break;
		  }
	  }
	  
	  // we skip empty expressions for ?, + and * matches
	  if ( cs.prec_stack.start() == cs.output.offset() ) {
		  break;
	  }

	  switch ( cs.op ) {
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

		  if ( cs.op == '+' ) {	// jump over initial failure_jump
			  cs.output.store_jump(cs.prec_stack.start(), OP_FAKE_FAILURE_GOTO,
				  cs.prec_stack.start() + 6);
		  }
		  break;
	  }
	  break;

	case '[':
		return cs.output.store_class(cs);

	case TOK_REGISTER:
		if ( cs.ch == '0' || !(cs.ch > '0' && cs.ch <= '9') ) {
			return ILLEGAL_BACKREFERENCE;
		}
		cs.output.store(OP_BACKREF, cs.ch - '0');
		break;

	case TOK_BACKREF:
		cs.output.store(OP_BACKREF, cs.ch, cs.prec_stack);
		break;

	case '(':
		cs.number_of_backrefs++;
		cs.parenthesis_nesting++;

		cs.prec_stack.start(cs.output.offset());

		cs.output.store(OP_BACKREF_BEGIN, cs.next_backref);
		cs.backref_stack.push(cs.next_backref++);

		cs.prec_stack.push(precedence_element());
		cs.prec_stack.current(0);
		cs.prec_stack.start(cs.output.offset());
		break;

	case ')':
		if ( cs.parenthesis_nesting <= 0 ) {
			return MISMATCHED_PARENTHESIS;
		}
		cs.parenthesis_nesting--;

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

template <class strT>
bool syntax_base<strT>::incomplete_eoi(compile_state_type&) const {
	return false;
}

template <class strT>
int syntax_base<strT>::translate_cclass_escaped_op(compile_state_type& cs) const {
	const int flag = (cs.cclass_complement == true) ? 1 : 0;
	switch ( cs.ch ) {
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

/////////////////////////////////////////////////////////////////////////////////////

template <class strT>
bool generic_syntax<charT, intT>::context_independent_ops() const { return 1; }

template <class strT>
int generic_syntax<charT, intT>::translate_plain_op(compile_state_type& cs) const {
	switch ( cs.ch ) {
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

template <class strT>
int generic_syntax<charT, intT>::translate_escaped_op(compile_state_type&) const {
	return TOK_CHAR;
}

template <class strT>
int generic_syntax<charT, intT>::precedence(int op) const {
	switch ( op ) {
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

template <class strT>
bool grep_syntax<charT, intT>::context_independent_ops() const { return 0; }

// grep allows threes different escaped operators, all have to do 
// with backreferences (or tagged expressions in grep lingo). 
//	\( start of backref
//	\) end of backref
//	\digit refer to a backref -- should > 0 && < number of tags
// anything else is just a character match.

template <class strT>
int grep_syntax<charT, intT>::translate_escaped_op(compile_state_type& cs) const {
	if ( is_a_digit(cs.ch) ) {
		intT value = 0;
		int n_digits = cs.input.peek_number(value);

		if ( cs.number_of_backrefs && value <= cs.number_of_backrefs && value > 0 ) {
			cs.input.advance(n_digits - 1);
			cs.ch= value;
			return TOK_BACKREF;
		} else {
			return SYNTAX_ERROR;
		}
	}

	switch ( cs.ch ) {
	case '(':
	case ')':
		return cs.ch;

	default:
		return TOK_CHAR;
	}
}

// check for "\)" as in expressions like "\(^foo$\)"; i need to check to 
// make sure that this is legal grep syntax.

template <class strT>
bool grep_syntax<charT, intT>::incomplete_eoi(compile_state_type& cs) const {
	assert(cs.input.at_end() == false);
	intT ch = 0;
	cs.input.get(ch);
	if ( ch == '\\' && cs.input.at_end() == false ) {
		cs.input.get(ch);
		cs.input.unget();
		cs.input.unget();
		if ( ch == ')' ) {
			return true;
		}
	} else {
		cs.input.unget();
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////


template <class strT>
bool egrep_syntax<charT, intT>::context_independent_ops() const { return 0; }

template <class strT>
bool egrep_syntax<charT, intT>::incomplete_eoi(compile_state_type& cs) const {
	// check for '|' or ')' (note: ')' is not a register).
	assert(cs.input.at_end() == false);
	intT ch = 0;
	cs.input.get(ch);
	cs.input.unget();
	
	return (ch == '|' || ch == ')') ? true : false;
	return false;
}

template <class strT>
int egrep_syntax<charT, intT>::translate_plain_op(compile_state_type& cs) const {
	switch ( cs.ch ) {
	case '(':
	case ')':
	case '+':
	case '?':
	case '|':
		return cs.ch;

	default:
		return generic_syntax<charT, intT>::translate_plain_op(cs);
	}
}

template <class strT>
int egrep_syntax<charT, intT>::translate_escaped_op(compile_state_type& cs) const {
	switch ( cs.ch ) {
	case 'n':
	case 'f':
	case 'b':
	case 'r':
	case 't':
		return TOK_CTRL_CHAR;

	default:
		return generic_syntax<charT, intT>::translate_escaped_op(cs);
	}
	return TOK_CHAR;
}

/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////


template <class strT>
int perl_syntax<charT, intT>::translate_plain_op(compile_state_type& cs) const {
	switch ( cs.ch ) {
	case '{':
	case '}':
		return cs.ch;

	default:
		return awk_syntax<charT, intT>::translate_plain_op(cs);
	}
}

// perl has a bunch of escaped ops; the most complicated is the 
// backreferences, since depending upon the context a what looks like
// a backref could be a constant (like \33 == \033). 
// the rest of the escaped ops are really short cuts for something that
// could be specified by hand (like \d = [0-9]).

template <class strT>
int perl_syntax<charT, intT>::translate_escaped_op(compile_state_type& cs) const {
	if ( is_a_digit(cs.ch) ) {
		intT value = 0;
		int n_digits = cs.input.peek_number(value);

		if ( value != 0 && value <= cs.number_of_backrefs ) {
			cs.ch = value;
			return TOK_BACKREF;
		} else {
			// should be a constant, like an octal number? /0xx or /0xxxxx
			cs.input.advance(n_digits - 1);
			cs.ch = value;
			return TOK_CHAR;
		}
	}

	switch ( cs.ch ) {
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
		return awk_syntax<charT, intT>::translate_escaped_op(cs);
	}
}

template <class strT>
bool perl_syntax<charT, intT>::incomplete_eoi(compile_state_type& cs) const {
	assert(cs.input.at_end() == false);
	intT ch = 0;
	cs.input.get(ch);
	cs.input.unget();

	return (ch == '|' || ch == ')') ? true : false;
}

template <class strT>
int perl_syntax<charT, intT>::compile_opcode(compile_state_type& cs) const {
	switch ( cs.op ) {
	case '?':
	case '*':
	case '+':		  
	  if ( cs.beginning_context ) {
		  if ( context_independent_ops() ) {
			  return ILLEGAL_OPERATOR;
		  } else {
			  cs.output.store(OP_CHAR, cs.ch, cs.prec_stack);
			  break;
		  }
	  }
	  
	  // we skip empty expressions for ?, + and * matches
	  if ( cs.prec_stack.start() == cs.output.offset() ) {
		  break;
	  }

	  if ( !cs.input.at_end() && cs.input.peek() == '?' ) { // stingy versions
		  intT t;
		  cs.input.get(t);	// consume the '?'

		  switch ( cs.op ) {
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
	  } else {												// greedy versions
		  switch ( cs.op ) {
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

			  if ( cs.op == '+' ) {		// jump over initial failure_jump
				  cs.output.store_jump(cs.prec_stack.start(), OP_FAKE_FAILURE_GOTO,
					  cs.prec_stack.start() + 6);
			  }
			  break;
		  }
	  }
	  break;

	case '(':
		cs.number_of_backrefs++;
		cs.parenthesis_nesting++;

		cs.prec_stack.start(cs.output.offset());

		cs.output.store(OP_BACKREF_BEGIN, cs.next_backref);
		cs.backref_stack.push(cs.next_backref++);

		cs.prec_stack.push(precedence_element());
		cs.prec_stack.current(0);
		cs.prec_stack.start(cs.output.offset());
		break;

	case ')':
		if ( cs.parenthesis_nesting <= 0 ) {
			return MISMATCHED_PARENTHESIS;
		}
		cs.parenthesis_nesting--;

		assert(cs.prec_stack.size() > 1);
		cs.prec_stack.pop();
		cs.prec_stack.current(precedence('('));

		cs.output.store(OP_BACKREF_END, cs.backref_stack.top());
		cs.backref_stack.pop();
		break;

	case TOK_EXT_REGISTER:
		if ( cs.input.get_number(cs.ch) || cs.ch <= 0 || cs.ch >= MAX_BACKREFS ) {
			return BACKREFERENCE_OVERFLOW;
		}
		cs.output.store(OP_BACKREF, cs.ch, cs.prec_stack);
		break;

	case '{':
		return cs.output.store_closure(cs);
		break;

	case 'b':
		cs.output.store(OP_WORD_BOUNDARY, 0, cs.prec_stack);
		break;

	case 'B':
		cs.output.store(OP_WORD_BOUNDARY, 1, cs.prec_stack);
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
		return awk_syntax<charT, intT>::compile_opcode(cs);
	}
	return 0;
}

template <class strT>
int perl_syntax<charT, intT>::translate_cclass_escaped_op(compile_state_type& cs) const {
	if ( is_a_digit(cs.ch) ) {
		intT value = 0;
		int n_digits = cs.input.peek_number(value);

		if ( value != 0 && value <= cs.number_of_backrefs ) { // backref? ok here?
			cs.output.store(OP_BIN_CHAR, value);
		} else {
			// should be a constant, like an octal number? /0xx or /0xxxxx
			cs.input.advance(n_digits - 1);
			cs.output.store(OP_BIN_CHAR, value);
		}
		return 0;
	}
	
	const int flag = (cs.cclass_complement == true) ? 1 : 0;
	switch ( cs.ch ) {
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
		return awk_syntax<charT, intT>::translate_cclass_escaped_op(cs);
	}
	return 0;
}
