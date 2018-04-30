
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


namespace memoria {
namespace v1 {
namespace bt        {

template <
        template <typename> class,
        typename
>
class NodePageAdaptor;



template <typename Metadata, typename Base_>
class TreeNodeBase: public Base_ {
public:
    static const uint32_t VERSION = 1;
    typedef Base_                               Base;

    typedef typename Base::ID                   ID;


//    static_assert(std::is_trivial<Metadata>::value, "TreeNodeBase: metadata must be a trivial type");
//    static_assert(std::is_trivial<Base_>::value,    "TreeNodeBase: base must be a trivial type");
//    static_assert(std::is_trivial<ID>::value,       "TreeNodeBase: ID must be a trivial type");

    static const int32_t StreamsStart               = 1;

private:

    int32_t root_;
    int32_t leaf_;
    int32_t level_;

    ID  next_leaf_id_;

    ID  parent_id_;
    int32_t parent_idx_;

    int32_t alignment_gap_;

    PackedAllocator allocator_;

public:

    enum {METADATA = 0};

    typedef TreeNodeBase<Metadata, Base>        Me;

    TreeNodeBase() = default;

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

    const ID& next_leaf_id() const {
        return next_leaf_id_;
    }

    ID& next_leaf_id() {
        return next_leaf_id_;
    }

    const ID& parent_id() const
    {
        return parent_id_;
    }

    const int32_t& parent_idx() const
    {
        return parent_idx_;
    }

    ID& parent_id()
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
            const int32_t metadata_size = PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
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
        int32_t page_size = this->page_size();
        MEMORIA_V1_ASSERT(page_size, >, (int)sizeof(Me) + PackedAllocator::my_size());

        allocator_.setTopLevelAllocator();
        OOM_THROW_IF_FAILED(allocator_.init(page_size - sizeof(Me) + PackedAllocator::my_size(), entries), MMA1_SRC);
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
        this->page_size() = new_size;
        allocator_.resizeBlock(new_size - sizeof(Me) + PackedAllocator::my_size());
    }

public:

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

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


    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<int32_t>::serialize(buf, root_);
        FieldFactory<int32_t>::serialize(buf, leaf_);
        FieldFactory<int32_t>::serialize(buf, level_);

        FieldFactory<ID>::serialize(buf, next_leaf_id_);

        FieldFactory<ID>::serialize(buf, parent_id_);
        FieldFactory<int32_t>::serialize(buf, parent_idx_);

        allocator()->serialize(buf);

        if (has_root_metadata())
        {
            const Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::serialize(buf, meta);
        }
    }


    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<int32_t>::deserialize(buf, root_);
        FieldFactory<int32_t>::deserialize(buf, leaf_);
        FieldFactory<int32_t>::deserialize(buf, level_);

        FieldFactory<ID>::deserialize(buf, next_leaf_id_);

        FieldFactory<ID>::deserialize(buf, parent_id_);
        FieldFactory<int32_t>::deserialize(buf, parent_idx_);

        allocator()->deserialize(buf);

        if (has_root_metadata())
        {
            Metadata& meta = this->root_metadata();
            FieldFactory<Metadata>::deserialize(buf, meta);
        }
    }

    void copyFrom(const Me* page)
    {
        Base::copyFrom(page);

        this->set_root(page->is_root());
        this->set_leaf(page->is_leaf());

        this->level()       = page->level();

        this->next_leaf_id() = page->next_leaf_id();

        this->parent_id()   = page->parent_id();
        this->parent_idx()  = page->parent_idx();

        //FIXME: copy allocator?
        //FIXME: copy root metadata ?
    }
};







template <
    typename Types
>
class BranchNode: public Types::template TreeNodeBaseTF<typename Types::Metadata, typename Types::NodeBase>
{

//    static_assert(
//            std::is_trivial<TreeNodeBase<typename Types::Metadata, typename Types::NodeBase>>::value,
//            "TreeNodeBase must be a trivial type"
//            );

