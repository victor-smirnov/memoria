
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_DEFS_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_DEFS_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/relation.hpp>


namespace memoria    {
namespace btree     {



const BigInt ANY_LEVEL = 0x7fff;

typedef enum {ROOT, LEAF, LEVEL} NodeDescriptorColumns;

template <bool Root_, bool Leaf_, BigInt Level_>
struct NodeDescriptor {
    static const bool   Root  =      Root_;
    static const bool   Leaf  =      Leaf_;
    static const BigInt Level =      Level_;
};

template <Int Name, typename ColumnType, typename Record> struct ValueProvider;

typedef RelationMetadata<
            TypeList<Column<ROOT, bool>, Column<LEAF, bool>, Column<LEVEL, Short> >,
            ValueProvider
>                                                                               NodeDescriptorMetadata;


template <typename ColumnType, typename Record>
struct ValueProvider<ROOT, ColumnType, Record>:
    public StaticValue<ColumnType, Record::Descriptor::Root> {};

template <typename ColumnType, typename Record>
struct ValueProvider<LEAF, ColumnType, Record>:
    public StaticValue<ColumnType, Record::Descriptor::Leaf> {};

template <typename ColumnType, typename Record>
struct ValueProvider<LEVEL, ColumnType, Record>:
    public StaticValue<ColumnType, Record::Descriptor::Level> {};


}
}

#endif
