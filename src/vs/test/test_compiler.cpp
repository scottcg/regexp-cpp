#include <gtest/gtest.h>
#include "tokenizer.h"

#include <vector>
using namespace std;

enum class instruction_type {
    MATCH_CHAR,
    MATCH_CLASS,
    MATCH_NEGATED_CLASS,
    MATCH_META,
    SPLIT,
    JUMP,
    ACCEPT
};

struct instruction {
    instruction_type type;
    std::string value;  // For literals, character classes, etc.
    size_t next1;       // Primary transition state
    size_t next2;       // Secondary state (used in SPLIT)

    std::string convert_instruction_type_to_string() const {
        switch (type) {
            case instruction_type::MATCH_CHAR: return "MATCH_CHAR";
            case instruction_type::MATCH_CLASS: return "MATCH_CLASS";
            case instruction_type::MATCH_NEGATED_CLASS: return "MATCH_NEGATED_CLASS";
            case instruction_type::MATCH_META: return "MATCH_META";
            case instruction_type::SPLIT: return "SPLIT";
            case instruction_type::JUMP: return "JUMP";
            case instruction_type::ACCEPT: return "ACCEPT";
            default: return "UNKNOWN";
        }
    }
};

class compiler {
public:
    explicit compiler(const std::vector<token>& tokens) : tokens(tokens), pos(0) {}

    std::vector<instruction> compile() {
        instructions.clear();
        size_t start_anchor = handle_anchor("^");
        parse_expression();
        size_t end_anchor = handle_anchor("$");
        instructions.emplace_back(instruction_type::ACCEPT, "", 0, 0);
        return instructions;
    }

private:
    const std::vector<token>& tokens;
    std::vector<instruction> instructions;
    size_t pos;

    void parse_expression() {
        size_t last_index = instructions.size();
        while (pos < tokens.size() && tokens[pos].type != token_type::group_close) {
            switch (tokens[pos].type) {
                case token_type::literal:
                    add_instruction(instruction_type::MATCH_CHAR, tokens[pos].value);
                    ++pos;
                    break;

                case token_type::character_class_open:
                    parse_character_class();
                    break;

                case token_type::negated_class_open:
                    parse_negated_class();
                    break;

                case token_type::quantifier:
                    handle_quantifier(last_index);
                    break;

                case token_type::alternation:
                    handle_alternation(last_index);
                    break;

                case token_type::group_open:
                    ++pos; // Consume '('
                    parse_expression();
                    if (tokens[pos].type == token_type::group_close) ++pos;
                    break;

                default:
                    ++pos;
            }
            last_index = instructions.size() - 1;
        }
    }

    size_t handle_anchor(const std::string& anchor_value) {
        if (pos < tokens.size() && tokens[pos].type == token_type::meta_character && tokens[pos].value == anchor_value) {
            size_t idx = add_instruction(instruction_type::MATCH_META, anchor_value);
            ++pos;
            return idx;
        }
        return instructions.size();
    }

    void parse_character_class() {
        std::string class_content;
        ++pos;
        while (tokens[pos].type != token_type::character_class_close) {
            class_content += tokens[pos].value;
            ++pos;
        }
        ++pos;
        add_instruction(instruction_type::MATCH_CLASS, class_content);
    }

    void parse_negated_class() {
        std::string class_content;
        ++pos;
        while (tokens[pos].type != token_type::character_class_close) {
            class_content += tokens[pos].value;
            ++pos;
        }
        ++pos;
        add_instruction(instruction_type::MATCH_NEGATED_CLASS, class_content);
    }

    void handle_quantifier(size_t target_index) {
        if (tokens[pos].value == "*") {
            size_t split_index = add_instruction(instruction_type::SPLIT, "", target_index, instructions.size() + 1);
            add_instruction(instruction_type::JUMP, "", split_index);
        } else if (tokens[pos].value == "+") {
            add_instruction(instruction_type::SPLIT, "", target_index, instructions.size() + 1);
        } else if (tokens[pos].value == "?") {
            add_instruction(instruction_type::SPLIT, "", instructions.size() + 1, target_index);
        }
        ++pos;
    }

