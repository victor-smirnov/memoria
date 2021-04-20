
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

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

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

    struct CowSerializeFn {
        template <typename Tree, typename SerializationData, typename IDResolver>
        VoidResult stream(const Tree* tree, SerializationData* buf, const IDResolver*) noexcept
        {
            return tree->serialize(*buf);
        }

        template <typename SerializationData, typename IDResolver, bool Indexed, typename ValueHolder>
        VoidResult stream(
                const PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<ValueHolder>,
                            Indexed
                        >
                >* pkd_buffer,
                SerializationData* buf,
                const IDResolver* id_resolver
        ) noexcept
        {
            return pkd_buffer->cow_serialize(*buf, id_resolver);
        }
    };

    template <typename SerializationData, typename IDResolver>
    VoidResult cow_serialize(SerializationData& buf, const IDResolver* id_resolver) const noexcept
    {
        MEMORIA_TRY_VOID(Base::template cow_serialize<RootMetadataList>(buf, id_resolver));

        return Dispatcher::dispatchNotEmpty(allocator(), CowSerializeFn(), &buf, id_resolver);
    }


    struct MemCowResolveIDSFn {
        template <typename Tree, typename IDResolver>
        VoidResult stream(const Tree* tree, const IDResolver*) noexcept
        {
            return VoidResult::of();
        }

        template <typename IDResolver, bool Indexed, typename IDValueHolder>
        VoidResult stream(
                PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<IDValueHolder>,
                            Indexed
                        >
                >* pkd_buffer,
                const IDResolver* id_resolver
        ) noexcept
        {
            return wrap_throwing([&] () -> VoidResult {
                using DataType = CowBlockID<IDValueHolder>;

                using Buffer = PackedDataTypeBuffer<
                    PackedDataTypeBufferTypes<
                        DataType,
                        Indexed
                    >
                >;

                using ExtData = typename DataTypeTraits<DataType>::TypeDimensionsTuple;

                using BufferSO = PackedDataTypeBufferSO<
                ExtData,
                Buffer
                >;

                ExtData ext_data{};
                BufferSO buffer_so(&ext_data, pkd_buffer);

                psize_t size = buffer_so.size();

                for (psize_t c = 0; c < size; c++)
                {
                    auto memref_id = id_resolver->resolve_id(buffer_so.access(0, c));

                    MEMORIA_TRY_VOID(buffer_so.update_entries(c, 1, [&](auto col, auto row) noexcept {
                        return memref_id;
                    }));
                }

                return VoidResult::of();
            });
        }
    };


    template <typename IDResolver>
    VoidResult cow_resolve_ids(const IDResolver* id_resolver) noexcept
    {
        MEMORIA_TRY_VOID(Base::cow_resolve_ids(id_resolver));
        return Dispatcher::dispatchNotEmpty(allocator(), MemCowResolveIDSFn(), id_resolver);
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
        MEMORIA_TRY_VOID(Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf));

        ProfileSpecificBlockTools<typename Types::Profile>::after_deserialization(this);

        return VoidResult::of();
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
