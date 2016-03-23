
// Copyright Victor Smirnov 2013+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/exceptions/memoria.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/tools/assert.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/types/list/misc.hpp>
#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>


namespace memoria {
namespace bt        {


template <
    typename Types
>
class LeafNode: public TreeNodeBase<typename Types::Metadata, typename Types::NodeBase>
{
    static const Int BranchingFactor                                            = PackedTreeBranchingFactor;

    typedef LeafNode<Types>                                                     Me;
    typedef LeafNode<Types>                                                     MyType;

public:
    static const UInt VERSION                                                   = 2;

    static const bool Leaf                                                      = true;

    typedef bt::TreeNodeBase<
                typename Types::Metadata,
                typename Types::NodeBase
    >                                                                           Base;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList    = typename Types::BranchStreamsStructList;
    using LeafSubstreamsStructList      = typename Types::LeafStreamsStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            Linearize<LeafSubstreamsStructList>,
            Base::StreamsStart
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList>;

    template <Int StartIdx, Int EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            memoria::list_tree::LeafCountInf<LeafSubstreamsStructList, SubstreamsPath>::Value,
            memoria::list_tree::LeafCountSup<LeafSubstreamsStructList, SubstreamsPath>::Value
    >;

    template <Int StreamIdx>
    using StreamDispatcher = SubstreamsDispatcher<IntList<StreamIdx>>;

    template <Int StreamIdx>
    using StreamStartIdx = IntValue<
            memoria::list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>::Value
    >;

    template <Int StreamIdx>
    using StreamSize = IntValue<
            memoria::list_tree::LeafCountSup<LeafSubstreamsStructList, IntList<StreamIdx>>::Value -
            memoria::list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>::Value
    >;


    template <Int Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcher = typename Dispatcher::template SubsetDispatcher<
            memoria::list_tree::AddToValueList<
                memoria::list_tree::LeafCount<LeafSubstreamsStructList, IntList<Stream>>::Value,
                SubstreamIdxList
            >,
            Stream
    >;

    template <Int SubstreamIdx>
    using LeafPathT = typename memoria::list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <Int SubstreamIdx>
    using BranchPathT = typename memoria::list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;



    static const Int Streams                                                    = ListSize<LeafSubstreamsStructList>::Value;

    static const Int Substreams                                                 = Dispatcher::Size;

    static const Int SubstreamsStart                                            = Dispatcher::AllocatorIdxStart;
    static const Int SubstreamsEnd                                              = Dispatcher::AllocatorIdxEnd;


    //FIXME: Use SubDispatcher
    LeafNode() = default;


private:
    struct InitFn {
        Int block_size(Int items_number) const
        {
            Position sizes;
            sizes[0] = items_number;
            return MyType::block_size(sizes);
        }

        Int max_elements(Int block_size)
        {
            return block_size;
        }
    };

public:

    static Int free_space(Int page_size, bool root)
    {
        Int block_size = page_size - sizeof(Me) + PackedAllocator::my_size();
        Int client_area = PackedAllocator::client_area(block_size, SubstreamsStart + Substreams + 1);

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



    bool is_empty() const
    {
        for (Int c = SubstreamsStart; c < SubstreamsEnd; c++)
        {
            if (!allocator()->is_empty(c)) {
                return false;
            }
        }

        return true;
    }


//private:
//    struct BlockSizeFn {
//        Int size_ = 0;
//
//        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Node>
//        void stream(Node*, const Position& sizes)
//        {
//            if (sizes[StreamIdx] > 0)
//            {
//                size_ += Node::block_size(sizes[StreamIdx]);
//            }
//        }
//    };
//
//public:
//    static Int block_size(const Position& sizes)
//    {
//        BlockSizeFn fn;
//
//        MyType::processSubstreamGroupsStatic(fn, sizes);
//
//        Int client_area = fn.size_;
//
//        return PackedAllocator::block_size(client_area, Streams);
//    }
//
//
    static Int client_area(Int block_size, bool root)
    {
        Int free_space = MyType::free_space(block_size, root);

        //FIXME Check if Streams value below is correct.
        return PackedAllocator::client_area(free_space, Streams);
    }

    Int total_size() const
    {
        return allocator()->allocated();
    }

    void prepare()
    {
        Base::initAllocator(SubstreamsStart + Substreams); // FIXME +1?
    }

    void layout(const Position& sizes)
    {
        layout(-1ull);
    }


    struct LayoutFn {
        template <Int AllocatorIdx, Int Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, UBigInt streams)
        {
            if (streams & (1<<Idx))
            {
                if (alloc->is_empty(AllocatorIdx))
                {
                    alloc->template allocateEmpty<Stream>(AllocatorIdx);
                }
            }
        }
    };


