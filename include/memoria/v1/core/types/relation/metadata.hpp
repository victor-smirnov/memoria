
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {

template <Int Name_, typename Type_ = NullType>
struct Column {
    static const Int    Name = Name_;
    typedef Type_       Type;
};

template <typename T>
struct IsColumn: ConstValue<bool, true> {};

template <Int Name, typename T>
struct IsColumn<Column<Name, T> >: ConstValue<bool, true> {};


template <typename ColumnType, typename TypeValue_>
struct TypeValue {
    typedef TypeValue_  Value;
    typedef ColumnType  Column;
};

template <typename ColumnType, typename ColumnType::Type Value_>
struct StaticValue {
    static const typename ColumnType::Type Value = Value_;
    typedef ColumnType Column;
};


template <Int Name> class ColumnNotFound;

template <Int Name, typename ColumnsList>
class findColumn {

    MEMORIA_V1_STATIC_ASSERT(IsNonemptyList<ColumnsList>::Value);

    template <typename Config, typename Item, typename Accumulator>
    struct Handler {
        static const bool Equal = Item::Name == Name;
        
        typedef IfThenElse<
                    Equal,
                    Item,
                    typename Accumulator::Result
        >                                                                       Result;
    };

    struct Init {
        typedef NullType Result;
    };

    typedef typename ForEach<
                NullType,
                ColumnsList,
                Handler,
                Init
    >::Result::Result                                                           Result0;

    static const bool Found = !IfTypesEqual<Result0, NullType>::Value;

public:

    typedef IfThenElse<Found, Result0, ColumnNotFound<Name> >                   Result;
};



template <
        typename ColumnsList,
        template <Int Name, typename ColumnType, typename Record> class ValueProvider
>
class RelationMetadata {
    MEMORIA_V1_STATIC_ASSERT(IsNonemptyList<ColumnsList>::Value);
public:
    typedef ColumnsList List;
    template <Int Name, typename Record>
    class Provider {
        typedef typename findColumn<Name, ColumnsList>::Result ColumnType;
    public:
        typedef ValueProvider<Name, ColumnType, Record>                         Value;
    };
};


template <typename T> struct IsRelationMetadata: ConstValue<bool, false> {};

template <
        typename ColumnsList,
        template <Int Name, typename ColumnType, typename Record> class ValueProvider
>
struct IsRelationMetadata<RelationMetadata<ColumnsList, ValueProvider> >: ConstValue<bool, true> {};

}}