    static const int32_t  BranchingFactor                                           = PackedTreeBranchingFactor;

    typedef BranchNode<Types>                                                   MyType;

public:
    static const uint32_t VERSION                                                   = 1;

    static const bool Leaf                                                      = false;

    using Base = typename Types::template TreeNodeBaseTF<typename Types::Metadata, typename Types::NodeBase>;

public:

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::ID                                                  Value;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList    = typename Types::BranchStreamsStructList;
    using LeafSubstreamsStructList      = typename Types::LeafStreamsStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            FlattenBranchTree<BranchSubstreamsStructList>, Base::StreamsStart
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList, 0>;

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;

    template <int32_t StreamIdx>
    using StreamStartIdx = IntValue<
        v1::list_tree::LeafCountInf<BranchSubstreamsStructList, IntList<StreamIdx>>
    >;

    template <int32_t StreamIdx>
    using StreamSize = IntValue<
            v1::list_tree::LeafCountSup<BranchSubstreamsStructList, IntList<StreamIdx>> -
            v1::list_tree::LeafCountInf<BranchSubstreamsStructList, IntList<StreamIdx>>
    >;



    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            v1::list_tree::LeafCountInf<BranchSubstreamsStructList, SubstreamsPath>,
            v1::list_tree::LeafCountSup<BranchSubstreamsStructList, SubstreamsPath>
    >;

    template <int32_t SubstreamIdx>
    using LeafPathT = typename v1::list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <int32_t SubstreamIdx>
    using BranchPathT = typename v1::list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;


    static const int32_t Streams            = ListSize<BranchSubstreamsStructList>;

    static const int32_t Substreams         = Dispatcher::Size;

    static const int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static const int32_t ValuesBlockIdx     = SubstreamsEnd;


    template <typename LeafPath>
    using BuildBranchPath = typename v1::list_tree::BuildTreePath<
            BranchSubstreamsStructList,
            v1::list_tree::LeafCountInf<LeafSubstreamsStructList, LeafPath, 2> -
                FindLocalLeafOffsetV<
                    FlattenLeafTree<LeafSubstreamsStructList>,
                    v1::list_tree::LeafCount<LeafSubstreamsStructList, LeafPath>
                >::Value
    >::Type;


    BranchNode() = default;


    template <typename LeafPath>
    static const int32_t translateLeafIndexToBranchIndex(int32_t leaf_index)
    {
        return LeafToBranchIndexTranslator<LeafSubstreamsStructList, LeafPath, 0>::translate(leaf_index);
    }

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

    static int32_t free_space(int32_t page_size, bool root)
    {
        int32_t block_size  = page_size - sizeof(MyType) + PackedAllocator::my_size();
        int32_t client_area = PackedAllocator::client_area(block_size, SubstreamsStart + Substreams + 1);

        return client_area - root * PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
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


    int32_t capacity(uint64_t active_streams) const
    {
        int32_t free_space  = MyType::free_space(Base::page_size(), Base::is_root());
        int32_t max_size    = max_tree_size1(free_space, active_streams);
        int32_t cap         = max_size - size();

        return cap >= 0 ? cap : 0;
    }

    int32_t capacity() const
    {
        return capacity(active_streams());
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
        int32_t array_block_size    = PackedAllocator::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }


    static int32_t block_size(const Position& sizes, int32_t values_size)
    {
        TreeSize2Fn fn;

        processSubstreamGroupsStatic(fn, &sizes);

        int32_t tree_block_size     = fn.size_;
        int32_t array_block_size    = PackedAllocator::roundUpBytesToAlignmentBlocks(values_size * sizeof(Value));
        int32_t client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }

private:
    static int32_t max_tree_size1(int32_t block_size, uint64_t active_streams = -1)
    {
        return FindTotalElementsNumber2(block_size, InitFn(active_streams));
    }

public:
    static int32_t max_tree_size_for_block(int32_t page_size, bool root)
    {
        int32_t block_size = MyType::free_space(page_size, root);
        return max_tree_size1(block_size);
    }

    void prepare()
    {
        Base::initAllocator(SubstreamsStart + Substreams + 1);
    }


    struct LayoutFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename StreamType>
        void stream(StreamType*, PackedAllocator* allocator, uint64_t active_streams)
        {
            if (active_streams & (1 << Idx))
            {
                if (allocator->is_empty(AllocatorIdx))
                {
                    OOM_THROW_IF_FAILED(allocator->template allocateEmpty<StreamType>(AllocatorIdx), MMA1_SRC);
                }
            }
        }
    };


