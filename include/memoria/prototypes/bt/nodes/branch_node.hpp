
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_NODES_BRANCHNODE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_NODES_BRANCHNODE_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_accumulator_handlers.hpp>
#include <memoria/prototypes/bt/tools/bt_substreamgroup_dispatcher.hpp>


namespace memoria   {
namespace bt        {

template <
        template <typename> class,
        typename
>
class NodePageAdaptor;



template <typename Metadata, typename Base_>
class TreeNodeBase: public Base_ {
public:
    static const UInt VERSION = 1;
    typedef Base_                               Base;

    typedef typename Base::ID                   ID;


    static_assert(std::is_trivial<Metadata>::value, "TreeNodeBase: metadata must be a trivial type");
    static_assert(std::is_trivial<Base_>::value,    "TreeNodeBase: base must be a trivial type");
    static_assert(std::is_trivial<ID>::value,       "TreeNodeBase: ID must be a trivial type");

    static const Int StreamsStart               = 1;

private:

    Int root_;
    Int leaf_;
    Int level_;

    ID  next_leaf_id_;

    ID  parent_id_;
    Int parent_idx_;

    Int alignment_gap_;

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

    const Int& level() const
    {
        return level_;
    }

    Int& level()
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

    const Int& parent_idx() const
    {
        return parent_idx_;
    }

    ID& parent_id()
    {
        return parent_id_;
    }

    Int& parent_idx()
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
        return allocator()->element_size(METADATA) >= sizeof(Metadata);
    }

    const Metadata& root_metadata() const
    {
        return *allocator()->template get<Metadata>(METADATA);
    }

    Metadata& root_metadata()
    {
        MEMORIA_ASSERT_TRUE(!allocator_.is_empty(METADATA));
        return *allocator()->template get<Metadata>(METADATA);
    }

    void setMetadata(const Metadata& meta)
    {
        if (!has_root_metadata())
        {
            allocator_.template allocate<Metadata>(METADATA);
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
            const Int metadata_size = PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
            return allocator_.free_space() >= metadata_size;
        }
        else {
            return true;
        }
    }



    bool shouldBeMergedWithSiblings() const
    {
        Int client_area = allocator_.client_area();
        Int used        = allocator_.allocated();

        return used < client_area / 2;
    }

public:

    void initAllocator(Int entries)
    {
        Int page_size = this->page_size();
        MEMORIA_ASSERT(page_size, >, sizeof(Me) + PackedAllocator::my_size());

        allocator_.setTopLevelAllocator();
        allocator_.init(page_size - sizeof(Me) + PackedAllocator::my_size(), entries);
    }

    void transferDataTo(Me* other) const
    {
        for (Int c = 0; c < StreamsStart; c++)
        {
            other->allocator_.importBlock(c, &allocator_, c);
        }
    }

public:

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->value("ROOT",  &root_);
        handler->value("LEAF",  &leaf_);
        handler->value("LEVEL", &level_);

        IDValue next_id(next_leaf_id_);

        handler->value("NEXT_LEAF_ID_", &next_id);

        IDValue parent_id(parent_id_);
        handler->value("PARENT_ID", &parent_id);
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

        FieldFactory<Int>::serialize(buf, root_);
        FieldFactory<Int>::serialize(buf, leaf_);
        FieldFactory<Int>::serialize(buf, level_);

        FieldFactory<ID>::serialize(buf, next_leaf_id_);

        FieldFactory<ID>::serialize(buf, parent_id_);
        FieldFactory<Int>::serialize(buf, parent_idx_);

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

        FieldFactory<Int>::deserialize(buf, root_);
        FieldFactory<Int>::deserialize(buf, leaf_);
        FieldFactory<Int>::deserialize(buf, level_);

        FieldFactory<ID>::deserialize(buf, next_leaf_id_);

        FieldFactory<ID>::deserialize(buf, parent_id_);
        FieldFactory<Int>::deserialize(buf, parent_idx_);

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
class BranchNode: public TreeNodeBase<typename Types::Metadata, typename Types::NodeBase>
{

    static_assert(
            std::is_trivial<TreeNodeBase<typename Types::Metadata, typename Types::NodeBase>>::value,
            "TreeNodeBase must be a trivial type"
            );

    static const Int  BranchingFactor                                           = PackedTreeBranchingFactor;

    typedef BranchNode<Types>                                                   MyType;

public:
    static const UInt VERSION                                                   = 1;

    static const bool Leaf                                                      = false;

