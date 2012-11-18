
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_INDEXOF_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_INDEXOF_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/algo/for_each.hpp>
#include <memoria/core/types/algo/select.hpp>

#include <memoria/core/tools/assert.hpp>

namespace memoria    {

template <typename Item>
struct DefaultListItemProvider {
    typedef Item Type;
};

template <typename Type, typename List, template <typename> class Provider = DefaultListItemProvider>
class IndexOf {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const Int Idx = IfTypesEqual<
                                    Type,
                                    typename Provider<Item>::Type
                               >::Value ? Accumulator::Counter : Accumulator::Idx;
        static const Int Counter = Accumulator::Counter + 1;
    };

    struct Init {
        static const Int Idx        = -1;
        static const Int Counter    = 0;
    };

public:

    static const Int Value = ForEach<
                NullType,
                List,
                Handler,
                Init
    >::Result::Idx;
};



}

#endif