    void layout(UBigInt streams)
    {
        Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator(), streams);
    }


    UBigInt active_streams() const
    {
        UBigInt streams = 0;
        for (Int c = 0; c < Streams; c++)
        {
            UBigInt bit = !allocator()->is_empty(c);
            streams += (bit << c);
        }

        return streams;
    }

private:

//    struct InitStructFn {
//        template <Int AllocatorIdx, Int Idx, typename Tree>
//        void stream(Tree*, PackedAllocator* allocator, const Position& sizes)
//        {
//            if (sizes[Idx] > -1)
//            {
//                Int block_size = Tree::block_size(sizes[Idx]);
//                allocator->template allocate<Tree>(AllocatorIdx, block_size);
//            }
//        }
//
//        template <Int AllocatorIdx, Int Idx, typename Tree>
//        void stream(Tree*, PackedAllocator* allocator, Int client_area)
//        {
//            if (Idx == 0)
//            {
//                allocator->template allocate<Tree>(AllocatorIdx, client_area);
//            }
//        }
//    };

public:

//    void init0(Int block_size, const Position& sizes)
//    {
//        allocator()->init(block_size, Streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), sizes);
//    }
//
//    void init0(Int block_size)
//    {
//        allocator()->init(block_size, Streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), allocator()->client_area());
//    }


    struct ObjectSizeFn {
        template <typename Tree>
        void stream(const Tree* tree, PackedAllocator* allocator, const Int* size)
        {
            *size += tree->object_size();
        }
    };

    void init() {
        Base::initAllocator(SubstreamsStart + Substreams);
    }

    Int object_size() const
    {
        Int size = 0;
        Dispatcher::dispatchNotEmpty(ObjectSizeFn(), allocator(), &size);

        return size;
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
            auto allocator       = tree->allocator();
            auto other_allocator = other->allocator();

            other_allocator->importBlock(AllocatorIdx, allocator, AllocatorIdx);
        }
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        Base::transferDataTo(other);

