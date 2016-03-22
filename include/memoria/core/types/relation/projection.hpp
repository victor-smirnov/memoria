
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/algo.hpp>

#include <memoria/core/types/relation/metadata.hpp>


namespace memoria    {

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

    MEMORIA_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_STATIC_ASSERT(IsNonemptyList<ColumnNamesList>::Value);

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

    MEMORIA_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_STATIC_ASSERT(IsNonemptyList<ColumnNamesList>::Value);
    MEMORIA_STATIC_ASSERT(IsList<RecordList>::Value);
    
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


}