    void layout(uint64_t active_streams)
    {
        Dispatcher::dispatchAllStatic(LayoutFn(), allocator(), active_streams);
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

//    void init0(int32_t block_size, uint64_t active_streams)
//    {
//      Base::initAllocator(StreamsStart + Streams + 1);
//
//        int32_t tree_size = 0;//max_tree_size(block_size, active_streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), tree_size, allocator(), active_streams);
//
//        allocator()->template allocateArrayBySize<Value>(ValuesBlockIdx, tree_size);
//    }

    static int32_t client_area(int32_t block_size)
    {
        int32_t allocator_block_size = block_size - sizeof(MyType) + PackedAllocator::my_size();
        return PackedAllocator::client_area(allocator_block_size, Streams);
    }

    int32_t total_size() const
    {
        return allocator()->allocated();
    }


    void clearUnused() {}

    struct ReindexFn {
        template <typename Tree>
        void stream(Tree* tree)
        {
            OOM_THROW_IF_FAILED(tree->reindex(), MMA1_SRC);
        }
    };

    void reindex()
    {
        Dispatcher::dispatchNotEmpty(allocator(), ReindexFn());
    }

    struct CheckFn {
        template <typename Tree>
        void stream(const Tree* tree)
        {
            tree->check();
        }
    };

    void check() const
    {
        Dispatcher::dispatchNotEmpty(allocator(), CheckFn());
    }


    template <typename TreeType>
    struct TransferToFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, TreeType* other)
        {
            auto allocator          = tree->allocator();
            auto other_allocator    = other->allocator();

            other_allocator->importBlock(AllocatorIdx, allocator, AllocatorIdx);
        }
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        Base::transferDataTo(other);

        Dispatcher::dispatchNotEmpty(allocator(), TransferToFn<TreeType>(), other);

        int32_t size = this->size();

        int32_t requested_values_block_size = size * sizeof(Value);
        other->allocator()->resizeBlock(ValuesBlockIdx, requested_values_block_size);

        const auto* my_values   = values();
        auto* other_values      = other->values();

