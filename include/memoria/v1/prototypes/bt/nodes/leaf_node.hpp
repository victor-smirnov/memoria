
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

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/types/list/misc.hpp>
#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/types.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_iovector.hpp>

#include <memoria/v1/prototypes/bt/nodes/leaf_node_so.hpp>

namespace memoria {
namespace v1 {
namespace bt {




template <
    typename Types
>
class LeafNode: public Types::NodeBase
{
    static const int32_t BranchingFactor = PackedTreeBranchingFactor;

    using MyType = LeafNode<Types>;

public:
    static constexpr uint32_t VERSION = 2;

    static constexpr bool Leaf = true;

    template <typename CtrT, typename NodeT>
    using NodeSparseObject = LeafNodeSO<CtrT, NodeT>;

    using Base = typename Types::NodeBase;

    using TypesT = Types;

    //FIXME: Use SubDispatcher
    LeafNode() = default;

    using Base::allocator;

};




}



template <typename Types>
struct TypeHash<bt::LeafNode<Types> > {

    using Node = bt::LeafNode<Types>;

    static const uint64_t Value = HashHelper<
            TypeHashV<typename Node::Base>,
            Node::VERSION,
            true,
            TypeHashV<typename Types::Name>
    >;
};


}}
