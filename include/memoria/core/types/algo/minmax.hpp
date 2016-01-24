
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_MINMAX_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_MINMAX_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/traits.hpp>
#include <memoria/core/types/algo/for_each.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/types/traits.hpp>

#include <memoria/core/tools/assert.hpp>


#include <typeinfo>

namespace memoria    {

template <
        typename List,
        template <typename Item> class ValueProvider,
        typename ValueType = BigInt
>
class MinElement {

    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const ValueType Value    = ValueProvider<Item>::Value;
        
        static const bool IsMin         = Value <= Accumulator::MinValue;

        static const ValueType MinValue = IsMin ? Value : Accumulator::MinValue;

        typedef IfThenElse<
                    IsMin,
                    Item,
                    typename Accumulator::MinElement
        >                                                               		MinElement;
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
        typename ValueType = BigInt
>
class MaxElement {

    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const ValueType Value    = ValueProvider<Item>::Value;

        static const bool IsMax         = Value >= Accumulator::MaxValue;

        static const ValueType MaxValue = IsMax ? Value : Accumulator::MaxValue;

        typedef IfThenElse<
                    IsMax,
                    Item,
                    typename Accumulator::MaxElement
        >                                                               		MaxElement;
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
    static const Int Value = ValueTraits<Item>::Size;
};



}

#endif