        Dispatcher::dispatchNotEmpty(allocator(), TransferToFn<TreeType>(), other);
    }



    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize(); //FIXME: sizeof(Me) and array[] in PackedAllocator
    }



    struct MemUsedFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, Int* mem_used, Int except)
        {
            if (StreamIdx != except)
            {
                Int size = sizes[StreamIdx];

                if (tree != nullptr || size > 0)
                {
                    *mem_used += Tree::packed_block_size(size);
                }
            }
        }
    };



    struct CheckCapacitiesFn {

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, Int* mem_size)
        {
            Int size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }


        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree, typename Entropy>
        void stream(const Tree* tree, const Entropy& entropy, const Position& sizes, Int* mem_size)
        {
            Int size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }
    };

    bool checkCapacities(const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (Int c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        Int mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), fillment, &mem_size);

        Int free_space      = MyType::free_space(this->page_size(), this->is_root());
        Int client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }


    template <typename Entropy>
    bool checkCapacities(const Entropy& entropy, const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (Int c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        Int mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), entropy, fillment, &mem_size);

        Int free_space      = MyType::free_space(this->page_size(), this->is_root());
        Int client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }


    struct SingleStreamCapacityFn {

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, Int size, Int& mem_size)
        {
            mem_size += Tree::packed_block_size(size);
        }
    };


    Int stream_block_size(Int size) const
    {
        Int mem_size = 0;

        StreamDispatcher<0>::dispatchAllStatic(SingleStreamCapacityFn(), size, mem_size);

        return mem_size;
    }


    Int single_stream_capacity(Int max_hops) const
    {
        Int min = sizes()[0];
        Int max = this->page_size() * 8;

        Int free_space      = MyType::free_space(this->page_size(), this->is_root());
        Int client_area     = PackedAllocator::client_area(free_space, Streams);

        Int total = FindTotalElementsNumber(min, max, client_area, max_hops, [&](Int stream_size){
            return stream_block_size(stream_size);
        });

        return total - min;
    }


    struct Size2Fn {
        template <Int StreamIdx, typename T>
        Int process(const T* node)
        {
            return node->template streamSize<StreamIdx>();
        }
    };


    Int size(Int stream) const
    {
        return ForEachStream<Streams - 1>::process(stream, Size2Fn(), this);
    }

    struct SizeFn {
        template <typename Tree>
        Int stream(const Tree* tree)
        {
            return tree != nullptr ? tree->size() : 0;
        }
    };

    template <Int StreamIdx>
    Int streamSize() const
    {
        return this->processStream<IntList<StreamIdx>>(SizeFn());
    }

    struct SizesFn {
        template <Int StreamIdx, typename Tree>
        void stream(const Tree* tree, Position& pos)
        {
            pos[StreamIdx] = tree != nullptr ? tree->size() : 0;
        }
    };

    Position sizes() const
    {
        Position pos;
        this->processStreamsStart(SizesFn(), pos);
        return pos;
    }

    struct AccumSizesFn {

        template <typename... Args>
        void stream(Args&&... args) {

        }

        template <Int Offset, bool StreamStart, Int ListIdx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, Position& sizes)
        {
            static_assert(StreamStart, "StreamStart must be true for structures at the start of a stream");
            sizes[ListIdx] = accum[0];
        }
    };

    static Position sizes(const BranchNodeEntry& sums)
    {
        Position sz;
        processStreamsStartStaticAcc(AccumSizesFn(), sums, sz);
        return sz;
    }

    bool isEmpty(Int stream) const
    {
        return size(stream) == 0;
    }

    bool isEmpty() const
    {
        Position sizes = this->sizes();
        return sizes.eqAll(0);
    }


    bool isAfterEnd(const Position& idx, UBigInt active_streams) const
    {
        Position sizes = this->sizes();

        for (Int c = 0; c < Streams; c++)
        {
            if (active_streams & (1<<c) && idx[c] < sizes[c])
            {
                return false;
            }
        }

        return true;

        return idx.gteAll(sizes);
    }


    struct InitStreamIfEmpty
    {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, PackedAllocator* allocator, const Position& sizes)
        {
            if (tree == nullptr && sizes[StreamIdx] > 0)
            {
                allocator->template allocate<Tree>(AllocatorIdx, Tree::empty_size());
            }
        }
    };


    void initStreamsIfEmpty(const Position& sizes)
    {
        this->processSubstreamGroups(InitStreamIfEmpty(), allocator(), sizes);
    }


    struct InsertSpaceFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, PackedAllocator* allocator, const Position& room_start, const Position& room_length)
        {
            if (tree != nullptr)
            {
                tree->insertSpace(room_start[StreamIdx], room_length[StreamIdx]);
            }
            else {
                MEMORIA_ASSERT_TRUE(room_length[StreamIdx] == 0);
            }
        }
    };

    void insertSpace(const Position& room_start, const Position& room_length)
    {
        initStreamsIfEmpty(room_length);
        this->processSubstreamGroups(InsertSpaceFn(), allocator(), room_start, room_length);
    }

    void insertSpace(Int stream, Int room_start, Int room_length)
    {
        insertSpace(Position::create(stream, room_start), Position::create(stream, room_length));
    }

    struct RemoveSpaceFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, const Position& room_start, const Position& room_end)
        {
            if (tree != nullptr) {
                tree->removeSpace(room_start[StreamIdx], room_end[StreamIdx]);
            }
        }

        template <typename Tree>
        void stream(Tree* tree, Int room_start, Int room_end)
        {
            if (tree != nullptr)
            {
                tree->removeSpace(room_start, room_end);
            }
        }
    };

    void removeSpace(Int stream, Int room_start, Int room_end)
    {
        Dispatcher::dispatch(stream, allocator(), RemoveSpaceFn(), room_start, room_end);
    }

    void removeSpace(const Position& room_start, const Position& room_end)
    {
        this->processSubstreamGroups(RemoveSpaceFn(), room_start, room_end);

        // FIXME: enable once other methods (finders) are ready for
        // this optimization
        // removeEmptyStreams();
    }

