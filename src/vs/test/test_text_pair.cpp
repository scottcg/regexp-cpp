#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "traits.h"
#include "compile.h"
#include "code.h"
#include "tokens.h"
#include "ctext.h"

using ct = re_char_traits<char>;
using test_ctext = re::re_ctext<ct>;

namespace re {
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}