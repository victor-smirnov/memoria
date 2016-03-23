
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