
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/relation/expression.hpp>
#include <memoria/v1/core/types/relation/metadata.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>


namespace memoria {
namespace v1 {

using namespace memoria;

template <
        typename Metadata,
        Int ColumnName,
        bool Asc,
        typename Relation
>
class Sorter {
    MEMORIA_V1_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsList<Relation>::Value);

    typedef typename findColumn<ColumnName, typename Metadata::List>::Result    Column;
    typedef typename Column::Type                                               ValueType;

    template <typename Record>
    struct ValueProvider {
        typedef typename Metadata::template Provider<ColumnName, Record>::Value ColumnValue;
        static const ValueType Value = ColumnValue::Value;
    };

public:

    typedef typename Sort<Relation, ValueProvider, Asc, ValueType>::List        Result;
};


}}