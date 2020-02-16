
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

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/prototypes/bt/nodes/branch_node_so.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/core/packed/misc/packed_tuple.hpp>
#include <memoria/core/packed/misc/packed_map.hpp>

#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/tools/result.hpp>

#include <type_traits>

namespace memoria {
namespace bt {

template <
        template <typename> class,
        typename
>
class NodePageAdaptor;



template <typename Metadata, typename Header_>
class TreeNodeBase {
public:
    static const uint32_t VERSION = 1;
    using Header = Header_;

    using BlockID = typename Header::BlockID;


    static_assert(IsPackedStructV<Metadata>, "TreeNodeBase: Metadata must satisfy IsPackedStructV<>");
    static_assert(IsPackedStructV<Header>, "TreeNodeBase: Header must satisfy IsPackedStructV<>");
    static_assert(IsPackedStructV<BlockID>, "TreeNodeBase: ID must satisfy IsPackedStructV<>");

    enum {
            METADATA = 0,
            BRANCH_TYPES = 1,
            LEAF_TYPES = 2,
            CTR_PROPERTIES = 3,
            CTR_REFERENCES = 4,
            MAX_METADATA_NUM
    };

    static const int32_t StreamsStart = MAX_METADATA_NUM;

private:
    Header header_;

    int32_t root_;
    int32_t leaf_;
    int32_t level_;

    BlockID next_leaf_id_;

    PackedAllocator allocator_;

    static_assert(std::is_pod<PackedAllocator>::value, "PackedAllocator must be a POD tpye");

public:



    using MyType = TreeNodeBase<Metadata, Header>;

    TreeNodeBase() noexcept = default;

    Header& header() noexcept {return header_;}
    const Header& header() const noexcept {return header_;}

    Header* as_header() noexcept {return &header_;}
    const Header* as_header() const noexcept {return &header_;}

    BlockID& id() noexcept {return header_.id();}
    const BlockID& id() const noexcept {return header_.id();}

    BlockID& uuid() noexcept {return header_.uuid();}
    const BlockID& uuid() const noexcept {return header_.uuid();}


    uint64_t ctr_type_hash() const noexcept {
        return header_.ctr_type_hash();
    }

    uint64_t block_type_hash() const noexcept {
        return header_.block_type_hash();
    }

    int32_t used_memory_block_size() const noexcept {
        return header_.memory_block_size() - allocator()->free_space();
    }

    void init() noexcept {
        header_.init();
    }

    inline bool is_root() const noexcept {
        return root_;
    }

    void set_root(bool root) noexcept {
        root_ = root;
    }

    inline bool is_leaf() const noexcept {
        return leaf_;
    }

    void set_leaf(bool leaf) noexcept {
        leaf_ = leaf;
    }

    const int32_t& level() const noexcept
    {
        return level_;
    }

    int32_t& level() noexcept
    {
        return level_;
    }

    const BlockID& next_leaf_id() const noexcept {
        return next_leaf_id_;
    }

    BlockID& next_leaf_id() noexcept {
        return next_leaf_id_;
    }

    PackedAllocator* allocator() noexcept {
        return &allocator_;
    }

    const PackedAllocator* allocator() const noexcept {
        return &allocator_;
    }

    psize_t root_metadata_size() const noexcept
    {
        const PackedAllocator* alloc = allocator();

        psize_t size{};

        for (int32_t c = 0; c < StreamsStart; c++) {
            size += alloc->element_size(c);
        }

        return size;
    }

    bool has_root_metadata() const noexcept
    {
        return root_metadata_size() > 0;
    }

    const Metadata& root_metadata() const noexcept
    {
        return *allocator()->template get<Metadata>(METADATA);
    }

    Metadata& root_metadata() noexcept
    {
        return *allocator()->template get<Metadata>(METADATA);
    }

    VoidResult setMetadata(const Metadata& meta) noexcept
    {
        if (!has_root_metadata())
        {
            MEMORIA_TRY_VOID(allocator_.template allocate<Metadata>(METADATA));
        }

        root_metadata() = meta;

        return VoidResult::of();
    }

