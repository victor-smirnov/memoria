
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

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
        typename Result = TypeList<> >
class Sort;



template <
        typename SrcList,
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType,
        typename Result>
class Sort {

    MEMORIA_STATIC_ASSERT(IsList<SrcList>::Value);

    typedef IfThenElse<
                Asc,
                typename MinElement<SrcList, ValueProvider, ValueType>::Result,
                typename MaxElement<SrcList, ValueProvider, ValueType>::Result
    >                                                                           Element;

    typedef typename AppendTool<Element, Result>::Result                        List_;

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
class Sort<TypeList<>, ValueProvider, Asc, ValueType, Result> {
public:
    typedef Result                                                              List;
};


}
