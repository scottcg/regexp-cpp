#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "traits.h"
#include "compile.h"
#include "code.h"
#include "tokens.h"

using ct = re_char_traits<char>;
using test_code_vector = re::compiled_code_vector<ct>;

namespace re {

    TEST(CompiledCodeVectorTest, Initialization) {
        const test_code_vector code_vector;
        EXPECT_EQ(code_vector.offset(), 0);
    }

    TEST(CompiledCodeVectorTest, StoreSingleCode) {
        test_code_vector code_vector;
        const int pos = code_vector.store('A');
        EXPECT_EQ(pos, 0);
        EXPECT_EQ(code_vector[0], 'A');
    }

    TEST(CompiledCodeVectorTest, StoreMultipleCodes) {
        test_code_vector code_vector;
        code_vector.store('A');
        code_vector.store('B');
        EXPECT_EQ(code_vector[0], 'A');
        EXPECT_EQ(code_vector[1], 'B');
    }

    TEST(CompiledCodeVectorTest, ModifyValue) {
        test_code_vector code_vector;
        code_vector.store('A');
        code_vector.store('B');
        code_vector[1] = 'C';
        EXPECT_EQ(code_vector[1], 'C');
    }

    TEST(CompiledCodeVectorTest, PutAddress) {
        test_code_vector code_vector;
        code_vector.store('A');
        code_vector.store('B');
        code_vector.put_address(1, 10);
        EXPECT_EQ(static_cast<int>(code_vector[1]), 7);
        EXPECT_EQ(static_cast<int>(code_vector[2]), 0);
    }

    TEST(CompiledCodeVectorTest, StoreAndRetrieveMultipleValues) {
        test_code_vector code_vector;
        for (char c = 'A'; c <= 'Z'; ++c) {
            code_vector.store(c);
        }
        for (char c = 'A'; c <= 'Z'; ++c) {
            EXPECT_EQ(code_vector[c - 'A'], c);
        }
    }

} // namespace re

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}