    VoidResult clear_metadata() noexcept
    {
        for (int32_t c = 0; c < StreamsStart; c++)
        {
            MEMORIA_TRY_VOID(allocator_.free(c));
        }

        return VoidResult::of();
    }

    bool can_convert_to_root(psize_t metadata_size) const noexcept
    {
        if (!has_root_metadata())
        {
            return allocator_.free_space() >= (int32_t)metadata_size;
        }
        else {
            return true;
        }
    }

    VoidResult copy_metadata_from(const TreeNodeBase* other) noexcept
    {
        for (int32_t c = 0; c < StreamsStart; c++)
        {
            MEMORIA_TRY_VOID(allocator_.importBlock(c, other->allocator(), c));
        }

        return VoidResult::of();
    }



    BoolResult shouldBeMergedWithSiblings() const noexcept
    {
        int32_t client_area = allocator_.client_area();
        int32_t used        = allocator_.allocated();

        return BoolResult::of(used < (client_area / 2));
    }

public:

    VoidResult initAllocator(int32_t entries) noexcept
    {
        int32_t block_size = this->header().memory_block_size();
        MEMORIA_ASSERT_RTN(block_size, >, static_cast<int>(sizeof(MyType)) + PackedAllocator::my_size());

        allocator_.allocatable().setTopLevelAllocator();
        return allocator_.init(block_size - static_cast<int>(sizeof(MyType)) + PackedAllocator::my_size(), entries);
    }

    VoidResult resizeBlock(int32_t new_size) noexcept
    {
        int32_t space_delta = new_size - header_.memory_block_size();
        int32_t free_space  = allocator_.free_space();

        if (space_delta < 0 && free_space < -space_delta)
        {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "Resizing block {} has insufficient space for downsizing: available {}, requied {}",
                uuid(),
                free_space,
                -space_delta
            );
        }

        header_.memory_block_size() = new_size;
        return allocator_.resizeBlock(new_size - sizeof(MyType) + PackedAllocator::my_size());
    }

public:

    template <typename List>
    struct GenerateDataEventsFn {
        template <int32_t Idx>
        static BoolResult process(IBlockDataEventHandler* handler, const PackedAllocator* allocator)
        {
            using T = Select<Idx, List>;
            if (!allocator->is_empty(Idx))
            {
                const T* value = get<T>(allocator, Idx);

                using SparseObject = typename T::SparseObject;

                SparseObject so(const_cast<T*>(value));

                MEMORIA_TRY_VOID(so.generateDataEvents(handler));
            }

            return BoolResult::of(true);
        }
    };


    template <typename MetadataTypesList>
    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        static_assert(ListSize<MetadataTypesList> == StreamsStart, "");

        MEMORIA_TRY_VOID(header_.generateDataEvents(handler));

        handler->value("ROOT",  &root_);
        handler->value("LEAF",  &leaf_);
        handler->value("LEVEL", &level_);

        handler->value("NEXT_LEAF_ID_", &next_leaf_id_);

        MEMORIA_TRY_VOID(allocator()->generateDataEvents(handler));

        return ForEach<0, StreamsStart>::process_res(GenerateDataEventsFn<MetadataTypesList>(), handler, allocator());
    }


    template <typename List>
    struct SerializeFn {
        template <int32_t Idx, typename SerializationData>
        static BoolResult process(SerializationData& buf, const PackedAllocator* allocator) noexcept
        {
            using T = Select<Idx, List>;
            if (!allocator->is_empty(Idx))
            {
                const T* value = get<T>(allocator, Idx);
                MEMORIA_TRY_VOID(value->serialize(buf));
            }

            return BoolResult::of(true);
        }
    };


    template <typename MetadataTypesList, typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        static_assert(ListSize<MetadataTypesList> == StreamsStart, "");

        MEMORIA_TRY_VOID(header_.template serialize<FieldFactory>(buf));

        FieldFactory<int32_t>::serialize(buf, root_);
        FieldFactory<int32_t>::serialize(buf, leaf_);
        FieldFactory<int32_t>::serialize(buf, level_);

        FieldFactory<BlockID>::serialize(buf, next_leaf_id_);

        MEMORIA_TRY_VOID(allocator()->serialize(buf));

