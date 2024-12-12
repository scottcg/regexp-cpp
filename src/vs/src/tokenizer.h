#pragma once

//#include <iostream>
#include <vector>
#include <string>
#include <cctype>


// Token types
enum class token_type {
    literal,                // Literal character
    escape,                 // Escape sequences like \d, \w
    meta_character,         // Special characters like ., ^, $
    quantifier,             // *, +, ?, {n,m}
    group_open,             // (
    group_close,            // )
    character_class_open,   // [
    character_class_close,  // ]
    negated_class_open,     // [^
    alternation,            // |
    end_of_input            // End of input
};

// Token structure
struct token {
    token_type type;
    std::string value;

    token(token_type t, const std::string& v = "") : type(t), value(v) {}
};

// Tokenizer class
class tokenizer {
public:
    explicit tokenizer(const std::string& regex) : input(regex), pos(0) {}

    std::vector<token> tokenize() {
    std::vector<token> tokens;

    while (pos < input.size()) {
        char current = input[pos];

        if (current == '\\') {
            // Handle escape sequences, including special cases for \[ and \]
            std::string escape_value = read_escape();

            if (escape_value == "[") {
                tokens.emplace_back(token_type::character_class_open, "[");
            } else if (escape_value == "]") {
                tokens.emplace_back(token_type::character_class_close, "]");
            } else {
                tokens.emplace_back(token_type::escape, escape_value);
            }
            continue;
        }

        if (current == '[') {
            // Handle character class opening
            ++pos;
            if (pos < input.size() && input[pos] == '^') {
                tokens.emplace_back(token_type::negated_class_open, "[^");
                ++pos;
            } else {
                tokens.emplace_back(token_type::character_class_open, "[");
            }
            continue;
        }

        if (current == ']') {
            // Handle character class closing
            tokens.emplace_back(token_type::character_class_close, "]");
            ++pos;
            continue;
        }

        if (current == '(') {
            tokens.emplace_back(token_type::group_open, "(");
            ++pos;
            continue;
        }

        if (current == ')') {
            tokens.emplace_back(token_type::group_close, ")");
            ++pos;
            continue;
        }

        if (is_quantifier_start(current)) {
            // Read quantifiers like *, +, ?, {n,m}, and handle trailing +
            tokens.emplace_back(read_quantifier());
            continue;
        }

        if (current == '|') {
            tokens.emplace_back(token_type::alternation, "|");
            ++pos;
            continue;
        }

        if (is_meta_character(current)) {
            tokens.emplace_back(token_type::meta_character, std::string(1, current));
            ++pos;
            continue;
        }

        // Default case: treat as a literal
        tokens.emplace_back(token_type::literal, std::string(1, current));
        ++pos;
    }

    // Add the end-of-input marker
    tokens.emplace_back(token_type::end_of_input, "");
    return tokens;
}

private:
    std::string input;
    size_t pos;

    bool is_meta_character(char c) const {
        return c == '.' || c == '^' || c == '$';
    }

    bool is_quantifier_start(char c) const {
        return c == '*' || c == '+' || c == '?' || c == '{';
    }

    std::string read_escape() {
        std::string result = "\\";
        ++pos;

        if (pos < input.size()) {
            char escaped = input[pos];
            ++pos;

            // Return just the escaped character
            if (escaped == '[' || escaped == ']') {
                return std::string(1, escaped);
            }

            result += escaped;
        }
        return result;
    }

    token read_character_class() {
        std::string result = "[";
        ++pos;

        if (pos < input.size() && input[pos] == '^') {
            result += "^";
            ++pos;
            return token(token_type::negated_class_open, result);
        }

        return token(token_type::character_class_open, result);
    }

    token read_quantifier() {
        std::string result;
        result += input[pos];
        ++pos;

        // Handle multi-character quantifiers like {2,}
        if (result == "{" && pos < input.size()) {
            while (pos < input.size() && input[pos] != '}') {
                result += input[pos];
                ++pos;
            }
            if (pos < input.size() && input[pos] == '}') {
                result += '}';
                ++pos;
            }
        }

        // Check for a trailing '+' after the quantifier
        if (pos < input.size() && input[pos] == '+') {
            result += "+";
            ++pos;
        }

        return token(token_type::quantifier, result);
    }
};


// Helper: convert token type to string
inline std::string token_type_to_string(token_type type) {
    switch (type) {
        case token_type::literal: return "literal";
        case token_type::escape: return "escape";
        case token_type::meta_character: return "meta_character";
        case token_type::quantifier: return "quantifier";
        case token_type::group_open: return "group_open";
        case token_type::group_close: return "group_close";
        case token_type::character_class_open: return "character_class_open";
        case token_type::negated_class_open: return "negated_class_open";
        case token_type::character_class_close: return "character_class_close";
        case token_type::alternation: return "alternation";
        case token_type::end_of_input: return "end_of_input";
        default: return "unknown";
    }
}

// Helper: print tokens for debugging
inline void print_tokens(const std::vector<token>& tokens) {
    for (const auto& t : tokens) {
        //std::cout << "token type: " << token_type_to_string(t.type) << ", value: '" << t.value << "'\n";
        auto value = t.value.starts_with("\\") ? "\\" + t.value : t.value;
        std::cout << "{ token_type::" << token_type_to_string(t.type) << ", \"" << value << "\"}," <<  std::endl;
    }
}