    typedef TreeNodeBase<
        typename Types::Metadata,
        typename Types::NodeBase
    >                                                                           Base;

public:

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::ID                                                  Value;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList 	= typename Types::BranchStreamsStructList;
    using LeafSubstreamsStructList 		= typename Types::LeafStreamsStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
    		FlattenBranchTree<BranchSubstreamsStructList>, Base::StreamsStart
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList, 0>;

    template <Int StartIdx, Int EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
    		memoria::list_tree::LeafCountInf<BranchSubstreamsStructList, SubstreamsPath>::Value,
    		memoria::list_tree::LeafCountSup<BranchSubstreamsStructList, SubstreamsPath>::Value
    >;

    static const Int Streams                                                    = ListSize<BranchSubstreamsStructList>::Value;

    static const Int Substreams                                                 = Dispatcher::Size;

    static const Int SubstreamsStart                                            = Dispatcher::AllocatorIdxStart;
    static const Int SubstreamsEnd                                              = Dispatcher::AllocatorIdxEnd;

    static const Int ValuesBlockIdx                                             = SubstreamsEnd;



    template <Int Idx, typename... Args>
    using DispatchRtnFnType = auto(Args...) -> decltype(
            Dispatcher::template dispatch<Idx>(std::declval<Args>()...)
    );

    template <typename... Args>
    using DynDispatchRtnFnType = auto(Args...) -> decltype(
            Dispatcher::template dispatch(std::declval<Args>()...)
    );

    template <Int Idx, typename Fn, typename... T>
    using DispatchRtnType = typename FnTraits<
            DispatchRtnFnType<Idx, PackedAllocator*, Fn, T...>
    >::RtnType;

    template <Int Idx, typename Fn, typename... T>
    using DispatchRtnConstType = typename FnTraits<
            DispatchRtnFnType<Idx, const PackedAllocator*, Fn, T...>
    >::RtnType;

    template <typename Fn, typename... T>
    using DynDispatchRtnType = typename FnTraits<
            DynDispatchRtnFnType<Int, PackedAllocator*, Fn, T...>
    >::RtnType;

    template <typename Fn, typename... T>
    using DynDispatchRtnConstType = typename FnTraits<
            DynDispatchRtnFnType<Int, const PackedAllocator*, Fn, T...>
    >::RtnType;




    template <typename Fn, typename... T>
    using ProcessAllRtnType = typename Dispatcher::template ProcessAllRtnType<Fn, T...>;

    template <typename Fn, typename... T>
    using ProcessAllRtnConstType = typename Dispatcher::template ProcessAllRtnConstType<Fn, T...>;

    template <typename SubstreamsPath, typename Fn, typename... T>
    using ProcessSubstreamsRtnType = typename SubstreamsDispatcher<SubstreamsPath>::template ProcessAllRtnType<Fn, T...>;

    template <typename SubstreamsPath, typename Fn, typename... T>
    using ProcessSubstreamsRtnConstType = typename SubstreamsDispatcher<SubstreamsPath>::template ProcessAllRtnConstType<Fn, T...>;

    template <typename LeafPath>
    using BuildBranchPath = typename memoria::list_tree::BuildTreePath<
    		BranchSubstreamsStructList,
    		memoria::list_tree::LeafCountInf<LeafSubstreamsStructList, LeafPath, 2>::Value -
    			FindLocalLeafOffsetV<
    				FlattenLeafTree<LeafSubstreamsStructList>,
    				memoria::list_tree::LeafCount<LeafSubstreamsStructList, LeafPath>::Value
    			>::Value
    >::Type;


    BranchNode() = default;


    template <typename LeafPath>
    static Int translateLeafIndexToBranchIndex(Int leaf_index)
    {
    	//return LeafToBranchIndexTranslator<LeafSubstreamsStructList, LeafPath, 0>::BranchIndex + leaf_index;

    	return LeafToBranchIndexTranslator<LeafSubstreamsStructList, LeafPath, 0>::translate(leaf_index);
    }

private:
    struct InitFn {
        UBigInt active_streams_;

        InitFn(BigInt active_streams): active_streams_(active_streams) {}

        Int block_size(Int items_number) const
        {
            return MyType::block_size(items_number, active_streams_);
        }

        Int max_elements(Int block_size)
        {
            return block_size;
        }
    };

public:

    static Int free_space(Int page_size, bool root)
    {
        Int block_size = page_size - sizeof(MyType) + PackedAllocator::my_size();
        Int client_area = PackedAllocator::client_area(block_size, SubstreamsStart + Substreams + 1);

        return client_area - root * PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
    }


