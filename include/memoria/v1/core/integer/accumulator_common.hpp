
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
#include <memoria/v1/core/integer/clang/integer_primitives.hpp>
#elif BOOST_MSVC
#include <memoria/v1/core/integer/msvc/integer_primitives.hpp>
#else
#include <memoria/v1/core/integer/gcc/integer_primitives.hpp>
#endif

//#include <memoria/v1/core/integer/gcc/integer_primitives.hpp>

#include <ostream>

namespace memoria {
namespace v1 {

template <size_t Size> struct UnsignedAccumulator;

template <typename T>
using UnsignedAccumulatorT = UnsignedAccumulator<sizeof(T) * 8>;

}}
