
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/types/typelist.hpp>

#include <memoria/prototypes/bt/nodes/node_dispatcher.hpp>

namespace memoria {
namespace bt {

template <
    template <typename> class TreeNode,
    typename Types
>
class NodePageAdaptor;



template<
    template <typename> class Type
>
struct LeafNodeTypes {
    template <typename BrachTypes, typename LeafTypes>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, LeafTypes>
    >;

    template <typename LeafTypes>
    using LeafTypesList = TypeList<
            NodePageAdaptor<Type, LeafTypes>
    >;

    template <typename Types>
    using BranchTypesList = TypeList<>;
};





template<
    template <typename> class Type
>
struct BranchNodeTypes {
    template <typename BranchTypes, typename LeafTypes>
    using AllTypesList = TypeList<
            NodePageAdaptor<Type, BranchTypes>
    >;

    template <typename LeafTypes>
    using LeafTypesList = TypeList<>;

    template <typename BranchTypes>
    using BranchTypesList = TypeList<
            NodePageAdaptor<Type, BranchTypes>
    >;
};



template<
    template <typename> class NodeType
>
struct TreeNodeType {
    template <typename Types>
    using Type = NodePageAdaptor<NodeType, Types>;
};


template <typename BranchTypes, typename LeafTypes, typename NodeTypes> struct NodeTypeListBuilder;


template <typename BranchTypes, typename LeafTypes, typename Head, typename... Tail>
struct NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Head, Tail...>> {

    typedef MergeLists<
            typename Head::template AllTypesList<BranchTypes, LeafTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::AllTypesList
    >                                                                           AllTypesList;


    typedef MergeLists<
            typename Head::template LeafTypesList<LeafTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::LeafTypesList
    >                                                                           LeafTypesList;

    typedef MergeLists<
            typename Head::template BranchTypesList<BranchTypes>,
            typename NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<Tail...>>::BranchTypesList
    >                                                                           BranchTypesList;
};


template <typename BranchTypes, typename LeafTypes>
struct NodeTypeListBuilder<BranchTypes, LeafTypes, TypeList<>> {
    typedef TypeList<>                                                          AllTypesList;
    typedef TypeList<>                                                          LeafTypesList;
    typedef TypeList<>                                                          BranchTypesList;
};





template <typename Types, typename NodeTypes> struct DefaultNodeTypeListBuilder;

template <typename Types, typename Head, typename... Tail>
struct DefaultNodeTypeListBuilder<Types, TypeList<Head, Tail...>> {

    typedef MergeLists<
            typename Head::template Type<Types>,
            typename DefaultNodeTypeListBuilder<Types, TypeList<Tail...>>::List
    >                                                                           List;
};


template <typename Types>
struct DefaultNodeTypeListBuilder<Types, TypeList<>> {
    typedef TypeList<>                                                          List;
};




template <
    typename Types
>
class BTreeDispatchers {

    using MyType            = BTreeDispatchers<Types>;

    using BranchTypes       = typename Types::BranchNodeTypes;
    using LeafTypes         = typename Types::LeafNodeTypes;

    using NodeTypesList     = typename Types::NodeTypesList;
    using NodeBaseG_        = typename Types::NodeBasePtr;

    using DefaultBranchNodeTypesList    = typename Types::DefaultBranchNodeTypesList;
    using DefaultLeafNodeTypesList      = typename Types::DefaultLeafNodeTypesList;

public:
    struct NodeTypesBase {
        using NodeBasePtr = NodeBaseG_;
    };

    struct AllDTypes: NodeTypesBase {
        using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::AllTypesList;
    };

    struct LeafDTypes: NodeTypesBase {
        using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::LeafTypesList;
    };

    struct BranchDTypes: NodeTypesBase {
        using List = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::BranchTypesList;
    };

    struct DefaultDTypes: NodeTypesBase {
        using List = MergeLists<
                typename DefaultNodeTypeListBuilder<LeafTypes, DefaultLeafNodeTypesList>::List,
                typename DefaultNodeTypeListBuilder<BranchTypes, DefaultBranchNodeTypesList>::List
        >;
    };

    struct TreeDTypes: NodeTypesBase {
        using List      = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::BranchTypesList;
        using ChildList = typename NodeTypeListBuilder<BranchTypes, LeafTypes, NodeTypesList>::AllTypesList;
    };

    template <typename CtrT>
    using NodeDispatcher        = NDT<CtrT, AllDTypes>;

    template <typename CtrT>
    using LeafDispatcher        = NDT<CtrT, LeafDTypes>;

    template <typename CtrT>
    using BranchDispatcher      = NDT<CtrT, BranchDTypes>;

    template <typename CtrT>
    using DefaultDispatcher     = NDT<CtrT, DefaultDTypes>;

    template <typename CtrT>
    using TreeDispatcher        = NDTTree<CtrT, TreeDTypes>;
};

}}