        for (int32_t c = 0; c < size; c++)
        {
            other_values[c] = my_values[c];
        }
    }

    struct ClearFn {
        template <typename Tree>
        void stream(Tree* tree, int32_t start, int32_t end)
        {
            tree->clear(start, end);
        }
    };

    void clear(int32_t start, int32_t end)
    {
        Dispatcher::dispatchNotEmpty(allocator(), ClearFn(), start, end);

        Value* values   = this->values();

        for (int32_t c = start; c < end; c++)
        {
            values[c] = 0;
        }
    }


    int32_t data_size() const
    {
        return sizeof(MyType) + this->getDataSize();
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

    int32_t size(int32_t stream) const
    {
        SizeFn fn;
        Dispatcher::dispatch(stream, allocator(), fn);
        return fn.size_;
    }

    struct SizesFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, Position& pos)
        {
            pos.value(StreamIdx) = tree->size();
        }
    };

    Position sizes() const
    {
        Position pos;
        processSubstreamGroups(SizesFn(), pos);
        return pos;
    }

    struct SizeSumsFn {
        template <int32_t ListIdx, typename Tree>
        void stream(const Tree* tree, Position& sizes)
        {
            sizes[ListIdx] = tree != nullptr ? tree->sum(0) : 0;
        }
    };

    Position size_sums() const
    {
        Position sums;
        processStreamsStart(SizeSumsFn(), sums);
        return sums;
    }


    bool isEmpty() const
    {
        return size() == 0;
    }

    bool isAfterEnd(const Position& idx, uint64_t active_streams) const
    {
        return idx.get() >= size();
    }

    struct SetChildrenCountFn {
        template <typename Tree>
        void stream(Tree* tree, int32_t size)
        {
            tree->size() = size;
        }
    };

    void set_children_count1(int32_t map_size)
    {
        Dispatcher::dispatchNotEmpty(allocator(), SetChildrenCountFn(), map_size);
    }



    struct InsertFn {
        template <int32_t Idx, typename StreamType>
        void stream(StreamType* obj, int32_t idx, const BranchNodeEntry& keys)
        {
            OOM_THROW_IF_FAILED(obj->insert(idx, std::get<Idx>(keys)), MMA1_SRC);
        }
    };

    OpStatus insert(int32_t idx, const BranchNodeEntry& keys, const Value& value)
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, size);

        Dispatcher::dispatchNotEmpty(allocator(), InsertFn(), idx, keys);

        int32_t requested_block_size = (size + 1) * sizeof(Value);

        if (isFail(allocator()->resizeBlock(ValuesBlockIdx, requested_block_size))) {
            return OpStatus::FAIL;
        }

        Value* values = this->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value;

        return OpStatus::OK;
    }




    struct InsertSpaceFn {
        template <typename Tree>
        void stream(Tree* tree, int32_t room_start, int32_t room_length)
        {
            tree->insertSpace(room_start, room_length);
        }
    };

    void insertSpace(const Position& from_pos, const Position& length_pos)
    {
        int32_t room_start  = from_pos.get();
        int32_t room_length = length_pos.get();

        insertSpace(0, room_start, room_length);
    }


    void insertSpace(int32_t stream, int32_t room_start, int32_t room_length)
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(room_start, >=, 0);
        MEMORIA_V1_ASSERT(room_start, <=, size);
        MEMORIA_V1_ASSERT(stream, ==, 0);

        Dispatcher::dispatchNotEmpty(allocator(), InsertSpaceFn(), room_start, room_length);

        insertValuesSpace(size, room_start, room_length);
    }

    void insertSpace(int32_t room_start, int32_t room_length)
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(room_start, >=, 0);
        MEMORIA_V1_ASSERT(room_start, <=, size);

        Dispatcher::dispatchNotEmpty(allocator(), InsertSpaceFn(), room_start, room_length);

        insertValuesSpace(size, room_start, room_length);
    }


    void insertValuesSpace(int32_t old_size, int32_t room_start, int32_t room_length)
    {
        MEMORIA_V1_ASSERT(room_start, >=, 0);
        MEMORIA_V1_ASSERT(room_start, <=, old_size);

        int32_t requested_block_size = (old_size + room_length) * sizeof(Value);

        OOM_THROW_IF_FAILED(toOpStatus(allocator()->resizeBlock(ValuesBlockIdx, requested_block_size)), MMA1_SRC);

        Value* values = this->values();

        CopyBuffer(values + room_start, values + room_start + room_length, old_size - room_start);

        for (int32_t c = room_start; c < room_start + room_length; c++)
        {
            values[c] = Value();
        }
    }

    void insertValues(int32_t old_size, int32_t idx, int32_t length, std::function<Value()> provider)
    {
        insertValuesSpace(old_size, idx, length);

        Value* values = this->values();

        for (int32_t c = idx; c < idx + length; c++)
        {
            values[c] = provider();
        }
    }



    struct RemoveSpaceFn {
        template <typename Tree>
        void stream(Tree* tree, int32_t room_start, int32_t room_end)
        {
            OOM_THROW_IF_FAILED(tree->removeSpace(room_start, room_end), MMA1_SRC);
        }
    };

    void removeSpace(const Position& from_pos, const Position& end_pos)
    {
        this->removeSpace(from_pos.get(), end_pos.get());
    }

    void removeSpaceAcc(int32_t room_start, int32_t room_end)
    {
        removeSpace(room_start, room_end);
    }

    void removeSpace(int32_t room_start, int32_t room_end)
    {
        int32_t old_size = this->size();

        Dispatcher::dispatchNotEmpty(allocator(), RemoveSpaceFn(), room_start, room_end);

        Value* values = this->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        this->reindex();

        MEMORIA_V1_ASSERT(old_size, >=, room_end - room_start);

        int32_t requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);
        OOM_THROW_IF_FAILED(toOpStatus(allocator()->resizeBlock(values, requested_block_size)), MMA1_SRC);
    }

    void removeSpace(int32_t stream, int32_t room_start, int32_t room_end)
    {
        removeSpace(room_start, room_end);
    }



    struct CopyToFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, MyType* other, int32_t copy_from, int32_t count, int32_t copy_to)
        {
            MEMORIA_V1_ASSERT_TRUE(!other->allocator()->is_empty(AllocatorIdx));

            tree->copyTo(other->allocator()->template get<Tree>(AllocatorIdx), copy_from, count, copy_to);
        }
    };


    void copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
    {
        MEMORIA_V1_ASSERT(copy_from + count, <=, size());

        Dispatcher::dispatchNotEmpty(allocator(), CopyToFn(), other, copy_from, count, copy_to);

        CopyBuffer(this->values() + copy_from, other->values() + copy_to, count);
    }



    struct CanMergeWithFn {
        int32_t mem_used_ = 0;

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, const MyType* other)
        {
            if (tree != nullptr)
            {
                if (other->allocator()->is_empty(AllocatorIdx))
                {
                    mem_used_ += tree->block_size();
                }
                else {
                    const Tree* other_tree = other->allocator()->template get<Tree>(AllocatorIdx);
                    mem_used_ += tree->block_size(other_tree);
                }
            }
            else {
                if (!other->allocator()->is_empty(AllocatorIdx))
                {
                    int32_t element_size = other->allocator()->element_size(AllocatorIdx);
                    mem_used_ += element_size;
                }
            }
        }
    };

    bool canBeMergedWith(const MyType* other) const
    {
        CanMergeWithFn fn;
        Dispatcher::dispatchAll(allocator(), fn, other);

        int32_t client_area = this->allocator()->client_area();

        int32_t my_data_size    = this->allocator()->element_size(ValuesBlockIdx);
        int32_t other_data_size = other->allocator()->element_size(ValuesBlockIdx);

        fn.mem_used_ += my_data_size;
        fn.mem_used_ += other_data_size;

        return client_area >= fn.mem_used_;
    }

    struct MergeWithFn {
        template <int32_t AllocatorIdx, int32_t ListIdx, typename Tree>
        void stream(Tree* tree, MyType* other)
        {
            int32_t size = tree->size();

            if (size > 0)
            {
                if (other->allocator()->is_empty(AllocatorIdx))
                {
                    OOM_THROW_IF_FAILED(other->allocator()->template allocateEmpty<Tree>(AllocatorIdx), MMA1_SRC);
                }

                Tree* other_tree = other->allocator()->template get<Tree>(AllocatorIdx);

                OOM_THROW_IF_FAILED(tree->mergeWith(other_tree), MMA1_SRC);
            }
        }
    };

    void mergeWith(MyType* other)
    {
        int32_t other_size  = other->size();
        int32_t my_size     = this->size();

        Dispatcher::dispatchNotEmpty(allocator(), MergeWithFn(), other);

        int32_t other_values_block_size          = other->allocator()->element_size(ValuesBlockIdx);
        int32_t required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            OOM_THROW_IF_FAILED(toOpStatus(other->allocator()->resizeBlock(other->values(), required_other_values_block_size)), MMA1_SRC);
        }

        CopyBuffer(values(), other->values() + other_size, my_size);
    }


    struct SplitToFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree* tree, MyType* other, int32_t idx)
        {
            int32_t size = tree->size();
            if (size > 0)
            {
                Tree* other_tree = other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                OOM_THROW_IF_FAILED(tree->splitTo(other_tree, idx), MMA1_SRC);
            }
        }
    };


    BranchNodeEntry splitTo(MyType* other, int32_t split_idx)
    {
        int32_t size        = this->size();
        int32_t remainder   = size - split_idx;

        MEMORIA_V1_ASSERT(split_idx, <=, size);

        BranchNodeEntry result;
//        this->sums(split_idx, size, result);

        Dispatcher::dispatchNotEmpty(allocator(), SplitToFn(), other, split_idx);

        OOM_THROW_IF_FAILED(other->allocator()->template allocateArrayBySize<Value>(ValuesBlockIdx, remainder), MMA1_SRC);

        Value* other_values = other->values();
        Value* my_values    = this->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);

        return result;
    }

    void reindexAll(int32_t from, int32_t to)
    {
        reindex();
    }


    struct KeysAtFn {
        template <int32_t Idx, typename Tree>
        void stream(const Tree* tree, int32_t idx, BranchNodeEntry* acc)
        {
//            const int32_t Blocks = Tree::Blocks;

//            for (int32_t c = 0; c < Blocks; c++)
//            {
//                std::get<Idx>(*acc)[c] = tree->value(c, idx);
//            }

            std::get<Idx>(*acc) = tree->get_values(idx);
        }
    };

    BranchNodeEntry keysAt(int32_t idx) const
    {
        BranchNodeEntry acc;

        Dispatcher::dispatchNotEmpty(allocator(), KeysAtFn(), idx, &acc);

        return acc;
    }

    struct SetKeysFn {
        template <int32_t Idx, typename Tree>
        void stream(Tree* tree, int32_t idx, const BranchNodeEntry& keys)
        {
            for (int32_t c = 0; c < Tree::Blocks; c++)
            {
                auto k = std::get<Idx>(keys)[c];
                tree->value(c, idx) = k;
            }

            tree->reindex();
        }
    };

    void setKeys(int32_t idx, const BranchNodeEntry& keys)
    {
        Dispatcher::dispatchNotEmpty(allocator(), SetKeysFn(), idx, keys);
    }


    Value& value(int32_t idx)
    {
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, size());

        return *(values() + idx);
    }

    const Value& value(int32_t idx) const
    {
        if (idx >= size() || idx < 0) {
            int a = 0; a++;
        }

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <, size());

        return *(values() + idx);
    }

    struct SumsFn {
        template <int32_t Idx, typename StreamType>
        void stream(const StreamType* obj, int32_t start, int32_t end, BranchNodeEntry& accum)
        {
            obj->sums(start, end, std::get<Idx>(accum));
        }

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamType>
        void stream(const StreamType* obj, const Position& start, const Position& end, BranchNodeEntry& accum)
        {
            obj->sums(start[StreamIdx], end[StreamIdx], std::get<AllocatorIdx - SubstreamsStart>(accum));
        }

        template <int32_t Idx, typename StreamType>
        void stream(const StreamType* obj, BranchNodeEntry& accum)
        {
            obj->sums(std::get<Idx>(accum));
        }

        template <typename StreamType>
        void stream(const StreamType* obj, int32_t block, int32_t start, int32_t end, int64_t& accum)
        {
            accum += obj->sum(block, start, end);
        }
    };

    void sums(int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), start, end, sums);
    }

    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processSubstreamGroups(SumsFn(), start, end, sums);
    }

    void sums(BranchNodeEntry& sums) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
    }

    BranchNodeEntry sums() const
    {
        BranchNodeEntry sums;
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
        return sums;
    }

    void sum(int32_t stream, int32_t block_num, int32_t start, int32_t end, int64_t& accum) const
    {
        Dispatcher::dispatch(stream, allocator(), SumsFn(), block_num, start, end, accum);
    }

    template <typename SubstreamPath>
    void sum_substream(int32_t block_num, int32_t start, int32_t end, int64_t& accum) const
    {
        processStream<SubstreamPath>(SumsFn(), block_num, start, end, accum);
    }

    template <typename LeafSubstreamPath>
    void sum_substream_for_leaf_path(int32_t leaf_block_num, int32_t start, int32_t end, int64_t& accum) const
    {
        using BranchPath = BuildBranchPath<LeafSubstreamPath>;

        const int32_t index = MyType::translateLeafIndexToBranchIndex<LeafSubstreamPath>(leaf_block_num);

        processStream<BranchPath>(SumsFn(), index, start, end, accum);
    }


    struct MaxFn {
        template <int32_t Idx, typename StreamType>
        void stream(const StreamType* obj, BranchNodeEntry& accum)
        {
            obj->max(std::get<Idx>(accum));
        }
    };

    void max(BranchNodeEntry& entry) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), MaxFn(), entry);
    }



    template <typename V>
    void forAllValues(int32_t start, int32_t end, std::function<void (const V&, int32_t)> fn) const
    {
        const Value* v = this->values();
        for (int32_t c = start; c < end; c++)
        {
            fn(v[c], c);
        }
    }

    template <typename V>
    void forAllValues(int32_t start, std::function<void (const V&, int32_t)> fn) const
    {
        auto end = this->size();
        forAllValues(start, end, fn);
    }

    template <typename V>
    void forAllValues(std::function<void (const V&, int32_t)> fn) const
    {
        forAllValues(0, fn);
    }

    template <typename V>
    std::vector<V> values_as_vector(int32_t start, int32_t end) const
    {
        std::vector<V> vals;

        const auto* vv = values();

        for (int32_t c = start; c < end; c++)
        {
            vals.emplace_back(vv[c]);
        }

        return vals;
    }

    template <typename V>
    std::vector<V> values_as_vector() const
    {
        return values_as_vector<V>(0, size());
    }


    int32_t find_child_idx(const Value& id) const
    {
    	const Value* v = this->values();
    	int32_t size = this->size();

    	for (int32_t c = 0; c < size; c++)
    	{
    		if (v[c] == id) {
    			return c;
    		}
    	}

    	return -1;
    }


    template <typename SubstreamPath>
    auto substream()
    {
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }


    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }

    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }


    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto processStreamByIdx(Fn&& fn, Args&&... args) const
    {
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t StreamIdx, typename Fn, typename... Args>

    auto processStreamByIdx(Fn&& fn, Args&&... args)
    {
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const
    {
        using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    {
        using Subset = StreamsStartSubset<BranchSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const
    {
        using Subset = StreamsStartSubset<BranchSubstreamsStructList>;

        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    struct UpdateUpFn {
        template <int32_t Idx, typename StreamType>
        void stream(StreamType* tree, int32_t idx, const BranchNodeEntry& accum)
        {
            OOM_THROW_IF_FAILED(tree->setValues(idx, std::get<Idx>(accum)), MMA1_SRC);
        }
    };


    void updateUp(int32_t idx, const BranchNodeEntry& keys)
    {
        Dispatcher::dispatchNotEmpty(allocator(), UpdateUpFn(), idx, keys);
    }


    //FIXME: remove?
    BranchNodeEntry keys(int32_t pos) const
    {
        BranchNodeEntry value;
        return sums(pos, pos + 1, value);
        return value;
    }


    bool checkCapacities(const Position& sizes) const
    {
        return capacity() >= sizes.get();
    }

    struct DumpFn {
        template <typename Tree>
        void stream(const Tree* tree)
        {
            tree->dump(std::cout);
        }
    };

    void dump() const
    {
        Dispatcher::dispatchNotEmpty(allocator(), DumpFn());
        dumpValues();
    }

    void dumpValues() const
    {
        int32_t size = this->size();
        auto values = this->values();

        std::cout << "Values:" << std::endl;
        for (int32_t c = 0; c < size; c++)
        {
            std::cout << c << " " << values[c] << std::endl;
        }
    }

    struct GenerateDataEventsFn {
        template <int32_t Idx, typename Tree>
        void stream(const Tree* tree, IPageDataEventHandler* handler)
        {
            tree->generateDataEvents(handler);
        }
    };

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Dispatcher::dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);

        handler->startGroup("TREE_VALUES", size());

        for (int32_t idx = 0; idx < size(); idx++)
        {
            ValueHelper<Value>::setup(handler, "CHILD_ID", value(idx));
        }

        handler->endGroup();
    }

    struct SerializeFn {
        template <typename StreamObj>
        void stream(const StreamObj* stream, SerializationData* buf)
        {
            stream->serialize(*buf);
        }
    };

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);

        int32_t size = this->size();

        FieldFactory<Value>::serialize(buf, values(), size);
    }

    struct DeserializeFn {
        template <typename StreamObj>
        void stream(StreamObj* obj, DeserializationData* buf)
        {
            obj->deserialize(*buf);
        }
    };

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);

        int32_t size = this->size();

        FieldFactory<Value>::deserialize(buf, values(), size);
    }

    static void InitType() {}
};





