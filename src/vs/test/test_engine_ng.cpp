#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <gtest/gtest.h>

// #define DEBUG // Uncomment this line for debugging

struct GroupState {
    int start;
    int end;
};

struct Instruction {
    enum Type {
        MATCH, MATCH_ANY, MATCH_GROUP, JUMP, ACCEPT,
        BEGIN_GROUP, END_GROUP, ANCHOR_START, ANCHOR_END,
        MATCH_RANGE, MATCH_ONE_OF, MATCH_STRING
    } type;
    std::string value;    // Primary value (used for MATCH, MATCH_ONE_OF, MATCH_STRING)
    std::string value2;   // Secondary value (used for MATCH_RANGE)
    int target;           // Jump target
    int fail_target;     // Fail target
    int group_id;         // Group ID for captures

    std::string to_string() const;
};

std::string type_to_string(Instruction::Type type) {
    switch (type) {
        case Instruction::MATCH: return "MATCH";
        case Instruction::MATCH_ANY: return "MATCH_ANY";
        case Instruction::MATCH_GROUP: return "MATCH_GROUP";
        case Instruction::JUMP: return "JUMP";
        case Instruction::ACCEPT: return "ACCEPT";
        case Instruction::BEGIN_GROUP: return "BEGIN_GROUP";
        case Instruction::END_GROUP: return "END_GROUP";
        case Instruction::ANCHOR_START: return "ANCHOR_START";
        case Instruction::ANCHOR_END: return "ANCHOR_END";
        case Instruction::MATCH_RANGE: return "MATCH_RANGE";
        case Instruction::MATCH_ONE_OF: return "MATCH_ONE_OF";
        case Instruction::MATCH_STRING: return "MATCH_STRING";
        default: return "UNKNOWN";
    }
}

std::string Instruction::to_string() const {
    return "Type:" + type_to_string(type) +
           ", Value:'" + value + "'" +
           ", Value2:" + value2 +
           ", Target:" + std::to_string(target) +
           ", Group ID:" + std::to_string(group_id);
}


