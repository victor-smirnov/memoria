
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_ALGO_SORT_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_ALGO_SORT_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/algo/minmax.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>


namespace memoria    {

template <
        typename List,
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType = BigInt,
        typename Result = NullType>
class Sort;

template <
        typename SrcList,
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType,
        typename Result>
class Sort {

    MEMORIA_STATIC_ASSERT(IsList<SrcList>::Value);

    typedef typename IfThenElse<
                Asc,
                typename MinElement<SrcList, ValueProvider, ValueType>::Result,
                typename MaxElement<SrcList, ValueProvider, ValueType>::Result
    >::Result                                                                   Element;

    typedef TL<Element, Result>                                                 List_;

public:
    typedef typename Sort<
                typename RemoveTool<Element, SrcList>::Result,
                ValueProvider,
                Asc,
                ValueType,
                List_
    >::List                                                                     List;
};


template <
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType,
        typename Result
>
class Sort<NullType, ValueProvider, Asc, ValueType, Result> {
public:
    typedef Result                                                              List;
};


}

#endif
