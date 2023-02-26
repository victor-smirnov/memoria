
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
#include <memoria/core/reflection/typehash.hpp>
#include <memoria/core/types.hpp>

#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_batch_input.hpp>

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

    LeafNode()  = default;

    struct SerializeFn {
        template <typename Tree, typename SerializationData>
        void stream(const Tree* tree, SerializationData* buf)
        {
            return tree->serialize(*buf);
        }
    };

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<RootMetadataList>(buf);

        return Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);
    }

    struct CowSerializeFn {
        template <typename Tree, typename SerializationData, typename IDResolver>
        void stream(const Tree* tree, SerializationData* buf, const IDResolver*)
        {
            return tree->serialize(*buf);
        }

        template <typename SerializationData, typename IDResolver, bool Indexed, typename ValueHolder>
        void stream(
                const PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<ValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                >* pkd_buffer,
                SerializationData* buf,
                const IDResolver* id_resolver
        )
        {
            return pkd_buffer->cow_serialize(*buf, id_resolver);
        }
    };

    template <typename SerializationData, typename IDResolver>
    void cow_serialize(SerializationData& buf, const IDResolver* id_resolver) const
    {
        Base::template cow_serialize<RootMetadataList>(buf, id_resolver);
        return Dispatcher::dispatchNotEmpty(allocator(), CowSerializeFn(), &buf, id_resolver);
    }







    struct MemCowResolveIDSFn {
        template <typename Tree, typename IDResolver>
        void stream(const Tree* tree, const IDResolver*)
        {}

        template <typename IDResolver, bool Indexed, typename IDValueHolder>
        void stream(
                PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<IDValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                >* pkd_buffer,
                const IDResolver* id_resolver
        )
        {
            using DataType = CowBlockID<IDValueHolder>;

            using Buffer = PackedDataTypeBuffer<
                PackedDataTypeBufferTypes<
                    DataType,
                    Indexed,
                    1,
                    DTOrdering::UNORDERED
                >
            >;

            using ExtData = typename DataTypeTraits<DataType>::TypeDimensionsTuple;

            using BufferSO = PackedDataTypeBufferSO<
                ExtData,
                Buffer
            >;

            ExtData ext_data{};
            BufferSO buffer_so(&ext_data, pkd_buffer);

            size_t size = buffer_so.size();
            for (size_t c = 0; c < size; c++)
            {
                auto memref_id = id_resolver->resolve_id(buffer_so.access(0, c));

                auto state = buffer_so.make_update_state();
                buffer_so.commit_update(c, 1, state.first, [&](auto col, auto row)  {
                    return memref_id;
                });
            }
        }
    };


    template <typename IDResolver>
    void cow_resolve_ids(const IDResolver* id_resolver)
    {
        Base::cow_resolve_ids(id_resolver);
        return Dispatcher::dispatchNotEmpty(allocator(), MemCowResolveIDSFn(), id_resolver);
    }


    struct CowForEachChildFn {
        template <typename Tree, typename CallbackFn>
        void stream(const Tree*, CallbackFn&&){}

        template <typename IDValueHolder, bool Indexed, typename CallbackFn>
        void stream(
                const PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<IDValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                >* pkd_buffer,
                CallbackFn&& callback
        )
        {
            using DataType = CowBlockID<IDValueHolder>;

            using Buffer = PackedDataTypeBuffer<
                PackedDataTypeBufferTypes<
                    DataType,
                    Indexed,
                    1,
                    DTOrdering::UNORDERED
                >
            >;

            using ExtData = typename DataTypeTraits<DataType>::TypeDimensionsTuple;

            using BufferSO = PackedDataTypeBufferSO<
                ExtData,
                Buffer
            >;

            ExtData ext_data{};
            BufferSO buffer_so(&ext_data, const_cast<Buffer*>(pkd_buffer));

            size_t size = buffer_so.size();
            for (size_t c = 0; c < size; c++) {
                callback(buffer_so.access(0, c));
            }
        }
    };



    template <typename Fn>
    void for_each_child_node(Fn&& fn) const {
        return Dispatcher::dispatchNotEmpty(allocator(), CowForEachChildFn(), std::forward<Fn>(fn));
    }


    struct DeserializeFn {
        template <typename Tree, typename DeserializationData>
        void stream(Tree* tree, DeserializationData* buf)
        {
            return tree->deserialize(*buf);
        }
    };

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<RootMetadataList>(buf);
        Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);

        ProfileSpecificBlockTools<typename Types::Profile>::after_deserialization(this);
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