//    FIXME: this code doesn't work with substreams
//    void removeEmptyStreams()
//    {
//        Position sizes = this->sizes();
//
//        for (Int c = Position::Indexes - 1; c >= 0; c--)
//        {
//            if (sizes[c] == 0)
//            {
//                allocator()->free(c);
//            }
//        }
//    }

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
                    const Tree* other_tree = other->allocator()->template get<Tree>(AllocatorIdx);
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

        Int client_area = this->allocator()->client_area();

        return client_area >= fn.mem_used_;
    }



    struct MergeWithFn {
        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, MyType* other)
        {
            Int size = tree->size();

            if (size > 0)
            {
                if (other->allocator()->is_empty(AllocatorIdx))
                {
                    other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                }

                Tree* other_tree = other->allocator()->template get<Tree>(AllocatorIdx);
                tree->mergeWith(other_tree);
            }
        }
    };

    void mergeWith(MyType* other)
    {
        Dispatcher::dispatchNotEmpty(allocator(), MergeWithFn(), other);
    }

    struct SplitToFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(Tree* tree, MyType* other, const Position& indexes)
        {
            if (tree != nullptr)
            {
                Int idx   = indexes[StreamIdx];
                Int size  = tree->size();

                if (idx < 0 || idx > size) {
                    int a = 0; a++;
                }

                MEMORIA_ASSERT(idx, >=, 0);
                MEMORIA_ASSERT(idx, <=, size);

                Tree* other_tree = other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                tree->splitTo(other_tree, idx);
            }
        }
    };


    BranchNodeEntry splitTo(MyType* other, const Position& from)
    {
        BranchNodeEntry result;

//        Position sizes = this->sizes();

//        sums(from, sizes, result);

        this->processSubstreamGroups(SplitToFn(), other, from);

        return result;
    }


    struct CopyToFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(
                const Tree* tree,
                MyType* other,
                const Position& copy_from,
                const Position& count,
                const Position& copy_to
        )
        {
            tree->copyTo(
                    other->allocator()->template get<Tree>(AllocatorIdx),
                    copy_from[StreamIdx],
                    count[StreamIdx],
                    copy_to[StreamIdx]
            );
        }
    };


    void copyTo(MyType* other, const Position& copy_from, const Position& count, const Position& copy_to) const
    {
        MEMORIA_ASSERT_TRUE((copy_from + count).lteAll(sizes()));
        MEMORIA_ASSERT_TRUE((copy_to + count).lteAll(other->max_sizes()));

        this->processSubstreamGroups(CopyToFn(), other, copy_from, count, copy_to);
    }



    struct BranchNodeEntryHandler
    {
        template <Int Offset, bool StreamStart, Int Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, Int start, Int end)
        {
            if (obj != nullptr)
            {
//                if (StreamStart)
//                {
//                  accum[Offset - 1] += end - start;
//                }

                obj->template sum<Offset>(start, end, accum);
            }
        }

        template <Int Offset, bool StreamStart, Int Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum)
        {
            if (obj != nullptr)
            {
                if (StreamStart)
                {
                    accum[Offset - 1] += obj->size();
                }

                obj->template sum<Offset>(accum);
            }
        }

        template <Int Offset, bool StreamStart, Int ListIdx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, const Position& start, const Position& end)
        {
            const Int StreamIdx = FindTopLevelIdx<LeafSubstreamsStructList, ListIdx>::Value;

            Int startIdx    = start[StreamIdx];
            Int endIdx      = end[StreamIdx];

            stream<Offset, StreamStart, ListIdx>(obj, accum, startIdx, endIdx);
        }
    };


    struct BranchNodeEntryMaxHandler
    {

        template <Int Offset, bool StreamStart, Int Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum)
        {
            if (obj != nullptr)
            {
                obj->template max<Offset>(accum);
            }
        }
    };



    void sums(Int start, Int end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end);
    }


    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end);
    }


    struct LeafSumsFn {
        template <typename StreamType>
        auto stream(const StreamType* obj, Int start, Int end)
        {
            return obj ? obj->sum(start, end) : decltype(obj->sum(start, end))();
        }

        template <typename StreamType>
        auto stream(const StreamType* obj, Int block, Int start, Int end)
        {
            return obj ? obj->sum(block, start, end) : 0;
        }
    };


    template <typename Path, typename... Args>
    auto leaf_sums(Args&&... args) const
    {
        return processStream<Path>(LeafSumsFn(), std::forward<Args>(args)...);
    }


    void max(BranchNodeEntry& entry) const
    {
        processAllSubstreamsAcc(BranchNodeEntryMaxHandler(), entry);
    }


    struct SizeSumsFn {
        template <Int ListIdx, typename Tree>
        void stream(Tree* tree, Position& sizes)
        {
            sizes[ListIdx] = tree != nullptr ? tree->size() : 0;
        }
    };

    Position size_sums() const
    {
        Position sums;
        processStreamsStart(SizeSumsFn(), sums);
        return sums;
    }



    struct EstimateEntropyFn
    {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree, typename... TupleTypes>
        void stream(const Tree* tree, std::tuple<TupleTypes...>& entropy, const Position& start, const Position& end)
        {
            if (tree != nullptr) tree->estimateEntropy(std::get<StreamIdx>(entropy), start[StreamIdx], end[StreamIdx]);
        }

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree, typename... TupleTypes>
        void stream(const Tree* tree, std::tuple<TupleTypes...>& entropy)
        {
            if (tree != nullptr) tree->estimateEntropy(std::get<StreamIdx>(entropy));
        }
    };

    template <typename... TupleTypes>
    void estimateEntropy(std::tuple<TupleTypes...>& entropy, const Position& start, const Position& end)
    {
        processSubstreamGroups(EstimateEntropyFn(), entropy, start, end);
    }

    template <typename... TupleTypes>
    void estimateEntropy(std::tuple<TupleTypes...>& entropy)
    {
        processSubstreamGroups(EstimateEntropyFn(), entropy);
    }




    struct ComputeDataLengthsFn
    {
        template <typename Tree, typename EntryType, typename Lengths>
        void stream(Tree*, const EntryType& entry, Lengths& lengths)
        {
            Tree::computeDataLength(entry, lengths);
        }
    };

    template <typename EntryType, typename... TupleTypes>
    static void computeDataLengths(const EntryType& entry, std::tuple<TupleTypes...>& lengths)
    {
        Dispatcher::dispatchAllStatic(ComputeDataLengthsFn(), entry, lengths);
    }






    template <typename Fn, typename... Args>
    auto process(Int stream, Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto process(Int stream, Fn&& fn, Args&&... args)
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



    struct ProcessSubstreamsAccFnAdaptor
    {
        template <
            Int BranchNodeEntryIdx,
            Int ListIdx,
            typename StreamType,
            typename Accum,
            typename Fn,
            typename... Args
        >
        void stream(StreamType* obj, Fn&& fn, Accum&& accum, Args&&... args)
        {
            const Int LeafIdx = BranchNodeEntryIdx - SubstreamsStart;

            const Int BranchStructIdx   = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::BranchStructIdx;
            const Int LeafOffset        = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::LeafOffset;
            const bool IsStreamStart    = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::IsStreamStart;

            fn.template stream<LeafOffset, IsStreamStart, ListIdx>(
                    obj,
                    std::get<BranchStructIdx>(accum),
                    std::forward<Args>(args)...
            );
        }
    };



    template <
        Int Stream,
        typename Fn,
        typename... Args
    >
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
    {
        return StreamDispatcher<Stream>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }

    template <
        Int Stream,
        typename Fn,
        typename... Args
    >
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
    {
        return StreamDispatcher<Stream>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        typename SubstreamPath,
        typename Fn,
        typename... Args
    >
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
    {
        const Int SubstreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        typename SubstreamPath,
        typename Fn,
        typename... Args
    >
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
    {
        const Int SubstreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        Int Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        Int Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        typename Fn,
        typename... Args
    >
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
    {
        return Dispatcher::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }

    template <
        typename Fn,
        typename... Args
    >
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
    {
        return Dispatcher::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }




    template <
        Int Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) const
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <
        Int Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args)
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }



    template <typename SubstreamPath>
    auto substream()
    {
        const Int SubstreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const Int SubstreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }




    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const Int StreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const Int StreamIdx = memoria::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>::Value;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const
    {
        using GroupsList = BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    {
        using Subset = StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const
    {
        using Subset = StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    static auto processStreamsStartStatic(Fn&& fn, Args&&... args)
    {
        using Subset = StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename Accum, typename... Args>
    static auto processStreamsStartStaticAcc(Fn&& fn, Accum&& accum, Args&&... args)
    {
        using Subset = StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                std::forward<Accum>(accum),
                std::forward<Args>(args)...
        );
    }



    struct GenerateDataEventsFn {
        template <Int Idx, typename Tree>
        void stream(const Tree* tree, IPageDataEventHandler* handler)
        {
            tree->generateDataEvents(handler);
        }
    };

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Dispatcher::dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);
    }

    struct SerializeFn {
        template <typename Tree>
        void stream(const Tree* tree, SerializationData* buf)
        {
            tree->serialize(*buf);
        }
    };

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);
    }

    struct DeserializeFn {
        template <typename Tree>
        void stream(Tree* tree, DeserializationData* buf)
        {
            tree->deserialize(*buf);
        }
    };

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);
    }

    static void InitType() {}

    void set_children_count(Int)
    {
        throw Exception(MA_SRC, "Deprecated method set_children_count()");
    }

    struct DumpFn {
        template <typename Tree>
        void stream(Tree* tree)
        {
            tree->dump();
        }
    };


    void dump() const {
        Dispatcher::dispatchNotEmpty(allocator(), DumpFn());
    }
};




}



template <typename Types>
struct TypeHash<bt::LeafNode<Types> > {

    typedef bt::LeafNode<Types> Node;

    static const UInt Value = HashHelper<
            TypeHash<typename Node::Base>::Value,
            Node::VERSION,
            true,
            TypeHash<typename Types::Name>::Value
    >::Value;
};


}