    template <typename T>
    T* get_stream(Int idx)
    {
        return allocator()->template get<T>(idx + SubstreamsStart);
    }

    template <typename T>
    const T* get_stream(Int idx) const
    {
        return allocator()->template get<T>(idx + SubstreamsStart);
    }

    PackedAllocator* allocator()
    {
        return Base::allocator();
    }

    const PackedAllocator* allocator() const
    {
        return Base::allocator();
    }

    bool is_stream_empty(Int idx) const
    {
        return allocator()->is_empty(idx + SubstreamsStart);
    }

    Value* values() {
        return allocator()->template get<Value>(ValuesBlockIdx);
    }

    const Value* values() const {
        return allocator()->template get<Value>(ValuesBlockIdx);
    }


    Int capacity(UBigInt active_streams) const
    {
        Int free_space  = MyType::free_space(Base::page_size(), Base::is_root());
        Int max_size    = max_tree_size1(free_space, active_streams);
        Int cap         = max_size - size();

        return cap >= 0 ? cap : 0;
    }

    Int capacity() const
    {
        return capacity(active_streams());
    }

    bool is_empty() const
    {
        for (Int c = SubstreamsStart; c < SubstreamsEnd; c++) // StreamsEnd+1 because of values block?
        {
            if (!allocator()->is_empty(c)) {
                return false;
            }
        }

        return true;
    }

private:
    struct TreeSizeFn {
        Int size_ = 0;

        template <Int StreamIndex, Int AllocatorIdx, Int Idx, typename Node>
        void stream(Node*, Int tree_size, UBigInt active_streams)
        {
            if (active_streams && (1 << StreamIndex))
            {
                size_ += Node::block_size(tree_size);
            }
        }
    };


    struct TreeSize2Fn {
        Int size_ = 0;

        template <Int StreamIndex, Int AllocatorIdx, Int Idx, typename Node>
        void stream(Node*, const Position* sizes)
        {
            Int size = sizes->value(StreamIndex);
            if (size > 0)
            {
                size_ += Node::block_size(size);
            }
        }
    };

public:
    static Int block_size(Int tree_size, UBigInt active_streams = -1)
    {
        TreeSizeFn fn;

        processSubstreamGroupsStatic(fn, tree_size, active_streams);

        Int tree_block_size     = fn.size_;
        Int array_block_size    = PackedAllocator::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));
        Int client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }


    static Int block_size(const Position& sizes, Int values_size)
    {
        TreeSize2Fn fn;

        processSubstreamGroupsStatic(fn, &sizes);

        Int tree_block_size     = fn.size_;
        Int array_block_size    = PackedAllocator::roundUpBytesToAlignmentBlocks(values_size * sizeof(Value));
        Int client_area         = tree_block_size + array_block_size;

        return PackedAllocator::block_size(client_area, Streams + 1);
    }

private:
    static Int max_tree_size1(Int block_size, UBigInt active_streams = -1)
    {
        return FindTotalElementsNumber2(block_size, InitFn(active_streams));
    }

public:
    static Int max_tree_size_for_block(Int page_size, bool root)
    {
        Int block_size = MyType::free_space(page_size, root);
        return max_tree_size1(block_size);
    }

    void prepare()
    {
        Base::initAllocator(SubstreamsStart + Substreams + 1);
    }


    struct LayoutFn {
        template <Int AllocatorIdx, Int Idx, typename StreamType>
        void stream(StreamType*, PackedAllocator* allocator, UBigInt active_streams)
        {
            if (active_streams && (1 << Idx))
            {
                if (allocator->is_empty(AllocatorIdx))
                {
                    allocator->template allocateEmpty<StreamType>(AllocatorIdx);
                }
            }
        }
    };


    void layout(UBigInt active_streams)
    {
        Dispatcher::dispatchAllStatic(LayoutFn(), allocator(), active_streams);
    }


    UBigInt active_streams() const
    {
        UBigInt streams = 0;
        for (Int c = 0; c < Streams; c++)
        {
            UBigInt bit = !allocator()->is_empty(c + SubstreamsStart);
            streams += (bit << c);
        }

        return streams;
    }

private:

    struct InitStructFn {
        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree*, Int tree_size, PackedAllocator* allocator, UBigInt active_streams)
        {
            if (active_streams && (1 << Idx))
            {
                Int tree_block_size = Tree::block_size(tree_size);
                allocator->template allocate<Tree>(AllocatorIdx, tree_block_size);
            }
        }