        return ForEach<0, StreamsStart>::process_res(SerializeFn<MetadataTypesList>(), buf, allocator());
    }

    template <typename List>
    struct DeserializeFn {
        template <int32_t Idx, typename DeserializationData>
        static BoolResult process(DeserializationData& buf, PackedAllocator* allocator) noexcept
        {
            using T = Select<Idx, List>;
            if (!allocator->is_empty(Idx))
            {
                T* value = get<T>(allocator, Idx);
                MEMORIA_TRY_VOID(value->deserialize(buf));
            }

            return BoolResult::of(true);
        }
    };

    template <typename MetadataTypesList, typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        static_assert(ListSize<MetadataTypesList> == StreamsStart, "");
        MEMORIA_TRY_VOID(header_.template deserialize<FieldFactory>(buf));

        FieldFactory<int32_t>::deserialize(buf, root_);
        FieldFactory<int32_t>::deserialize(buf, leaf_);
        FieldFactory<int32_t>::deserialize(buf, level_);

        FieldFactory<BlockID>::deserialize(buf, next_leaf_id_);

        MEMORIA_TRY_VOID(allocator()->deserialize(buf));

        return ForEach<0, StreamsStart>::process_res(DeserializeFn<MetadataTypesList>(), buf, allocator());
    }



    template <typename List>
    struct InitMetadataFn {
        template <int32_t Idx>
        static BoolResult process(PackedAllocator* allocator) noexcept
        {
            using T = Select<Idx, List>;
            if (allocator->is_empty(Idx))
            {
                MEMORIA_TRY_VOID(allocator->allocateEmpty<T>(Idx));
            }

            return BoolResult::of(true);
        }
    };

    template <typename MetadataTypesList>
    VoidResult init_root_metadata() noexcept
    {
        static_assert(ListSize<MetadataTypesList> == StreamsStart, "");

        return ForEach<0, StreamsStart>::process_res(InitMetadataFn<MetadataTypesList>(), allocator());
    }


    int32_t compute_metadata_size() const noexcept
    {
        int32_t mem_size = 0;

        for (int32_t c = 0; c < StreamsStart; c++) {
            mem_size += allocator_.element_size(c);
        }

        return mem_size;
    }

    int32_t compute_parent_size() const noexcept
    {
        int32_t client_area = allocator_.client_area();
        int32_t metadata_segments_size = compute_metadata_size();
        return client_area - metadata_segments_size;
    }

    int32_t compute_streams_available_space() const noexcept
    {
        int32_t occupied = compute_parent_size();
        return occupied;
    }
};







template <
    typename Types
>
class BranchNode: public Types::NodeBase
{
    using MyType = BranchNode;

public:
    static constexpr uint32_t VERSION = 1;
    static constexpr bool Leaf = false;

    using Base = typename Types::NodeBase;

public:

    using TypesT = Types;

    template <typename CtrT, typename NodeT>
    using NodeSparseObject = BranchNodeSO<CtrT, NodeT>;

    using BranchNodeEntry = typename Types::BranchNodeEntry;
    using Position = typename Types::Position;

    using Value = typename Types::ID;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList    = typename Types::BranchStreamsStructList;
    using LeafSubstreamsStructList      = typename Types::LeafStreamsStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            FlattenBranchTree<BranchSubstreamsStructList>, Base::StreamsStart
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList>;

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

    static constexpr int32_t Streams            = ListSize<BranchSubstreamsStructList>;

    static constexpr int32_t Substreams         = Dispatcher::Size;

    static constexpr int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static constexpr int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static constexpr int32_t ValuesBlockIdx     = SubstreamsEnd;

    constexpr BranchNode() noexcept = default;

private:
    struct InitFn {
        uint64_t active_streams_;

        InitFn(uint64_t active_streams) noexcept: active_streams_(active_streams) {}

        Int32Result block_size(int32_t items_number) const noexcept
        {
            return MyType::block_size(items_number, active_streams_);
        }

        int32_t max_elements(int32_t block_size) const noexcept
        {
            return block_size;
        }
    };

public:
    PackedAllocator* allocator()
    {
        return Base::allocator();
    }

    const PackedAllocator* allocator() const
    {
        return Base::allocator();
    }

