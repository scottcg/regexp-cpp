
#pragma once

#include "traits.h"

namespace re {
    // Define the concept
    template <typename T>
    concept IsReCharTraits = std::is_same_v<T, re_char_traits<char>> ||
                             std::is_same_v<T, re_char_traits<wchar_t>>;

    template <typename traitsType> requires IsReCharTraits<traitsType> class compile_state;

}