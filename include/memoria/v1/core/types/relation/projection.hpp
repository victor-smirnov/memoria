
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
#include <memoria/v1/core/types/algo.hpp>

#include <memoria/v1/core/types/relation/metadata.hpp>


namespace memoria {
namespace v1 {

template <typename ColumnType, typename ColumnValue> struct WrapperProvider;

template <typename Type_, Type_ V>
struct ValueWrapper {
    typedef Type_               Type;
    static const Type Value     = V;
};


template <Int Name, typename ColumnValue>
struct WrapperProvider<Column<Name, NullType>, ColumnValue> {
    // no wrapper if value is a type
    typedef typename ColumnValue::Value                                         Type;
};

template <typename ColumnType, typename ColumnValue>
struct WrapperProvider {
    // ValueWrapper for constants
    typedef ValueWrapper<typename ColumnValue::Column::Type, ColumnValue::Value> Type;
};


template <typename Metadata, typename ColumnNamesList, typename Record>
class CreateColumnValuesList {

    MEMORIA_V1_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsNonemptyList<ColumnNamesList>::Value);

    template <typename Config, typename Column, typename Accumulator>
    struct Handler {
        typedef typename Metadata::template Provider<Column::Value, Record>::Value ColumnValue;
        typedef typename ColumnValue::Column ColumnType;

        typedef typename AppendTool<
                    typename WrapperProvider<ColumnType, ColumnValue>::Type,
                    typename Accumulator::Result
        >::Result                                                               Result;
    };

    struct Init {
        typedef TypeList<> Result;
    };

public:

    typedef typename ForEach<
                NullType,
                ColumnNamesList,
                Handler,
                Init
    >::Result::Result                                                           List;
};

template <typename Metadata, typename ColumnNamesList, typename RecordList>
class Projection {

    MEMORIA_V1_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsNonemptyList<ColumnNamesList>::Value);
    MEMORIA_V1_STATIC_ASSERT(IsList<RecordList>::Value);
    
    template <typename Config, typename Record, typename Accumulator>
    struct Handler {
        typedef typename CreateColumnValuesList<Metadata, ColumnNamesList, Record>::List ValuesList;

        typedef typename AppendTool<
                    ValuesList,
                    typename Accumulator::Result
        >::Result                                                               Result;
    };

    struct Init {
        typedef TypeList<> Result;
    };

public:

    typedef typename ForEach<
                NullType,
                RecordList,
                Handler,
                Init
    >::Result::Result                                                           List;
};


}}