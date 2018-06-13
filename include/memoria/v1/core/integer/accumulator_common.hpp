
// Copyright 2018 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memoria/v1/core/types.hpp>

#include <boost/config.hpp>

#if BOOST_CLANG
#   include <memoria/v1/core/integer/clang/integer_primitives.hpp>
#elif BOOST_MSVC
#   include <memoria/v1/core/integer/msvc/integer_primitives.hpp>
#else
#   include <memoria/v1/core/integer/gcc/integer_primitives.hpp>
#endif



#include <ostream>

namespace memoria {
namespace v1 {

template <size_t Size> struct UnsignedAccumulator;

template <typename T>
using UnsignedAccumulatorT = UnsignedAccumulator<sizeof(T) * 8>;

using UAcc64T = UnsignedAccumulator<64>;
using UAcc128T = UnsignedAccumulator<128>;
using UAcc192T = UnsignedAccumulator<192>;
using UAcc256T = UnsignedAccumulator<256>;


namespace _ {

static inline bool long_add_to(uint64_t* dest, const uint64_t* source, size_t length)
{
    uint64_t carry_in{};
    uint64_t carry_out{};

    for (size_t c = 0; c < length; c++)
    {
        dest[c] = BuiltinAddCH<uint64_t>::process(dest[c], source[c], carry_in, carry_out);
        carry_in = carry_out;
    }

    return carry_out == 0;
}

static inline bool long_add_to(uint64_t* dest, const uint64_t* x, const uint64_t* y, size_t length)
{
    uint64_t carry_in{};
    uint64_t carry_out{};

    for (size_t c = 0; c < length; c++)
    {
        dest[c] = BuiltinAddCH<uint64_t>::process(x[c], y[c], carry_in, carry_out);
        carry_in = carry_out;
    }

    return carry_out == 0;
}


static inline bool long_sub_from(uint64_t* dest, const uint64_t* op, size_t length)
{
    uint64_t carry_in{};
    uint64_t carry_out{};

    for (size_t c = 0; c < length; c++)
    {
        dest[c] = BuiltinSubCH<uint64_t>::process(dest[c], op[c], carry_in, carry_out);
        carry_in = carry_out;
    }

    return carry_out == 0;
}

static inline bool long_sub_from(uint64_t* dest, const uint64_t* x, const uint64_t* y, size_t length)
{
    uint64_t carry_in{};
    uint64_t carry_out{};

    for (size_t c = 0; c < length; c++)
    {
        dest[c] = BuiltinSubCH<uint64_t>::process(x[c], y[c], carry_in, carry_out);
        carry_in = carry_out;
    }

    return carry_out == 0;
}


}

}}
