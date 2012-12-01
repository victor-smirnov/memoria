
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_DISPATCHERS_TOOLS_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/relation.hpp>


namespace memoria    {
namespace btree      {


template <
        typename Expression,
        typename Relation
>
class NodeFilter: public Filter<NodeDescriptorMetadata, Expression, Relation> {};


template <typename List, typename Result = VTL<> > struct LevelListBuilder;

template <typename Head, typename ... Tail, typename Result>
struct LevelListBuilder<VTL<Head, Tail...>, Result> {
    static const BigInt Level = Head::Descriptor::Level;
private:
    typedef typename AppendTool<TypeCode<Level>, Result>::Result                NewResult;
public:
    typedef typename LevelListBuilder<VTL<Tail...>, NewResult>::List            List;
};

template <typename Result>
struct LevelListBuilder<VTL<>, Result> {
    typedef typename RemoveDuplicatesTool<Result>::Result                       List;
};


namespace intrnl {

template <
        typename List,
        typename SrcList,
        typename Result = VTL<>
>
class Node2NodeMapBuilderTool;

template <
        typename SrcList,
        typename Result
>
class Node2NodeMapBuilderTool<VTL<>, SrcList, Result> {
public:
    typedef Result                                                              Map;
};

template <
        typename Head,
        typename ... Tail,
        typename SrcList,
        typename Result
>
class Node2NodeMapBuilderTool<VTL<Head, Tail...>, SrcList, Result> {
    static const bool   Root  = Head::Descriptor::Root;
    static const bool   Leaf  = Head::Descriptor::Leaf;
    static const BigInt Level = Head::Descriptor::Level;

    typedef typename NodeFilter<ValueOp<ROOT,  EQ, bool, !Root>,  SrcList>::Result         InverseRootList;
    typedef typename NodeFilter<ValueOp<LEAF,  EQ, bool,  Leaf>,  InverseRootList>::Result SameLeafList;
    typedef typename NodeFilter<ValueOp<LEVEL, EQ, Short, Level>, SameLeafList>::Result    SameLevelList;

    typedef typename AppendTool<
                Pair<
                    Head,
                    typename ListHead<SameLevelList>::Type
                >,
                Result
            >::Result                                                           NewResult;

public:
    typedef typename Node2NodeMapBuilderTool<VTL<Tail...>, SrcList, NewResult>::Map     Map;
};
}

template <
        typename List,
        bool IsRoot2Node,
        typename SrcList = List,
        typename Result = VTL<>
>
class Node2NodeMapTool {
    typedef typename NodeFilter<
                        ValueOp<ROOT, EQ, bool, IsRoot2Node>,
                        List
                     >::Result                                                  TargetList;
public:
    typedef typename intrnl::Node2NodeMapBuilderTool<TargetList, SrcList>::Map  Map;
};


template <bool Root, bool Leaf, typename NodeTL>
class FindNodeWithMaxLevelTool {
    typedef typename NodeFilter<
                And <
                    ValueOp<ROOT, EQ, bool, Root>,
                    ValueOp<LEAF, EQ, bool, Leaf>
                >,
                NodeTL
    >::Result                                                                   SubList;
public:
    
    typedef typename ListHead<typename Sorter<NodeDescriptorMetadata, LEVEL, false, SubList>::Result>::Type Type;
};

}
}

#endif