template <
    template <typename> class TreeNode,
    typename Types
>
class NodePageAdaptor: public TreeNode<Types>
{
public:

    using MyType = NodePageAdaptor<TreeNode, Types>;
    using Base   = TreeNode<Types>;

    static const uint64_t PAGE_HASH = TypeHashV<Base>;

//    static_assert(std::is_trivial<TreeNode<Types>>::value, "TreeNode must be a trivial type");


    template <
        template <typename> class,
        typename
    >
    friend class NodePageAdaptor;

private:
    static PageMetadataPtr page_metadata_;

public:
    NodePageAdaptor() = default;

    static uint64_t hash() {
        return PAGE_HASH;
    }

    static const PageMetadataPtr& page_metadata() {
        return page_metadata_;
    }

    class PageOperations: public IPageOperations
    {
        virtual ~PageOperations() {}

        virtual int32_t serialize(const void* page, void* buf) const
        {
            const MyType* me = T2T<const MyType*>(page);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->template serialize<FieldFactory>(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, int32_t buf_size, void* page) const
        {
            MyType* me = T2T<MyType*>(page);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->template deserialize<FieldFactory>(data);
        }

        virtual int32_t getPageSize(const void *page) const
        {
            const MyType* me = T2T<const MyType*>(page);
            return me->page_size();
        }

        virtual void resize(const void* page, void* buffer, int32_t new_size) const
        {
//            const MyType* me = T2T<const MyType*>(page);
            MyType* tgt = T2T<MyType*>(buffer);
//
//            tgt->copyFrom(me);
//            tgt->page_size() = new_size;
//            tgt->init();
//
//            me->transferDataTo(tgt);
//
//            tgt->clearUnused();
//            tgt->reindex();

            tgt->resizePage(new_size);
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            handler->startPage("BTREE_NODE", me);
            me->generateDataEvents(handler);
            handler->endPage();
        }

        virtual void generateLayoutEvents(
                        const void* page,
                        const LayoutEventsParams& params,
                        IPageLayoutEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            handler->startPage("BTREE_NODE");
            me->generateLayoutEvents(handler);
            handler->endPage();
        }
    };

    static uint64_t initMetadata()
    {
        Base::InitType();

        if (!page_metadata_)
        {
            int32_t attrs = 0;

            PageOperations* ops = new PageOperations();

            page_metadata_ = metadata_make_shared<PageMetadata>("BTREE_PAGE", attrs, hash(), ops);
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
    template <typename> class TreeNode,
    typename Types
>
PageMetadataPtr NodePageAdaptor<TreeNode, Types>::page_metadata_;

}

template <typename Metadata, typename Base>
struct TypeHash<bt::TreeNodeBase<Metadata, Base>> {
    using TargetType = bt::TreeNodeBase<Metadata, Base>;

    static constexpr uint64_t Value = HashHelper<
            TypeHashV<Base>,
            TargetType::VERSION,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<int32_t>,
            TypeHashV<typename TargetType::ID>,
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