        template <Int AllocatorIdx, Int Idx>
        void stream(Value*, Int tree_size, PackedAllocator* allocator)
        {
            allocator->template allocateArrayBySize<Value>(AllocatorIdx, tree_size);
        }
    };

public:

//    void init0(Int block_size, UBigInt active_streams)
//    {
//      Base::initAllocator(StreamsStart + Streams + 1);
//
//        Int tree_size = 0;//max_tree_size(block_size, active_streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), tree_size, allocator(), active_streams);
//
//        allocator()->template allocateArrayBySize<Value>(ValuesBlockIdx, tree_size);
//    }

    static Int client_area(Int block_size)
    {
        Int allocator_block_size = block_size - sizeof(MyType) + PackedAllocator::my_size();
        return PackedAllocator::client_area(allocator_block_size, Streams);
    }

    Int total_size() const
    {
        return allocator()->allocated();
    }


    void clearUnused() {}

    struct ReindexFn {
        template <typename Tree>
        void stream(Tree* tree)
        {
            tree->reindex();
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
        template <Int AllocatorIdx, Int Idx, typename Tree>
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

        Int size = this->size();

        Int requested_values_block_size = size * sizeof(Value);
        other->allocator()->resizeBlock(ValuesBlockIdx, requested_values_block_size);

        const auto* my_values   = values();
        auto* other_values      = other->values();

        for (Int c = 0; c < size; c++)
        {
            other_values[c] = my_values[c];
        }
    }

    struct ClearFn {
        template <typename Tree>
        void stream(Tree* tree, Int start, Int end)
        {
            tree->clear(start, end);
        }
    };

    void clear(Int start, Int end)
    {
        Dispatcher::dispatchNotEmpty(allocator(), ClearFn(), start, end);

        Value* values   = this->values();

        for (Int c = start; c < end; c++)
        {
            values[c] = 0;
        }
    }


    Int data_size() const
    {
        return sizeof(MyType) + this->getDataSize();
    }

    struct SizeFn {
        Int size_ = 0;

        template <typename Tree>
        void stream(const Tree* tree)
        {
            size_ = tree != nullptr ? tree->size() : 0;
        }
    };

    Int size() const
    {
        SizeFn fn;
        //FIXME: use correct procedure to get number of children
        Dispatcher::dispatchNotEmpty(allocator(), fn);
        return fn.size_;
    }

    Int size(Int stream) const
    {
        SizeFn fn;
        Dispatcher::dispatch(stream, allocator(), fn);
        return fn.size_;
    }

    struct SizesFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, Position* pos)
        {
            pos->value(StreamIdx) = tree->size();
        }
    };

    Position sizes() const
    {
        Position pos;
        processSubstreamGroups(SizesFn(), &pos);
        return pos;
    }


    bool isEmpty() const
    {
        return size() == 0;
    }

    bool isAfterEnd(const Position& idx, UBigInt active_streams) const
    {
        return idx.get() >= size();
    }

    struct SetChildrenCountFn {
        template <typename Tree>
        void stream(Tree* tree, Int size)
        {
            tree->size() = size;
        }
    };

    void set_children_count1(Int map_size)
    {
        Dispatcher::dispatchNotEmpty(allocator(), SetChildrenCountFn(), map_size);
    }



    struct InsertFn {
        template <Int Idx, typename StreamType>
        void stream(StreamType* obj, Int idx, const Accumulator& keys)
        {
            ValueSource<typename std::tuple_element<Idx, Accumulator>::type> src(std::get<Idx>(keys));
            obj->insert(&src, idx, 1);
        }
    };

    void insert(Int idx, const Accumulator& keys, const Value& value)
    {
        Int size = this->size();

        Dispatcher::dispatchNotEmpty(allocator(), InsertFn(), idx, keys);

        Int requested_block_size = (size + 1) * sizeof(Value);

        allocator()->resizeBlock(ValuesBlockIdx, requested_block_size);

        Value* values = this->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value;
    }




    struct InsertSpaceFn {
        template <typename Tree>
        void stream(Tree* tree, Int room_start, Int room_length)
        {
            tree->insertSpace(room_start, room_length);
        }
    };

    void insertSpace(const Position& from_pos, const Position& length_pos)
    {
        Int room_start  = from_pos.get();
        Int room_length = length_pos.get();

        insertSpace(0, room_start, room_length);
    }


    void insertSpace(Int stream, Int room_start, Int room_length)
    {
        Int size = this->size();

        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_start, <=, size);
        MEMORIA_ASSERT(stream, ==, 0);

        Dispatcher::dispatchNotEmpty(allocator(), InsertSpaceFn(), room_start, room_length);

        insertValuesSpace(size, room_start, room_length);
    }

    void insertValuesSpace(Int old_size, Int room_start, Int room_length)
    {
        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_start, <=, old_size);

        Int requested_block_size = (old_size + room_length) * sizeof(Value);

        allocator()->resizeBlock(ValuesBlockIdx, requested_block_size);

        Value* values = this->values();

        CopyBuffer(values + room_start, values + room_start + room_length, old_size - room_start);

        for (Int c = room_start; c < room_start + room_length; c++)
        {
            values[c] = 0;
        }
    }

    void insertValues(Int old_size, Int idx, Int length, std::function<Value()> provider)
    {
        insertValuesSpace(old_size, idx, length);

        Value* values = this->values();

        for (Int c = idx; c < idx + length; c++)
        {
            values[c] = provider();
        }
    }



    struct RemoveSpaceFn {
        template <typename Tree>
        void stream(Tree* tree, Int room_start, Int room_end)
        {
            tree->removeSpace(room_start, room_end);
        }
    };

    Accumulator removeSpace(const Position& from_pos, const Position& end_pos)
    {
        Accumulator sums;

        this->sums(from_pos.get(), end_pos.get(), sums);
        this->removeSpace(from_pos.get(), end_pos.get());

        return sums;
    }

    Accumulator removeSpaceAcc(Int room_start, Int room_end)
    {
        Accumulator sums;

        this->sums(room_start, room_end, sums);
        removeSpace(room_start, room_end);

        return -sums;
    }

    void removeSpace(Int room_start, Int room_end)
    {
        Int old_size = this->size();

        Dispatcher::dispatchNotEmpty(allocator(), RemoveSpaceFn(), room_start, room_end);

        Value* values = this->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        this->reindex();

        MEMORIA_ASSERT(old_size, >=, room_end - room_start);

        Int requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);
        allocator()->resizeBlock(values, requested_block_size);
    }

    Accumulator removeSpace(Int stream, Int room_start, Int room_end)
    {
        Accumulator accum;

        sum(stream, room_start, room_end, accum);

        removeSpace(room_start, room_end);

        return accum;
    }



    struct CopyToFn {
        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, MyType* other, Int copy_from, Int count, Int copy_to)
        {
            MEMORIA_ASSERT_TRUE(!other->is_stream_empty(AllocatorIdx));

            tree->copyTo(other->template get_stream<Tree>(AllocatorIdx), copy_from, count, copy_to);
        }
    };


    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_ASSERT(copy_from + count, <=, size());

        Dispatcher::dispatchNotEmpty(allocator(), CopyToFn(), other, copy_from, count, copy_to);

        CopyBuffer(this->values() + copy_from, other->values() + copy_to, count);
    }



    struct CanMergeWithFn {
        Int mem_used_ = 0;

        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, const MyType* other)
        {
            if (tree != nullptr)
            {
                if (other->allocator()->is_empty(AllocatorIdx))
                {
                    mem_used_ += tree->block_size();
                }
                else {
                    const Tree* other_tree = other->template get_stream<Tree>(AllocatorIdx);
                    mem_used_ += tree->block_size(other_tree);
                }
            }
            else {
                if (!other->allocator()->is_empty(AllocatorIdx))
                {
                    Int element_size = other->allocator()->element_size(AllocatorIdx);
                    mem_used_ += element_size;
                }
            }
        }
    };

    bool canBeMergedWith(const MyType* other) const
    {
        CanMergeWithFn fn;
        Dispatcher::dispatchAll(allocator(), fn, other);

        Int free_space = this->allocator()->free_space();

        Int my_data_size    = this->allocator()->element_size(ValuesBlockIdx);
        Int other_data_size = other->allocator()->element_size(ValuesBlockIdx);

        fn.mem_used_ += my_data_size;
        fn.mem_used_ += other_data_size;

        return free_space >= fn.mem_used_;
    }

    struct MergeWithFn {
        template <Int AllocatorIdx, typename Tree>
        void stream(Tree* tree, MyType* other)
        {
            Int size = tree->size();

            if (size > 0)
            {
                if (other->allocator()->is_empty(AllocatorIdx))
                {
                    other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                }

                Tree* other_tree = other->template get_stream<Tree>(AllocatorIdx);

                tree->mergeWith(other_tree);
            }
        }
    };

    void mergeWith(MyType* other)
    {
        Int other_size  = other->size();
        Int my_size     = this->size();

        Dispatcher::dispatchNotEmpty(allocator(), MergeWithFn(), other);

        Int other_values_block_size          = other->allocator()->element_size(ValuesBlockIdx);
        Int required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            other->allocator()->resizeBlock(other->values(), required_other_values_block_size);
        }

        CopyBuffer(values(), other->values() + other_size, my_size);
    }


    struct SplitToFn {
        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, MyType* other, Int idx)
        {
            Int size = tree->size();
            if (size > 0)
            {
                Tree* other_tree = other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                tree->splitTo(other_tree, idx);
            }
        }
    };


    Accumulator splitTo(MyType* other, Int split_idx)
    {
        Int size        = this->size();
        Int remainder   = size - split_idx;

        MEMORIA_ASSERT(split_idx, <=, size);

        Accumulator result;
        this->sums(split_idx, size, result);

        Dispatcher::dispatchNotEmpty(allocator(), SplitToFn(), other, split_idx);

        other->allocator()->template allocateArrayBySize<Value>(ValuesBlockIdx, remainder);

        Value* other_values = other->values();
        Value* my_values    = this->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);

        return result;
    }

    void reindexAll(Int from, Int to)
    {
        reindex();
    }


    struct KeysAtFn {
        template <Int Idx, typename Tree>
        void stream(const Tree* tree, Int idx, Accumulator* acc)
        {
            const Int Blocks = Tree::Blocks;

            for (Int c = 0; c < Blocks; c++)
            {
                std::get<Idx>(*acc)[c] = tree->value(c, idx);
            }
        }
    };

    Accumulator keysAt(Int idx) const
    {
        Accumulator acc;

        Dispatcher::dispatchNotEmpty(allocator(), KeysAtFn(), idx, &acc);

        return acc;
    }

    struct SetKeysFn {
        template <Int Idx, typename Tree>
        void stream(Tree* tree, Int idx, const Accumulator& keys)
        {
            for (Int c = 0; c < Tree::Blocks; c++)
            {
                auto k = std::get<Idx>(keys)[c];
                tree->value(c, idx) = k;
            }

            tree->reindex();
        }
    };

    void setKeys(Int idx, const Accumulator& keys)
    {
        Dispatcher::dispatchNotEmpty(allocator(), SetKeysFn(), idx, keys);
    }


    Value& value(Int idx)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, size());

        return *(values() + idx);
    }

    const Value& value(Int idx) const
    {
        if (idx < 0) {
            int a = 0; a++;
        }

        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, size());

        return *(values() + idx);
    }

    struct SumsFn {
        template <Int Idx, typename StreamType>
        void stream(const StreamType* obj, Int start, Int end, Accumulator& accum)
        {
            obj->sums(start, end, std::get<Idx>(accum));
        }

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename StreamType>
        void stream(const StreamType* obj, const Position& start, const Position& end, Accumulator& accum)
        {
            obj->sums(start[StreamIdx], end[StreamIdx], std::get<Idx>(accum));
        }

        template <Int Idx, typename StreamType>
        void stream(const StreamType* obj, Accumulator& accum)
        {
            obj->sums(std::get<Idx>(accum));
        }

        template <typename StreamType>
        void stream(const StreamType* obj, Int block, Int start, Int end, BigInt& accum)
        {
            accum += obj->sum(block, start, end);
        }
    };

    void sums(Int start, Int end, Accumulator& sums) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), start, end, sums);
    }

    void sums(const Position& start, const Position& end, Accumulator& sums) const
    {
    	processSubstreamGroups(SumsFn(), start, end, sums);
    }

    void sums(Accumulator& sums) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
    }

    Accumulator sums() const
    {
        Accumulator sums;
        Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
        return sums;
    }

    void sum(Int stream, Int block_num, Int start, Int end, BigInt& accum) const
    {
        Dispatcher::dispatch(stream, allocator(), SumsFn(), block_num, start, end, accum);
    }




    template <typename V>
    void forAllValues(Int start, Int end, std::function<void (const V&, Int)> fn) const
    {
        const Value* v = this->values();
        for (Int c = start; c < end; c++)
        {
            fn(v[c], c);
        }
    }


