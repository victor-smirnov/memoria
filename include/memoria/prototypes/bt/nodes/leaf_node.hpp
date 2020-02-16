
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

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/misc.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_iovector.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

namespace memoria {
namespace bt {




template <
    typename Types
>
class LeafNode: public Types::NodeBase
{
    using MyType = LeafNode<Types>;

public:
    static constexpr uint32_t VERSION = 2;

    static constexpr bool Leaf = true;

    template <typename CtrT, typename NodeT>
    using NodeSparseObject = LeafNodeSO<CtrT, NodeT>;

    using Base = typename Types::NodeBase;

    using TypesT = Types;

    using LeafSubstreamsStructList = typename Types::LeafStreamsStructList;
    using BranchSubstreamsStructList = typename Types::BranchStreamsStructList;

    template <typename PkdT>
    using PkdExtDataT = typename PkdT::ExtData;

    using BranchSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<BranchSubstreamsStructList>>;
    using BranchExtData = MakeTuple<BranchSubstreamExtensionsList>;

    using LeafSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<LeafSubstreamsStructList>>;
    using LeafExtData = MakeTuple<LeafSubstreamExtensionsList>;

    using CtrPropertiesMap = PackedMap<Varchar, Varchar>;
    using CtrReferencesMap = PackedMap<Varchar, ProfileCtrID<typename Types::Profile>>;

    using RootMetadataList = MergeLists<
        typename Types::Metadata,
        PackedTuple<BranchExtData>,
        PackedTuple<LeafExtData>,
        CtrPropertiesMap,
        CtrReferencesMap
    >;


    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            Linearize<LeafSubstreamsStructList>,
            Base::StreamsStart
    >::Type;


    using Dispatcher = PackedDispatcher<
        StreamDispatcherStructList
    >;

    using Base::allocator;

    LeafNode() noexcept = default;

    struct SerializeFn {
        template <typename Tree, typename SerializationData>
        VoidResult stream(const Tree* tree, SerializationData* buf) noexcept
        {
            return tree->serialize(*buf);
        }
    };

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::template serialize<RootMetadataList>(buf));

        return Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);
    }

    struct DeserializeFn {
        template <typename Tree, typename DeserializationData>
        VoidResult stream(Tree* tree, DeserializationData* buf) noexcept
        {
            return tree->deserialize(*buf);
        }
    };

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::template deserialize<RootMetadataList>(buf));
        return Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);
    }

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


}