    bool is_stream_empty(int32_t idx) const
    {
        return allocator()->is_empty(idx + SubstreamsStart);
    }

    Value* values() {
        return allocator()->template get<Value>(ValuesBlockIdx);
    }

    const Value* values() const {
        return allocator()->template get<Value>(ValuesBlockIdx);
    }




    bool is_empty() const
    {
        for (int32_t c = SubstreamsStart; c < SubstreamsEnd; c++) // StreamsEnd+1 because of values block?
        {
            if (!allocator()->is_empty(c)) {
                return false;
            }
        }

        return true;
    }

private:
    struct TreeSizeFn {
        int32_t size_ = 0;

        template <int32_t StreamIndex, int32_t AllocatorIdx, int32_t Idx, typename Node>
        void stream(Node*, int32_t tree_size, uint64_t active_streams)
        {
            if (active_streams & (1ull << StreamIndex))
            {
                size_ += Node::packed_block_size(tree_size);
            }
        }
    };


    struct TreeSize2Fn {
        int32_t size_ = 0;

        template <int32_t StreamIndex, int32_t AllocatorIdx, int32_t Idx, typename Node>
        void stream(Node*, const Position* sizes)
        {
            int32_t size = sizes->value(StreamIndex);
            if (size > 0)
            {
                size_ += Node::packed_block_size(size);
            }
        }
    };

public:
    static Int32Result block_size(int32_t tree_size, uint64_t active_streams = -1) noexcept
    {
        TreeSizeFn fn;

        MEMORIA_TRY_VOID(processSubstreamGroupsStatic(fn, tree_size, active_streams));

        int32_t tree_block_size     = fn.size_;
        int32_t array_block_size    = PackedAllocatable::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return Int32Result::of(PackedAllocator::block_size(client_area, Streams + 1));
    }


    static Int32Result block_size(const Position& sizes, int32_t values_size) noexcept
    {
        TreeSize2Fn fn;

        MEMORIA_TRY_VOID(processSubstreamGroupsStatic(fn, &sizes));

        int32_t tree_block_size     = fn.size_;
        int32_t array_block_size    = PackedAllocatable::roundUpBytesToAlignmentBlocks(values_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return Int32Result::of(PackedAllocator::block_size(client_area, Streams + 1));
    }

public:
    static Int32Result max_tree_size1(int32_t block_size, uint64_t active_streams = -1) noexcept
    {
        return FindTotalElementsNumber2(block_size, InitFn(active_streams));
    }

public:
    VoidResult prepare() noexcept
    {
        return Base::initAllocator(SubstreamsStart + Substreams + 1);
    }

    uint64_t active_streams() const
    {
        uint64_t streams = 0;
        for (int32_t c = 0; c < Streams; c++)
        {
            uint64_t bit = !allocator()->is_empty(c + SubstreamsStart);
            streams += (bit << c);
        }

        return streams;
    }

private:

    struct InitStructFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree*, int32_t tree_size, PackedAllocator* allocator, uint64_t active_streams)
        {
            if (active_streams && (1 << Idx))
            {
                int32_t tree_block_size = Tree::block_size(tree_size);
                allocator->template allocate<Tree>(AllocatorIdx, tree_block_size);
            }
        }

        template <int32_t AllocatorIdx, int32_t Idx>
        void stream(Value*, int32_t tree_size, PackedAllocator* allocator)
        {
            allocator->template allocateArrayBySize<Value>(AllocatorIdx, tree_size);
        }
    };

