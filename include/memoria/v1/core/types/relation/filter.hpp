
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