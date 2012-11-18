
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_RELATION_SORT_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_RELATION_SORT_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/relation/expression.hpp>
#include <memoria/core/types/relation/metadata.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>


namespace memoria    {

using namespace memoria;

template <
        typename Metadata,
        Int ColumnName,
        bool Asc,
        typename Relation
>
class Sorter {
    MEMORIA_STATIC_ASSERT(IsRelationMetadata<Metadata>::Value);
    MEMORIA_STATIC_ASSERT(IsList<Relation>::Value);

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


}

#endif