public:

    static int32_t client_area(int32_t block_size)
    {
        int32_t allocator_block_size = block_size - sizeof(MyType) + PackedAllocator::my_size();
        return PackedAllocator::client_area(allocator_block_size, Streams);
    }




    struct SizeFn {
        int32_t size_ = 0;

        template <typename Tree>
        void stream(const Tree* tree)
        {
            size_ = tree != nullptr ? tree->size() : 0;
        }
    };

    Int32Result size() const noexcept
    {
        SizeFn fn;
        //FIXME: use correct procedure to get number of children
        MEMORIA_TRY_VOID(Dispatcher::dispatch(0, allocator(), fn));
        return Int32Result::of(fn.size_);
    }


    struct SerializeFn {
        template <typename StreamObj, typename SerializationData>
        VoidResult stream(const StreamObj* stream, SerializationData* buf) noexcept
        {
            return stream->serialize(*buf);
        }
    };

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::template serialize<RootMetadataList>(buf));

        MEMORIA_TRY_VOID(Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf));

        MEMORIA_TRY(size, this->size());

        FieldFactory<Value>::serialize(buf, values(), size);

        return VoidResult::of();
    }

    struct DeserializeFn {
        template <typename StreamObj, typename DeserializationData>
        VoidResult stream(StreamObj* obj, DeserializationData* buf) noexcept
        {
            return obj->deserialize(*buf);
        }
    };

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::template deserialize<RootMetadataList>(buf));

        MEMORIA_TRY_VOID(Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf));

        MEMORIA_TRY(size, this->size());

        FieldFactory<Value>::deserialize(buf, values(), size);

        return VoidResult::of();
    }





    /************************************************/

    template <typename Fn, typename... Args>
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};





template <
    template <typename> class TreeNode,
    typename Types
>
class NodePageAdaptor: public TreeNode<Types>
{
public:

    using MyType  = NodePageAdaptor<TreeNode, Types>;
    using Base    = TreeNode<Types>;
    using Profile = typename Types::Profile;

    template <typename CtrT>
    using SparseObject = typename Base::template NodeSparseObject<CtrT, MyType>;

    static const uint64_t BLOCK_HASH = TypeHashV<Base>;

    static_assert(IsPackedStructV<TreeNode<Types>>, "TreeNode must satisfy IsPackedStructV");

    template <
        template <typename> class,
        typename
    >
    friend class NodePageAdaptor;

private:

public:
    NodePageAdaptor() = default;

    static uint64_t hash() {
        return BLOCK_HASH;
    }


    struct BlockOperations: public IBlockOperations<Profile>
    {
        using typename IBlockOperations<Profile>::BlockType;

        virtual ~BlockOperations() noexcept {}

        virtual Int32Result serialize(const BlockType* block, void* buf) const noexcept
        {
            return wrap_throwing([&]() -> Int32Result {
                const MyType* node = ptr_cast<const MyType>(block);

                SerializationData data;
                data.buf = ptr_cast<char>(buf);

                MEMORIA_TRY_VOID(node->serialize(data));

                return Int32Result::of(data.total);
            });
        }

        virtual VoidResult deserialize(const void* buf, int32_t buf_size, BlockType* block) const noexcept
        {
            return wrap_throwing([&]() -> VoidResult {
                MyType* node = ptr_cast<MyType>(block);

                DeserializationData data;
                data.buf = ptr_cast<const char>(buf);

                MEMORIA_TRY_VOID(node->deserialize(data));

                return VoidResult::of();
            });
        }

        virtual VoidResult resize(const BlockType* block, void* buffer, int32_t new_size) const noexcept
        {
            return wrap_throwing([&]() {
                MyType* tgt = ptr_cast<MyType>(buffer);
                return tgt->resizeBlock(new_size);
            });
        }

        virtual uint64_t block_type_hash() const noexcept {
            return MyType::BLOCK_HASH;
        }
    };

    static BlockOperationsPtr<Profile> block_operations() noexcept {
        return std::make_shared<BlockOperations>();
    }
};

}

template <typename Metadata, typename Base>
struct TypeHash<bt::TreeNodeBase<Metadata, Base>> {
    using TargetType = bt::TreeNodeBase<Metadata, Base>;

    static constexpr uint64_t Value = HashHelper<
           // TypeHashV<Base>,
            TargetType::VERSION,
            TargetType::StreamsStart,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<typename TargetType::BlockID>,
            TypeHashV<int32_t>,
            TypeHashV<Metadata>
    >;
};


template <typename Types>
struct TypeHash<bt::BranchNode<Types> > {

    using Node = bt::BranchNode<Types>;

    static constexpr uint64_t Value = HashHelper<
            TypeHashV<typename Node::Base>,
            Node::VERSION,
            false,
            TypeHashV<typename Types::Name>
    >;
};


}
