
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>
#include <memoria/v1/core/types/algo/select.hpp>

#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {

template <typename Item>
struct DefaultListItemProvider {
    typedef Item Type;
};

template <typename Type, typename List, template <typename> class Provider = DefaultListItemProvider>
class IndexOf {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const int32_t Idx = IfTypesEqual<
                                    Type,
                                    typename Provider<Item>::Type
                               >::Value ? Accumulator::Counter : Accumulator::Idx;
        static const int32_t Counter = Accumulator::Counter + 1;
    };

    struct Init {
        static const int32_t Idx        = -1;
        static const int32_t Counter    = 0;
    };

public:

    static const int32_t Value = ForEachItem<
                NullType,
                List,
                Handler,
                Init
    >::Result::Idx;
};



}}