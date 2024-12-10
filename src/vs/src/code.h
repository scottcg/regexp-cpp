
#pragma once

#include "tokens.h"

namespace re {
	template<class traitsT>
	class compiled_code_vector {
	public:
		typedef traitsT traits_type;
		typedef typename traits_type::char_type char_type;
		typedef typename traits_type::int_type int_type;
		typedef typename traits_type::char_type code_type;
		typedef compile_state<traitsT> compile_state_type;

		compiled_code_vector() {
			initialize();
		}

		~compiled_code_vector() = default;

		code_type operator[](int i) const {
			return _code_vector[i];
		}

		code_type &operator[](int i) {
			if (i > _offset) _offset = i;
			return _code_vector[i];
		}

	private:
		void initialize() {
			_offset = 0;
			std::fill(_code_vector.begin(), _code_vector.end(), 0);
		}

	public:
		int store(code_type t) {
			const auto start = _offset;
			_code_vector.push_back(t);
			_offset++;
			return start;
		}

		int store(code_type op, code_type flag) {
			const auto start = _offset;
			_code_vector.push_back(op);
			_code_vector.push_back(flag);
			_offset += 2;
			return start;
		}

		void put_address(int off, const int addr) {
			const int dsp = addr - off - 2;

#if 0
			std::cout << "put_address " << " count=" << _code_vector.size() << std::endl;
			dump_code(std::cout);
			std::cout << "STACK ADDRESS: (offset:" << off << ", addr:" << addr << ")";
			std::cout << "   dsp:" << dsp
					<< " [" << off << "]:" << static_cast<unsigned int>(dsp & 0xFF)
					<< " [" << off+1 << "]:" << static_cast<unsigned int>((dsp >> 8) & 0xFF) << std::endl;
#endif
			_code_vector[off] = static_cast<code_type>(dsp & 0xFF);
			_code_vector[off + 1] = static_cast<code_type>((dsp >> 8) & 0xFF);
#if 0
			dump_code(std::cout);
			std::cout << " end put_address" << std::endl << std::endl;
#endif
		}

		void store_jump(int opcode_pos, opcodes type, const int to_addr) {
			_code_vector.insert(_code_vector.begin() + opcode_pos, {static_cast<code_type>(type), 0, 0});
			put_address(opcode_pos + 1, to_addr);
			_offset += 3;
		}

		int offset() const {
			return _offset;
		}

		const code_type *code() const {
			return _code_vector.data();
		}

		int store_alternate(compile_state_type &cs) {
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);
			store(OP_GOTO);
			cs.jump_stack.push(offset());
			store(0);
			store(0);
			cs.prec_stack.start(offset());
			return 0;
		}

