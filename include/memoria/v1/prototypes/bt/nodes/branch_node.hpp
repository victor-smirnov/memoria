
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

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/tools/reflection.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/tools/packed_tools.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node_so.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {
namespace bt        {

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

    static const int32_t StreamsStart = 1;

private:
    Header header_;

    int32_t root_;
    int32_t leaf_;
    int32_t level_;

    BlockID next_leaf_id_;

    BlockID parent_id_;
    int32_t parent_idx_;

    int32_t alignment_gap_;

    // TODO use alignof
    PackedAllocator allocator_;

    static_assert(std::is_pod<PackedAllocator>::value, "PackedAllocator must be a POD tpye");

public:

    enum {METADATA = 0};

    using Me = TreeNodeBase<Metadata, Header>;

    TreeNodeBase() = default;

    Header& header() {return header_;}
    const Header& header() const {return header_;}

    Header* as_header() {return &header_;}
    const Header* as_header() const {return &header_;}

    BlockID& id() {return header_.id();}
    const BlockID& id() const {return header_.id();}

    BlockID& uuid() {return header_.uuid();}
    const BlockID& uuid() const {return header_.uuid();}



    uint64_t ctr_type_hash() const {
        return header_.ctr_type_hash();
    }

    uint64_t block_type_hash() const {
        return header_.block_type_hash();
    }

    void init() {
        header_.init();
    }

    inline bool is_root() const {
        return root_;
    }

    void set_root(bool root) {
        root_ = root;
    }

    inline bool is_leaf() const {
        return leaf_;
    }

    void set_leaf(bool leaf) {
        leaf_ = leaf;
    }

    const int32_t& level() const
    {
        return level_;
    }

    int32_t& level()
    {
        return level_;
    }

    const BlockID& next_leaf_id() const {
        return next_leaf_id_;
    }

    BlockID& next_leaf_id() {
        return next_leaf_id_;
    }

    const BlockID& parent_id() const
    {
        return parent_id_;
    }

    const int32_t& parent_idx() const
    {
        return parent_idx_;
    }

    BlockID& parent_id()
    {
        return parent_id_;
    }

    int32_t& parent_idx()
    {
        return parent_idx_;
    }

    PackedAllocator* allocator() {
        return &allocator_;
    }

    const PackedAllocator* allocator() const {
        return &allocator_;
    }

    bool has_root_metadata() const
    {
        return allocator()->element_size(METADATA) >= (int)sizeof(Metadata);
    }

    const Metadata& root_metadata() const
    {
        return *allocator()->template get<Metadata>(METADATA);
    }

    Metadata& root_metadata()
    {
        MEMORIA_V1_ASSERT_TRUE(!allocator_.is_empty(METADATA));
        return *allocator()->template get<Metadata>(METADATA);
    }

    void setMetadata(const Metadata& meta)
    {
        if (!has_root_metadata())
        {
            OOM_THROW_IF_FAILED(allocator_.template allocate<Metadata>(METADATA), MMA1_SRC);
        }

        root_metadata() = meta;
    }

    void clearMetadata() {
        allocator_.free(METADATA);
    }

    bool canConvertToRoot() const
    {
        if (!has_root_metadata())
        {
            const int32_t metadata_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
            return allocator_.free_space() >= metadata_size;
        }
        else {
            return true;
        }
    }



    bool shouldBeMergedWithSiblings() const
    {
        int32_t client_area = allocator_.client_area();
        int32_t used        = allocator_.allocated();

        return used < client_area / 2;
    }

public:

    void initAllocator(int32_t entries)
    {
        int32_t block_size = this->header().memory_block_size();
        MEMORIA_V1_ASSERT(block_size, >, (int)sizeof(Me) + PackedAllocator::my_size());

        allocator_.allocatable().setTopLevelAllocator();
        OOM_THROW_IF_FAILED(allocator_.init(block_size - sizeof(Me) + PackedAllocator::my_size(), entries), MMA1_SRC);
    }

    void transferDataTo(Me* other) const
    {
        for (int32_t c = 0; c < StreamsStart; c++)
        {
            other->allocator_.importBlock(c, &allocator_, c);
        }
    }

    void resizePage(int32_t new_size)
    {
        header_.memory_block_size() = new_size;
        allocator_.resizeBlock(new_size - sizeof(Me) + PackedAllocator::my_size());
    }

public:

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        header_.generateDataEvents(handler);

        handler->value("ROOT",  &root_);
        handler->value("LEAF",  &leaf_);
        handler->value("LEVEL", &level_);

        handler->value("NEXT_LEAF_ID_", &next_leaf_id_);

        handler->value("PARENT_ID", &parent_id_);
        handler->value("PARENT_IDX", &parent_idx_);

        allocator()->generateDataEvents(handler);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            meta.generateDataEvents(handler);
        }
    }


    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        header_.template serialize<FieldFactory>(buf);

        FieldFactory<int32_t>::serialize(buf, root_);
        FieldFactory<int32_t>::serialize(buf, leaf_);
        FieldFactory<int32_t>::serialize(buf, level_);

        FieldFactory<BlockID>::serialize(buf, next_leaf_id_);

        FieldFactory<BlockID>::serialize(buf, parent_id_);
        FieldFactory<int32_t>::serialize(buf, parent_idx_);

        allocator()->serialize(buf);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::serialize(buf, meta);
        }
    }


    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        header_.template deserialize<FieldFactory>(buf);

        FieldFactory<int32_t>::deserialize(buf, root_);
        FieldFactory<int32_t>::deserialize(buf, leaf_);
        FieldFactory<int32_t>::deserialize(buf, level_);

        FieldFactory<BlockID>::deserialize(buf, next_leaf_id_);

        FieldFactory<BlockID>::deserialize(buf, parent_id_);
        FieldFactory<int32_t>::deserialize(buf, parent_idx_);

        allocator()->deserialize(buf);

        if (has_root_metadata())
        {
            Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::deserialize(buf, meta);
        }
    }

    void copyFrom(const Me* block)
    {
        header_.copyFrom(block);

        this->set_root(block->is_root());
        this->set_leaf(block->is_leaf());

        this->level()       = block->level();

        this->next_leaf_id() = block->next_leaf_id();

        this->parent_id()   = block->parent_id();
        this->parent_idx()  = block->parent_idx();

        //FIXME: copy allocator?
        //FIXME: copy root metadata ?
    }
};







