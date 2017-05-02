
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

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/traits.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/types/traits.hpp>

#include <memoria/v1/core/tools/assert.hpp>


#include <typeinfo>

namespace memoria {
namespace v1 {

template <
        typename List,
        template <typename Item> class ValueProvider,
        typename ValueType = int64_t
>
class MinElement {

    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const ValueType Value    = ValueProvider<Item>::Value;
        
        static const bool IsMin         = Value <= Accumulator::MinValue;

        static const ValueType MinValue = IsMin ? Value : Accumulator::MinValue;

        typedef IfThenElse<
                    IsMin,
                    Item,
                    typename Accumulator::MinElement
        >                                                                       MinElement;
    };

    struct Init {
        typedef NullType            MinElement;
        static const ValueType      MinValue = ValueTraits<ValueType>::Max;
    };

public:

    typedef typename ForEachItem<
                NullType,
                List,
                Handler,
                Init
    >::Result                                                                   Result0;

    typedef typename Result0::MinElement                                        Result;
};




template <
        typename List,
        template <typename Item> class ValueProvider,
        typename ValueType = int64_t
>
class MaxElement {

    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const ValueType Value    = ValueProvider<Item>::Value;

        static const bool IsMax         = Value >= Accumulator::MaxValue;

        static const ValueType MaxValue = IsMax ? Value : Accumulator::MaxValue;

        typedef IfThenElse<
                    IsMax,
                    Item,
                    typename Accumulator::MaxElement
        >                                                                       MaxElement;
    };

    struct Init {
        typedef NullType            MaxElement;
        static const ValueType      MaxValue = ValueTraits<ValueType>::Min;
    };

    typedef typename ForEachItem<
                NullType,
                List,
                Handler,
                Init
    >::Result                                                                   Result0;

public:

    typedef typename Result0::MaxElement                                        Result;
};

template <typename Item>
struct TypeSizeValueProvider {
    static const int32_t Value = ValueTraits<Item>::Size;
};



}}