		int store_class_alternate(compile_state_type &cs) {
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE, offset() + 6);
			store(OP_POP_FAILURE_GOTO);
			cs.jump_stack.push(offset());
			store(0);
			store(0);
			cs.prec_stack.start(offset());
			return 0;
		}

		int store_concatenate(compile_state_type &cs) {
			store_jump(cs.prec_stack.start(), OP_PUSH_FAILURE2, _offset + 4);
			store(OP_FORWARD);
			cs.prec_stack.start(offset());
			store(OP_POP_FAILURE);
			return 0;
		}

		int store_class(compile_state_type &cs) {
			int start_offset = _offset;
			cs.prec_stack.start(_offset);
			cs.prec_stack.current(NUM_LEVELS - 1);
			cs.prec_stack.start(_offset);

			if (cs.input.get(cs.ch)) {
				return -1;
			}

			cs.cclass_complement = false;
			if (cs.ch == '^') {
				cs.cclass_complement = true;
				if (cs.input.get(cs.ch)) {
					return -1;
				}
			}

			bool first_time_thru = true;
			do {
				if (!first_time_thru && !cs.cclass_complement) {
					store_class_alternate(cs);
				} else {
					first_time_thru = false;
				}

				if (cs.ch == '\\') {
					cs.input.get(cs.ch);
					if (cs.syntax.translate_char_class_escaped_op(cs)) {
						return SYNTAX_ERROR;
					}
				} else if (cs.ch == '-' && cs.input.peek() != ']') {
					store((cs.cclass_complement ? OP_NOT_CHAR : OP_CHAR), '-');
				} else {
					if (cs.input.peek() == '-') {
						code_type first_ch = cs.ch;
						if (cs.input.get(cs.ch)) {
							return -1;
						}
						if (cs.input.peek() == ']') {
							cs.input.unget(cs.ch);
							store((cs.cclass_complement ? OP_NOT_CHAR : OP_CHAR), first_ch);
						} else {
							if (cs.input.get(cs.ch)) {
								return -1;
							}
							store((cs.cclass_complement ? OP_NOT_RANGE_CHAR : OP_RANGE_CHAR));
							store(first_ch);
							store(cs.ch);
						}
					} else {
						store((cs.cclass_complement ? OP_NOT_CHAR : OP_CHAR), cs.ch);
					}
				}

				if (cs.cclass_complement) {
					store(OP_BACKUP);
				}

				if (cs.input.get(cs.ch)) {
					return -1;
				}
			} while (cs.ch != ']');

			if (cs.cclass_complement) {
				store_concatenate(cs);
			}

			cs.prec_stack.start(start_offset);
			return 0;
		}

		void store_closure_count(compile_state_type &cs, int pos, const int addr, const int mi, const int mx) {
			constexpr int skip = 7;
			_code_vector.insert(_code_vector.begin() + pos, skip, 0);
			_code_vector[pos++] = static_cast<code_type>(OP_CLOSURE);
			put_address(pos, addr);
			pos += 2;
			put_number(pos, mi);
			pos += 2;
			put_number(pos, mx);
			_offset += skip;

			store_jump(_offset, OP_CLOSURE_INC, cs.prec_stack.start() + 3);
			put_number(_offset, mi);
			_offset += 2;
			put_number(_offset, mx);
			_offset += 2;

			cs.prec_stack.start(_offset); // is this right? look at other that place this earlier
		}

		int store_closure(compile_state_type &cs) {
			int_type ch = 0;
			if (cs.input.get(ch)) {
				return -1;
			}

			int minimum = -1, maximum = -1;
			if (ch == ',') {
				if (cs.input.get(ch)) {
					return -1;
				}
				minimum = 0;
				maximum = cs.input.get_number(ch);
			} else {
				minimum = cs.input.get_number(ch);
				maximum = 0;
				if (ch == ',') {
					if (cs.input.get(ch)) {
						return -1;
					}
					if (ch != '}') {
						maximum = cs.input.get_number(ch);
					}
				} else {
					maximum = minimum;
				}
			}
			if (!(minimum >= 0 && maximum >= 0)) {
				return -1;
			}

			if (ch != '}') {
				return -1;
			}

			store_closure_count(cs, cs.prec_stack.start(), _offset + 10, minimum, maximum);
			return 0;
		}

		static int decode_address_and_advance(const char *&cp) {
			short r = *cp++;
			r |= *cp++ << 8;
			return r;
		}

		static int decode_address_and_advance(const wchar_t *&cp) {
			wint_t r = *cp++;
			r |= *cp++ << 8;
			return r;
		}

		void dump_code(std::ostream &out) const {
			auto code = *this;
			const char_type *cp = code.code();
			int pos = 0;
			while ((pos = cp - code.code()) < code.offset()) {
				out << '\t' << pos << '\t';

				switch (*cp++) {
					case OP_END:
						out << "OP_END" << std::endl;
						break;

					case OP_BACKUP:
						out << "OP_BACKUP" << std::endl;
						break;

					case OP_FORWARD:
						out << "OP_FORWARD" << std::endl;
						break;

					case OP_BEGIN_OF_LINE:
						out << "OP_BEGIN_OF_LINE" << std::endl;
						break;

					case OP_BEGIN_OF_BUFFER:
					case OP_END_OF_BUFFER:
						out << "OP_BEGIN_OF_BUFFER/OP_END_OF_BUFFER" << std::endl;
						break;

					case OP_BEGIN_OF_WORD:
						out << "OP_BEGIN_OF_WORD" << std::endl;
						break;

					case OP_END_OF_WORD:
						out << "OP_END_OF_WORD" << std::endl;
						break;

					case OP_WORD:
						out << "OP_WORD (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_WORD_BOUNDARY:
						out << "OP_WORD_BOUNDARY (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_END_OF_LINE:
						out << "OP_END_OF_LINE" << std::endl;
						break;

					case OP_CHAR:
						out << "OP_CHAR (" << static_cast<char>(*cp++) << ")\n";
						break;

					case OP_NOT_CHAR:
						out << "OP_NOT_CHAR (" << static_cast<char>(*cp++) << ")\n";
						break;

					case OP_ANY_CHAR:
						out << "ANY_CHAR" << std::endl;
						break;

					case OP_RANGE_CHAR:
						out << "OP_RANGE_CHAR (" << static_cast<char>(*cp++);
						out << ',' << static_cast<char>(*cp++) << ")\n";
						break;

					case OP_NOT_RANGE_CHAR:
						out << "OP_NOT_RANGE_CHAR (" << static_cast<char>(*cp++);
						out << ',' << static_cast<char>(*cp++) << ")\n";
						break;

					case OP_BACKREF_BEGIN:
						out << "OP_BACKREF_BEGIN (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_BACKREF_END:
						out << "OP_BACKREF_END (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_BACKREF:
						out << "OP_BACKREF (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_BACKREF_FAIL:
						out << "OP_BACKREF_FAIL" << std::endl;
						break;

					case OP_CLOSURE:
						out << "OP_CLOSURE (" << decode_address_and_advance(cp) << ") ";
						out << "{" << decode_address_and_advance(cp) << ",";
						out << decode_address_and_advance(cp) << "}" << std::endl;
						break;

					case OP_CLOSURE_INC:
						out << "OP_CLOSURE_INC (" << decode_address_and_advance(cp) << ")";
						out << " {" << decode_address_and_advance(cp) << ",";
						out << decode_address_and_advance(cp) << "}" << std::endl;
						break;

					case OP_TEST_CLOSURE:
						out << "OP_CLOSURE_TEST" << std::endl;
						break;

					case OP_DIGIT:
						out << "OP_DIGIT (" << static_cast<int>(*cp++) << ")" << std::endl;
						break;

					case OP_SPACE:
						out << "OP_SPACE (" << static_cast<int>(*cp++) << ")\n";
						break;

					case OP_GOTO:
						out << "OP_GOTO (" << pos << ", " << decode_address_and_advance(cp) << ")\n";
						break;

					case OP_POP_FAILURE_GOTO:
						out << "OP_POP_FAILURE_GOTO (" << pos << ", " << decode_address_and_advance(cp) << ")\n";
						break;

					case OP_FAKE_FAILURE_GOTO:
						out << "OP_FAKE_FAILURE_GOTO (" << pos << ", " << decode_address_and_advance(cp) << ")\n";
						break;

					case OP_PUSH_FAILURE:
						out << "OP_PUSH_FAILURE (" << pos << ", " << decode_address_and_advance(cp) << ")\n";
						break;

					case OP_PUSH_FAILURE2:
						out << "OP_PUSH_FAILURE2 (" << pos << ", " << decode_address_and_advance(cp) << ")\n";
						break;

					case OP_POP_FAILURE:
						out << "OP_POP_FAILURE" << std::endl;
						break;

					case OP_NOOP:
						out << "OP_NOOP" << std::endl;
						break;

					default:
						out << "BAD CASE" << std::endl;
						break;
				}
			}
		}

	private:
		void put_number(int pos, const int n) {
			_code_vector[pos++] = static_cast<code_type>(n & 0xFF);
			_code_vector[pos] = static_cast<code_type>((n >> 8) & 0xFF);
		}

	public:
		std::vector<code_type> _code_vector;
		int _offset = 0;
		int _character_class_org = 0;
	};
}