template <
    typename Types
>
class BranchNode: public Types::NodeBase
{
    static const int32_t  BranchingFactor = PackedTreeBranchingFactor;

    using MyType = BranchNode<Types>;

public:
    static const uint32_t VERSION = 1;

    static const bool Leaf = false;

    using Base = typename Types::NodeBase;

public:

    using TypesT = Types;

    template <typename CtrT, typename NodeT>
    using NodeSparseObject = BranchNodeSO<CtrT, NodeT>;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::ID                                                  Value;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList    = typename Types::BranchStreamsStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            FlattenBranchTree<BranchSubstreamsStructList>, Base::StreamsStart
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList>;


    static const int32_t Streams            = ListSize<BranchSubstreamsStructList>;

    static const int32_t Substreams         = Dispatcher::Size;

    static const int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static const int32_t ValuesBlockIdx     = SubstreamsEnd;

    BranchNode() = default;

private:
    struct InitFn {
        uint64_t active_streams_;

        InitFn(int64_t active_streams): active_streams_(active_streams) {}

        int32_t block_size(int32_t items_number) const
        {
            return MyType::block_size(items_number, active_streams_);
        }

        int32_t max_elements(int32_t block_size)
        {
            return block_size;
        }
    };

public:

    static int32_t free_space(int32_t block_size, bool root)
    {
        int32_t fixed_block_size  = block_size - sizeof(MyType) + PackedAllocator::my_size();
        int32_t client_area = PackedAllocator::client_area(fixed_block_size, SubstreamsStart + Substreams + 1);

        return client_area - root * PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(typename Types::Metadata)) - 200;
    }




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
        void stream(Node* obj, int32_t tree_size, uint64_t active_streams)
        {
            if (active_streams & (1 << StreamIndex))
            {
                size_ += Node::block_size(tree_size);
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
                size_ += Node::block_size(size);
            }
        }
    };

public:
    static int32_t block_size(int32_t tree_size, uint64_t active_streams = -1)
    {
        TreeSizeFn fn;

        processSubstreamGroupsStatic(fn, tree_size, active_streams);

        int32_t tree_block_size     = fn.size_;
        int32_t array_block_size    = PackedAllocatable::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }


    static int32_t block_size(const Position& sizes, int32_t values_size)
    {
        TreeSize2Fn fn;

        processSubstreamGroupsStatic(fn, &sizes);

        int32_t tree_block_size     = fn.size_;
        int32_t array_block_size    = PackedAllocatable::roundUpBytesToAlignmentBlocks(values_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }

public:
    static int32_t max_tree_size1(int32_t block_size, uint64_t active_streams = -1)
    {
        return FindTotalElementsNumber2(block_size, InitFn(active_streams));
    }

public:
    static int32_t max_tree_size_for_block(int32_t block_size, bool root)
    {
        int32_t fixed_block_size = MyType::free_space(block_size, root);
        return max_tree_size1(fixed_block_size);
    }

    void prepare()
    {
        Base::initAllocator(SubstreamsStart + Substreams + 1);
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

    int32_t size() const
    {
        SizeFn fn;
        //FIXME: use correct procedure to get number of children
        Dispatcher::dispatch(0, allocator(), fn);
        return fn.size_;
    }


    struct SerializeFn {
        template <typename StreamObj, typename SerializationData>
        void stream(const StreamObj* stream, SerializationData* buf)
        {
            stream->serialize(*buf);
        }
    };

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);

        int32_t size = this->size();

        FieldFactory<Value>::serialize(buf, values(), size);
    }

    struct DeserializeFn {
        template <typename StreamObj, typename DeserializationData>
        void stream(StreamObj* obj, DeserializationData* buf)
        {
            obj->deserialize(*buf);
        }
    };

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);

        int32_t size = this->size();

        FieldFactory<Value>::deserialize(buf, values(), size);
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

        virtual int32_t serialize(const BlockType* block, void* buf) const
        {
            const MyType* me = T2T<const MyType*>(block);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->serialize(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, int32_t buf_size, BlockType* block) const
        {
            MyType* me = T2T<MyType*>(block);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->deserialize(data);
        }

        virtual void resize(const BlockType* block, void* buffer, int32_t new_size) const
        {
            MyType* tgt = T2T<MyType*>(buffer);
            tgt->resizePage(new_size);
        }

        virtual uint64_t block_type_hash() const {
            return MyType::BLOCK_HASH;
        }
    };

    static BlockOperationsPtr<Profile> block_operations() {
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
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
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


}}