    void handle_alternation(size_t last_index) {
        size_t split_index = add_instruction(instruction_type::SPLIT, "", last_index + 1, 0);
        ++pos; // Consume '|'
        parse_expression();
        instructions[split_index].next2 = instructions.size();
    }

    size_t add_instruction(instruction_type type, const std::string& value = "", size_t next1 = 0, size_t next2 = 0) {
        instructions.emplace_back(type, value, next1, next2);
        return instructions.size() - 1;
    }
};





std::pair<size_t, std::string> find_first_difference(const std::vector<token> &actual,
                                                     const std::vector<token> &expected) {
    const size_t min_size = std::min(actual.size(), expected.size());
    for (size_t i = 0; i < min_size; ++i) {
        if (actual[i].type != expected[i].type || actual[i].value != expected[i].value) {
            return {
                i,
                "Difference at index " + std::to_string(i) + ": actual(" + token_type_to_string(actual[i].type) + ", " +
                actual[i].value + ") vs expected(" + token_type_to_string(expected[i].type) + ", " + expected[i].value +
                ")"
            };
        }
    }
    return {std::string::npos, "No differences found"};
}

// Helper function to compare token outputs
bool compare_tokens(const std::vector<token> &actual, const std::vector<token> &expected) {
    if (actual.size() != expected.size()) {
        cout << "ERROR SIZE actual:" << actual.size() << " expected:" << expected.size() << endl;
        cout << "difference: " << find_first_difference(actual, expected).second << endl;
        return false;
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].type != expected[i].type || actual[i].value != expected[i].value) {
            return false;
        }
    }
    return true;
}

void print_instructions(const std::vector<instruction>& instructions) {
    for (const auto& instr : instructions) {
        std::cout << "{ type=" << instr.convert_instruction_type_to_string()
                  << ", value=" << instr.value
                  << ", next1=" << instr.next1
                  << ", next2=" << instr.next2 << " }\n";
    }
}

// Helper function to compare instructions
bool compare_instructions(const std::vector<instruction>& actual, const std::vector<instruction>& expected) {
    if (actual.size() != expected.size()) {
        std::cerr << "Instruction size mismatch: actual=" << actual.size()
                  << ", expected=" << expected.size() << std::endl;
        return false;
    }
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].type != expected[i].type || actual[i].value != expected[i].value ||
            actual[i].next1 != expected[i].next1 || actual[i].next2 != expected[i].next2) {
            std::cerr << "Instruction mismatch at index " << i << ":\n";
            std::cerr << "Actual: {type=" << static_cast<int>(actual[i].type)
                      << ", value=" << actual[i].value
                      << ", next1=" << actual[i].next1
                      << ", next2=" << actual[i].next2 << "}\n";
            std::cerr << "Expected: {type=" << static_cast<int>(expected[i].type)
                      << ", value=" << expected[i].value
                      << ", next1=" << expected[i].next1
                      << ", next2=" << expected[i].next2 << "}\n";
            return false;
        }
    }
    return true;
}

// Test Case: "^abc+$"
TEST(compiler_tests, compile_start_anchor_literals_quantifier_end_anchor) {
    std::string regex = R"(^abc+$)";
    tokenizer t(regex);
    auto tokens = t.tokenize();
    print_tokens(tokens);

    compiler c(tokens);
    auto instructions = c.compile();
    std::cout << "instructions" << std::endl;
    print_instructions(instructions);

    // Expected instruction set for "^abc+$"
    std::vector<instruction> expected = {
        {instruction_type::MATCH_META, "^", 1, 0},
        {instruction_type::MATCH_CHAR, "a", 2, 0},
        {instruction_type::MATCH_CHAR, "b", 3, 0},
        {instruction_type::MATCH_CHAR, "c", 4, 0},
        {instruction_type::SPLIT, "", 3, 5},
        {instruction_type::MATCH_META, "$", 6, 0},
        {instruction_type::ACCEPT, "", 0, 0}
    };

    EXPECT_TRUE(compare_instructions(instructions, expected));
}

// Main for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
