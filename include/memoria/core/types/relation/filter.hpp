
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_RELATION_FILTER_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_RELATION_FILTER_HPP

#include <memoria/core/types/relation/expression.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria    {


template <
        typename Metadata,
        typename Expression,
        typename Relation
>
class Filter {

    MEMORIA_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_STATIC_ASSERT(IsExpression<Expression>::Value);
    MEMORIA_STATIC_ASSERT(IsList<Relation>::Value);

    template <typename Config, typename Record, typename Accumulator>
    struct Handler {
        static const bool Value = Evaluator<Metadata, Expression, Record>::Value;
        typedef typename IfThenElse<
                    Value,
                    TL<Record, typename Accumulator::Result>,
                    typename Accumulator::Result
        >::Result                                                               Result;
    };

    struct Init {
        typedef NullType            Result;
    };

public:

    typedef typename ForEach<
                NullType,
                Relation,
                Handler,
                Init
    >::Result                                                                   Result0;

    typedef typename Result0::Result                                            Result;
};

}

#endif