//    template <typename Fn, typename... Args>
//    DynDispatchRtnConstType<Fn, Args...>
//    process(Int stream, Fn&& fn, Args... args) const
//    {
//        return Dispatcher::dispatch(stream, allocator(), std::forward<Fn>(fn), args...);
//    }
//
//    template <typename Fn, typename... Args>
//    DynDispatchRtnType<Fn, Args...>
//    process(Int stream, Fn&& fn, Args&&... args)
//    {
//        return Dispatcher::dispatch(stream, allocator(), std::forward<Fn>(fn), args...);
//    }
//
//    template <typename Fn, typename... Args>
//    ProcessAllRtnConstType<Fn, Args...>
//    processAll(Fn&& fn, Args&&... args) const
//    {
//        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), args...);
//    }
//
//    template <typename Fn, typename... Args>
//    ProcessAllRtnType<Fn, Args...>
//    processAll(Fn&& fn, Args&&... args)
//    {
//        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), args...);
//    }
//
    template <typename Fn, typename... Args>
    void processNotEmpty(Fn&& fn, Args&&... args) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }

    template <typename Fn, typename... Args>
    void processNotEmpty(Fn&& fn, Args&&... args)
    {
        Dispatcher::dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }
//
//    template <typename Fn, typename... Args>
//    void processNotEmpty(UBigInt streams, Fn&& fn, Args&&... args) const
//    {
//        Dispatcher::dispatchNotEmpty(streams, allocator(), std::forward<Fn>(fn), args...);
//    }
//
//    template <typename Fn, typename... Args>
//    void processNotEmpty(UBigInt streams, Fn&& fn, Args&&... args)
//    {
//        Dispatcher::dispatchNotEmpty(streams, allocator(), std::forward<Fn>(fn), args...);
//    }
//
//
//    template <typename SubstreamPath, typename Fn, typename... Args>
//    DispatchRtnConstType<memoria::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>::Value, Fn, Args...>
//    processStream(Fn&& fn, Args&&... args) const
//    {
//        const Int StreamIdx = memoria::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>::Value;
//        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), args...);
//    }
//
//
//    template <typename SubstreamPath, typename Fn, typename... Args>
//    DispatchRtnType<memoria::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>::Value, Fn, Args...>
//    processStream(Fn&& fn, Args&&... args)
//    {
//        const Int StreamIdx = memoria::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>::Value;
//        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), args...);
//    }

    template <typename Fn, typename... Args>
    DynDispatchRtnConstType<Fn, Args...>
    process(Int stream, Fn&& fn, Args&&... args) const
    {
    	return Dispatcher::dispatch(
    			stream,
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }

    template <typename Fn, typename... Args>
    DynDispatchRtnType<Fn, Args...>
    process(Int stream, Fn&& fn, Args&&... args)
    {
    	return Dispatcher::dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    ProcessAllRtnConstType<Fn, Args...>
    processAll(Fn&& fn, Args&&... args) const
    {
    	return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    ProcessAllRtnType<Fn, Args...>
    processAll(Fn&& fn, Args&&... args)
    {
    	return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    ProcessSubstreamsRtnConstType<SubstreamsPath, Fn, Args...>
    processSubstreams(Fn&& fn, Args&&... args) const
    {
    	return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    ProcessSubstreamsRtnType<SubstreamsPath, Fn, Args...>
    processSubstreams(Fn&& fn, Args&&... args)
    {
    	return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename SubstreamPath, typename Fn, typename... Args>
    DispatchRtnType<memoria::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>::Value, Fn, Args...>
    processStream(Fn&& fn, Args&&... args) const
    {
    	const Int StreamIdx = memoria::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>::Value;
    	return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    DispatchRtnType<memoria::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>::Value, Fn, Args...>
    processStream(Fn&& fn, Args&&... args)
    {
    	const Int StreamIdx = memoria::list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>::Value;
    	return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    //DispatchRtnType<StreamIdx, Fn, Args...>
    void processStreamByIdx(Fn&& fn, Args&&... args) const
    {
    	Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    //DispatchRtnType<StreamIdx, Fn, Args...>
    void processStreamByIdx(Fn&& fn, Args&&... args)
    {
    	Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Fn, typename... Args>
    void processSubstreamGroups(Fn&& fn, Args&&... args)
    {
    	using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }

    template <typename Fn, typename... Args>
    void processSubstreamGroups(Fn&& fn, Args&&... args) const
    {
    	using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }

    template <typename Fn, typename... Args>
    static void processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
    	using GroupsList = BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    -> typename Dispatcher::template SubsetDispatcher<
    		StreamsStartSubset<BranchSubstreamsStructList>
       >::template ProcessAllRtnConstType<Fn, Args...>
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
    -> typename Dispatcher::template SubsetDispatcher<
    		StreamsStartSubset<BranchSubstreamsStructList>
       >::template ProcessAllRtnConstType<Fn, Args...>
    {
    	using Subset = StreamsStartSubset<BranchSubstreamsStructList>;
    	return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }


    struct UpdateUpFn {
        template <Int Idx, typename StreamType>
        void stream(StreamType* tree, Int idx, const Accumulator& accum)
        {
            tree->addValues(idx, std::get<Idx>(accum));
        }

        template <typename StreamType, typename DataType>
        void stream(StreamType* tree, Int idx, const SingleIndexUpdateData<DataType>& data)
        {
            tree->addValue(data.index(), idx, data.delta());
        }
    };


    void updateUp(Int idx, const Accumulator& keys)
    {
        Dispatcher::dispatchNotEmpty(allocator(), UpdateUpFn(), idx, keys);
    }

    template <typename DataType>
    void updateUp(Int idx, const SingleIndexUpdateData<DataType>& data)
    {
        Dispatcher::dispatch(data.stream(), allocator(), UpdateUpFn(), idx, data);
    }

    //FIXME: remove?
    Accumulator keys(Int pos) const
    {
        Accumulator value;
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
            tree->dump(cout);
        }
    };

    void dump() const
    {
        Dispatcher::dispatchNotEmpty(allocator(), DumpFn());
        dumpValues();
    }

    void dumpValues() const
    {
        Int size = this->size();
        auto values = this->values();

        std::cout<<"Values:"<<std::endl;
        for (Int c = 0; c < size; c++)
        {
            std::cout<<c<<" "<<values[c]<<std::endl;
        }
    }

    struct GenerateDataEventsFn {
        template <typename Tree>
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

        for (Int idx = 0; idx < size(); idx++)
        {
            vapi::ValueHelper<Value>::setup(handler, value(idx));
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

        Int size = this->size();

        FieldFactory<Value>::serialize(buf, *values(), size);
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

        Int size = this->size();

        FieldFactory<Value>::deserialize(buf, *values(), size);
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

    typedef NodePageAdaptor<TreeNode, Types>                                    MyType;
    typedef TreeNode<Types>                                                     Base;


    static const UInt PAGE_HASH = TypeHash<Base>::Value;

    static_assert(std::is_trivial<TreeNode<Types>>::value, "TreeNode must be a trivial type");


    template <
        template <typename> class,
        typename
    >
    friend class NodePageAdaptor;

private:
    static PageMetadata *page_metadata_;

public:
    NodePageAdaptor() = default;

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata() {
        return page_metadata_;
    }

    class PageOperations: public IPageOperations
    {
        virtual Int serialize(const void* page, void* buf) const
        {
            const MyType* me = T2T<const MyType*>(page);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->template serialize<FieldFactory>(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, Int buf_size, void* page) const
        {
            MyType* me = T2T<MyType*>(page);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->template deserialize<FieldFactory>(data);
        }

        virtual Int getPageSize(const void *page) const
        {
            const MyType* me = T2T<const MyType*>(page);
            return me->page_size();
        }

        virtual void resize(const void* page, void* buffer, Int new_size) const
        {
            const MyType* me = T2T<const MyType*>(page);
            MyType* tgt = T2T<MyType*>(buffer);

            tgt->copyFrom(me);
            me->transferDataTo(tgt);

            tgt->clearUnused();
            tgt->reindex();
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            handler->startPage("BTREE_NODE");
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

    static Int initMetadata()
    {
        Base::InitType();

        if (page_metadata_ == NULL)
        {
            Int attrs = 0;
            page_metadata_ = new PageMetadata("BTREE_PAGE", attrs, hash(), new PageOperations());
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
    template <typename> class TreeNode,
    typename Types
>
PageMetadata* NodePageAdaptor<TreeNode, Types>::page_metadata_ = NULL;

}

template <typename Metadata, typename Base>
struct TypeHash<bt::TreeNodeBase<Metadata, Base>> {
    typedef bt::TreeNodeBase<Metadata, Base> TargetType;

    static const UInt Value = HashHelper<
            TypeHash<Base>::Value,
            TargetType::VERSION,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<Int>::Value,
            TypeHash<typename TargetType::ID>::Value,
            TypeHash<Int>::Value,
            TypeHash<Metadata>::Value
    >::Value;
};


template <typename Types>
struct TypeHash<bt::BranchNode<Types> > {

    typedef bt::BranchNode<Types> Node;

    static const UInt Value = HashHelper<
            TypeHash<typename Node::Base>::Value,
            Node::VERSION,
            false,
            TypeHash<typename Types::Name>::Value
    >::Value;
};


}

#endif
