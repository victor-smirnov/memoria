
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/relation/expression.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {


template <
        typename Metadata,
        typename Expression,
        typename Relation
>
class Filter {

    MEMORIA_V1_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsExpression<Expression>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsList<Relation>::Value);

    template <typename Config, typename Record, typename Accumulator>
    struct Handler {
        static const bool Value = Evaluator<Metadata, Expression, Record>::Value;
        typedef IfThenElse<
                    Value,
                    typename AppendTool<Record, typename Accumulator::Result>::Result,
                    typename Accumulator::Result
        >                                                                       Result;
    };

    struct Init {
        typedef TypeList<>            Result;
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

}}