// Execution Engine
bool execute(const std::vector<Instruction>& program, const std::string& input) {
    std::unordered_map<int, GroupState> groups; // Captured groups
    int instr_ptr = 0, input_ptr = 0;

#ifdef DEBUG
    std::cout << "Execution started\n";
#endif

    while (instr_ptr < program.size()) {
        const auto& instr = program[instr_ptr];

#ifdef DEBUG
        std::cout << "instr_ptr:" << instr_ptr
                  << " instr:" << instr.to_string()
                  << ", input_ptr:" << input_ptr
                  << "\n";
#endif

        switch (instr.type) {
            case Instruction::MATCH:
                if (input_ptr + instr.value.size() <= input.size() &&
                    input.substr(input_ptr, instr.value.size()) == instr.value) {
#ifdef DEBUG
                    std::cout << "MATCH SUCCESS: Value='" << instr.value
                              << "', Input at Pos='" << input.substr(input_ptr, instr.value.size())
                              << "', InputPtr=" << input_ptr << "\n";
#endif
                    input_ptr += instr.value.size();
                    ++instr_ptr;
                } else {
#ifdef DEBUG
                    std::cout << "MATCH FAIL: Value='" << instr.value
                              << "', Input at Pos='" << input.substr(input_ptr, instr.value.size())
                              << "', InputPtr=" << input_ptr << "\n";
#endif
                    return false;
                }
                break;

            case Instruction::MATCH_ANY:
                if (input_ptr < input.size()) {
#ifdef DEBUG
                    std::cout << "MATCH_ANY SUCCESS: Input='" << input[input_ptr]
                              << "', InputPtr=" << input_ptr << "\n";
#endif
                    ++input_ptr;
                    ++instr_ptr;
                } else {
#ifdef DEBUG
                    std::cout << "MATCH_ANY FAIL: End of input reached\n";
#endif
                    return false;
                }
                break;

            case Instruction::MATCH_ONE_OF:
                if (input_ptr < input.size() && instr.value.find(input[input_ptr]) != std::string::npos) {
#ifdef DEBUG
                    std::cout << "MATCH_ONE_OF SUCCESS: Value='" << instr.value
                              << "', Input='" << input[input_ptr] << "'\n";
#endif
                    ++input_ptr;
                    ++instr_ptr;
                } else {
#ifdef DEBUG
                    std::cout << "MATCH_ONE_OF FAIL: Input='" << input[input_ptr] << "'\n";
#endif
                    return false;
                }
                break;

            case Instruction::MATCH_RANGE:
                if (input_ptr < input.size() &&
                    input[input_ptr] >= instr.value[0] &&
                    input[input_ptr] <= instr.value2[0]) {
#ifdef DEBUG
                    std::cout << "MATCH_RANGE SUCCESS: Range='" << instr.value[0]
                              << "-" << instr.value2[0] << "', Input='" << input[input_ptr] << "'\n";
#endif
                    ++input_ptr;
                    ++instr_ptr;
                } else {
#ifdef DEBUG
                    std::cout << "MATCH_RANGE FAIL: Range='" << instr.value[0]
                              << "-" << instr.value2[0] << "', Input='" << input[input_ptr] << "'\n";
#endif
                    return false;
                }
                break;

            case Instruction::MATCH_STRING:
                if (input_ptr + instr.value.size() <= input.size() &&
                    input.substr(input_ptr, instr.value.size()) == instr.value) {
#ifdef DEBUG
                    std::cout << "MATCH_STRING SUCCESS: Value='" << instr.value
                              << "', Input at Pos='" << input.substr(input_ptr, instr.value.size())
                              << "'\n";
#endif
                    input_ptr += instr.value.size();
                    ++instr_ptr;
                } else {
#ifdef DEBUG
                    std::cout << "MATCH_STRING FAIL: Value='" << instr.value
                              << "', Input at Pos='" << input.substr(input_ptr, instr.value.size())
                              << "'\n";
#endif
                    return false;
                }
                break;

            case Instruction::BEGIN_GROUP:
                groups[instr.group_id] = {input_ptr, -1}; // Start of group capture
#ifdef DEBUG
                std::cout << "BEGIN_GROUP: Group ID=" << instr.group_id
                          << ", Start=" << input_ptr << "\n";
#endif
                ++instr_ptr;
                break;

            case Instruction::END_GROUP:
                if (groups.find(instr.group_id) != groups.end()) {
                    groups[instr.group_id].end = input_ptr; // End of group capture
#ifdef DEBUG
                    std::cout << "END_GROUP: Group ID=" << instr.group_id
                              << ", End=" << input_ptr << "\n";
#endif
                } else {
                    throw std::runtime_error("Group end without matching group start.");
                }
                ++instr_ptr;
                break;

            case Instruction::MATCH_GROUP: {
                auto it = groups.find(instr.group_id);
                if (it != groups.end()) {
                    auto& group = it->second;
                    if (group.end > group.start) {
                        std::string captured = input.substr(group.start, group.end - group.start);
#ifdef DEBUG
                        std::cout << "MATCH_GROUP: Group ID=" << instr.group_id
                                  << ", Captured='" << captured
                                  << "', Input at Pos='" << input.substr(input_ptr, captured.size()) << "'\n";
#endif
                        if (input.compare(input_ptr, captured.size(), captured) == 0) {
                            input_ptr += captured.size();
                            ++instr_ptr;
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
                break;
            }

            case Instruction::JUMP:
#ifdef DEBUG
                std::cout << "JUMP: Jumping to Target=" << instr.target << "\n";
#endif
                instr_ptr = instr.target;
                break;

            case Instruction::ANCHOR_START:
                if (input_ptr != 0) return false;
                ++instr_ptr;
                break;

            case Instruction::ANCHOR_END:
                if (input_ptr != input.size()) return false;
                ++instr_ptr;
                break;

            case Instruction::ACCEPT:
#ifdef DEBUG
                std::cout << "ACCEPT: Match successful\n";
#endif
                return true;

            default:
                throw std::runtime_error("Unknown instruction type.");
        }
    }

    return false; // If we reach here, execution fails
}

// Test: Single character match (regex: "a")
TEST(RegexEngineTest, MatchSingleCharacter) {
    std::string input = "a";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "a", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));
}

// Test: Match an entire string (regex: "hello")
TEST(RegexEngineTest, MatchString) {
    std::string input = "hello";
    std::vector<Instruction> program = {
        {Instruction::MATCH_STRING, "hello", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "hell";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match any character (regex: ".")
TEST(RegexEngineTest, MatchAnyCharacter) {
    std::string input = "x";
    std::vector<Instruction> program = {
        {Instruction::MATCH_ANY, "", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match range (regex: "[a-z]")
TEST(RegexEngineTest, MatchRange) {
    std::string input = "g";
    std::vector<Instruction> program = {
        {Instruction::MATCH_RANGE, "a", "z"},
        {Instruction::ACCEPT}
    };

    EXPECT_TRUE(execute(program, input));

    std::string failInput = "1";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Match one of specific characters (regex: "[abh]")
TEST(RegexEngineTest, MatchOneOf) {
    std::string input = "b";
    std::vector<Instruction> program = {
        {Instruction::MATCH_ONE_OF, "abh", ""},
        {Instruction::ACCEPT}
    };

    EXPECT_TRUE(execute(program, input));

    std::string failInput = "z";
    EXPECT_FALSE(execute(program, failInput));
}


// Test: Alternation (regex: "a|b")
TEST(RegexEngineTest, AlternationFixed) {
    std::string inputA = "a";
    std::string inputB = "b";

    std::vector<Instruction> program = {
        {Instruction::MATCH, "a", ""},          // Try matching 'a'
        {Instruction::JUMP, "", "", 4},         // If 'a' matches, jump to ACCEPT
        {Instruction::MATCH, "b", ""},          // Try matching 'b'
        {Instruction::JUMP, "", "", 4},         // If 'b' matches, jump to ACCEPT
        {Instruction::ACCEPT}                   // Successful match
    };

    EXPECT_TRUE(execute(program, inputA));  // Input 'a' should match
    EXPECT_TRUE(execute(program, inputB));  // Input 'b' should match

    std::string failInput = "c";
    EXPECT_FALSE(execute(program, failInput)); // Input 'c' should fail
}


TEST(RegexEngineTest, SimplifiedRepetition) {
    std::string input = "aaa";
    std::vector<Instruction> program = {
        {Instruction::MATCH, "a", ""},  // Match 'a'
        {Instruction::JUMP, "", "", 0}, // Loop back to match more 'a'
        {Instruction::ACCEPT}           // Successful match
    };

    EXPECT_TRUE(execute(program, input));

    std::string failInput = "b";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Backreference (regex: "(ab) \1")
TEST(RegexEngineTest, Backreference) {
    std::string input = "ab ab";
    std::vector<Instruction> program = {
        {Instruction::BEGIN_GROUP, "", "", 1},  // Start group 1
        {Instruction::MATCH, "ab", ""},
        {Instruction::END_GROUP, "", "", 1},    // End group 1
        {Instruction::MATCH, " ", ""},
        {Instruction::MATCH_GROUP, "", "", 1},  // Match backreference \1
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "ab ac";
    //EXPECT_FALSE(execute(program, failInput));
}

// Test: Start and end anchors (regex: "^abc$")
TEST(RegexEngineTest, Anchors) {
    std::string input = "abc";
    std::vector<Instruction> program = {
        {Instruction::ANCHOR_START},
        {Instruction::MATCH_STRING, "abc", ""},
        {Instruction::ANCHOR_END},
        {Instruction::ACCEPT}
    };

    EXPECT_TRUE(execute(program, input));

    std::string failInput = "aabc";
    EXPECT_FALSE(execute(program, failInput));
}
// Test: Complex pattern (regex: "^a[0-9]bc$")
TEST(RegexEngineTest, ComplexPattern) {
    std::string input = "a1bc";
    std::vector<Instruction> program = {
        {Instruction::ANCHOR_START, "", ""},
        {Instruction::MATCH, "a", ""},
        {Instruction::MATCH_RANGE, "0", "9"},
        {Instruction::MATCH_STRING, "bc", ""},
        {Instruction::ANCHOR_END, "", ""},
        {Instruction::ACCEPT}
    };
    EXPECT_TRUE(execute(program, input));

    std::string failInput = "aabc";
    EXPECT_FALSE(execute(program, failInput));
}

// Test: Combination of alternation and range (regex: "a|b|[c-f]")
TEST(RegexEngineTest, AlternationAndRange) {
    std::string input = "d";
    std::vector<Instruction> program = {
        {Instruction::JUMP, "", "", 3},          // Jump to 'b' branch if 'a' fails
        {Instruction::MATCH, "a", ""},           // Try matching 'a'
        {Instruction::JUMP, "", "", 8},          // If 'a' matches, jump to ACCEPT
        {Instruction::JUMP, "", "", 6},          // Jump to range branch if 'b' fails
        {Instruction::MATCH, "b", ""},           // Try matching 'b'
        {Instruction::JUMP, "", "", 8},          // If 'b' matches, jump to ACCEPT
        {Instruction::MATCH_RANGE, "c", "f"},    // Match any character in range 'c-f'
        {Instruction::ACCEPT}                    // Successful match
    };

    EXPECT_TRUE(execute(program, input));

    std::string failInput = "z";
    EXPECT_FALSE(execute(program, failInput));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
