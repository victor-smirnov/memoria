
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
#include <memoria/v1/core/types/algo/fold.hpp>

#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_size_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>


namespace memoria {
namespace v1 {
namespace bt {


template <
    typename Types
>
class LeafNode: public Types::template TreeNodeBaseTF<typename Types::Metadata, typename Types::NodeBase>
{
    static const int32_t BranchingFactor                                        = PackedTreeBranchingFactor;

    typedef LeafNode<Types>                                                     Me;
    typedef LeafNode<Types>                                                     MyType;

public:
    static const uint32_t VERSION                                               = 2;

    static const bool Leaf                                                      = true;

    using Base = typename Types::template TreeNodeBaseTF<typename Types::Metadata, typename Types::NodeBase>;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
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

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            v1::list_tree::LeafCountInf<LeafSubstreamsStructList, SubstreamsPath>,
            v1::list_tree::LeafCountSup<LeafSubstreamsStructList, SubstreamsPath>
    >;

    template <int32_t StreamIdx>
    using StreamDispatcher = SubstreamsDispatcher<IntList<StreamIdx>>;

    template <int32_t StreamIdx>
    using StreamStartIdx = IntValue<
            v1::list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;

    template <int32_t StreamIdx>
    using StreamSize = IntValue<
            v1::list_tree::LeafCountSup<LeafSubstreamsStructList, IntList<StreamIdx>> -
            v1::list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;


    template <int32_t Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcher = typename Dispatcher::template SubsetDispatcher<
            v1::list_tree::AddToValueList<
                v1::list_tree::LeafCount<LeafSubstreamsStructList, IntList<Stream>>,
                SubstreamIdxList
            >,
            Stream
    >;


    template <int32_t Stream>
    using BTTLStreamDataDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1
    >;


    template <int32_t Stream>
    using BTTLLastStreamDataDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;


    template <int32_t Stream>
    using BTTLStreamSizesDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;

    template <int32_t SubstreamIdx>
    using LeafPathT = typename v1::list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <int32_t SubstreamIdx>
    using BranchPathT = typename v1::list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;



    template <int32_t Stream, typename SubstreamIdxList, template <typename> class MapFn>
    using MapSubstreamsStructs  = typename SubstreamsByIdxDispatcher<Stream, SubstreamIdxList>::template ForAllStructs<MapFn>;

    template <int32_t Stream, template <typename> class MapFn>
    using MapStreamStructs      = typename StreamDispatcher<Stream>::template ForAllStructs<MapFn>;


    template <typename SubstreamPath>
    using PackedStruct = typename Dispatcher::template StreamTypeT<
            v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>
    >::Type;



    static const int32_t Streams            = ListSize<LeafSubstreamsStructList>;

    static const int32_t Substreams         = Dispatcher::Size;

    static const int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;


    //FIXME: Use SubDispatcher
    LeafNode() = default;


private:
    struct InitFn {
        int32_t block_size(int32_t items_number) const
        {
            Position sizes;
            sizes[0] = items_number;
            return MyType::block_size(sizes);
        }

        int32_t max_elements(int32_t block_size)
        {
            return block_size;
        }
    };

public:

    static int32_t free_space(int32_t page_size, bool root)
    {
        int32_t block_size = page_size - sizeof(Me) + PackedAllocator::my_size();
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



    bool is_empty() const
    {
        for (int32_t c = SubstreamsStart; c < SubstreamsEnd; c++)
        {
            if (!allocator()->is_empty(c)) {
                return false;
            }
        }

        return true;
    }


//private:
//    struct BlockSizeFn {
//        int32_t size_ = 0;
//
//        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Node>
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
//    static int32_t block_size(const Position& sizes)
//    {
//        BlockSizeFn fn;
//
//        MyType::processSubstreamGroupsStatic(fn, sizes);
//
//        int32_t client_area = fn.size_;
//
//        return PackedAllocator::block_size(client_area, Streams);
//    }
//
//
    static int32_t client_area(int32_t block_size, bool root)
    {
        int32_t free_space = MyType::free_space(block_size, root);

        //FIXME Check if Streams value below is correct.
        return PackedAllocator::client_area(free_space, Streams);
    }

    int32_t total_size() const
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
        template <int32_t AllocatorIdx, int32_t Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, uint64_t streams)
        {
            if (streams & (1<<Idx))
            {
                if (alloc->is_empty(AllocatorIdx))
                {
                    OOM_THROW_IF_FAILED(alloc->template allocateEmpty<Stream>(AllocatorIdx), MMA1_SRC);
                }
            }
        }
    };


    void layout(uint64_t streams)
    {
        Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator(), streams);
    }


    uint64_t active_streams() const
    {
        uint64_t streams = 0;
        for (int32_t c = 0; c < Streams; c++)
        {
            uint64_t bit = !allocator()->is_empty(c);
            streams += (bit << c);
        }

        return streams;
    }

private:

//    struct InitStructFn {
//        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
//        void stream(Tree*, PackedAllocator* allocator, const Position& sizes)
//        {
//            if (sizes[Idx] > -1)
//            {
//                int32_t block_size = Tree::block_size(sizes[Idx]);
//                allocator->template allocate<Tree>(AllocatorIdx, block_size);
//            }
//        }
//
//        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
//        void stream(Tree*, PackedAllocator* allocator, int32_t client_area)
//        {
//            if (Idx == 0)
//            {
//                allocator->template allocate<Tree>(AllocatorIdx, client_area);
//            }
//        }
//    };

public:

//    void init0(int32_t block_size, const Position& sizes)
//    {
//        allocator()->init(block_size, Streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), sizes);
//    }
//
//    void init0(int32_t block_size)
//    {
//        allocator()->init(block_size, Streams);
//
//        Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), allocator()->client_area());
//    }


    struct ObjectSizeFn {
        template <typename Tree>
        void stream(const Tree* tree, PackedAllocator* allocator, const int32_t* size)
        {
            *size += tree->object_size();
        }
    };

    void init() {
        Base::initAllocator(SubstreamsStart + Substreams);
    }

    int32_t object_size() const
    {
        int32_t size = 0;
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
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
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



    int32_t data_size() const
    {
        return sizeof(Me) + this->getDataSize(); //FIXME: sizeof(Me) and array[] in PackedAllocator
    }



    struct MemUsedFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, int32_t* mem_used, int32_t except)
        {
            if (StreamIdx != except)
            {
                int32_t size = sizes[StreamIdx];

                if (tree != nullptr || size > 0)
                {
                    *mem_used += Tree::packed_block_size(size);
                }
            }
        }
    };



    struct CheckCapacitiesFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, int32_t* mem_size)
        {
            int32_t size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }


        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename Entropy>
        void stream(const Tree* tree, const Entropy& entropy, const Position& sizes, int32_t* mem_size)
        {
            int32_t size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }
    };

    bool checkCapacities(const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (int32_t c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        int32_t mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), fillment, &mem_size);

        int32_t free_space      = MyType::free_space(this->page_size(), this->is_root());
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size + 300;
    }


    template <typename Entropy>
    bool checkCapacities(const Entropy& entropy, const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (int32_t c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        int32_t mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), entropy, fillment, &mem_size);

        int32_t free_space      = MyType::free_space(this->page_size(), this->is_root());
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }


    struct SingleStreamCapacityFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(const Tree* tree, int32_t size, int32_t& mem_size)
        {
            mem_size += Tree::packed_block_size(size);
        }
    };


    int32_t stream_block_size(int32_t size) const
    {
        int32_t mem_size = 0;

        StreamDispatcher<0>::dispatchAllStatic(SingleStreamCapacityFn(), size, mem_size);

        return mem_size;
    }


    int32_t single_stream_capacity(int32_t max_hops) const
    {
        int32_t min = sizes()[0];
        int32_t max = this->page_size() * 8;

        int32_t free_space      = MyType::free_space(this->page_size(), this->is_root());
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        int32_t total = FindTotalElementsNumber(min, max, client_area, max_hops, [&](int32_t stream_size){
            return stream_block_size(stream_size);
        });

        return total - min;
    }


    struct Size2Fn {
        template <int32_t StreamIdx, typename T>
        int32_t process(const T* node)
        {
            return node->template streamSize<StreamIdx>();
        }
    };


    int32_t size(int32_t stream) const
    {
        return ForEachStream<Streams - 1>::process(stream, Size2Fn(), this);
    }

    struct SizeFn {
        template <typename Tree>
        int32_t stream(const Tree* tree)
        {
            return tree != nullptr ? tree->size() : 0;
        }
    };

    template <int32_t StreamIdx>
    int32_t streamSize() const
    {
        return this->processStream<IntList<StreamIdx>>(SizeFn());
    }

    struct SizesFn {
        template <int32_t StreamIdx, typename Tree>
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

        template <int32_t Offset, bool StreamStart, int32_t ListIdx, typename StreamType, typename TupleItem>
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

    bool isEmpty(int32_t stream) const
    {
        return size(stream) == 0;
    }

    bool isEmpty() const
    {
        Position sizes = this->sizes();
        return sizes.eqAll(0);
    }


    bool isAfterEnd(const Position& idx, uint64_t active_streams) const
    {
        Position sizes = this->sizes();

        for (int32_t c = 0; c < Streams; c++)
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
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
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
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree* tree, PackedAllocator* allocator, const Position& room_start, const Position& room_length)
        {
            if (tree != nullptr)
            {
                tree->insertSpace(room_start[StreamIdx], room_length[StreamIdx]);
            }
            else {
                MEMORIA_V1_ASSERT_TRUE(room_length[StreamIdx] == 0);
            }
        }
    };

    void insertSpace(const Position& room_start, const Position& room_length)
    {
        initStreamsIfEmpty(room_length);
        this->processSubstreamGroups(InsertSpaceFn(), allocator(), room_start, room_length);
    }

    void insertSpace(int32_t stream, int32_t room_start, int32_t room_length)
    {
        insertSpace(Position::create(stream, room_start), Position::create(stream, room_length));
    }

    struct RemoveSpaceFn {
        OpStatus status_{OpStatus::OK};

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree* tree, const Position& room_start, const Position& room_end)
        {
            if (tree != nullptr && isOk(status_))
            {
                status_ <<= tree->removeSpace(room_start[StreamIdx], room_end[StreamIdx]);
            }
        }

        template <typename Tree>
        void stream(Tree* tree, int32_t room_start, int32_t room_end)
        {
            if (tree != nullptr && isOk(status_))
            {
                status_ <<= tree->removeSpace(room_start, room_end);
            }
        }
    };

    OpStatus removeSpace(int32_t stream, int32_t room_start, int32_t room_end)
    {
        RemoveSpaceFn fn;
        Dispatcher::dispatch(stream, allocator(), fn, room_start, room_end);
        return fn.status_;
    }

    OpStatus removeSpace(const Position& room_start, const Position& room_end)
    {
        RemoveSpaceFn fn;
        this->processSubstreamGroups(fn, room_start, room_end);

        // FIXME: enable once other methods (finders) are ready for
        // this optimization
        // removeEmptyStreams();

        return fn.status_;
    }

//    FIXME: this code doesn't work with substreams
//    void removeEmptyStreams()
//    {
//        Position sizes = this->sizes();
//
//        for (int32_t c = Position::Indexes - 1; c >= 0; c--)
//        {
//            if (sizes[c] == 0)
//            {
//                allocator()->free(c);
//            }
//        }
//    }

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

        return client_area >= fn.mem_used_;
    }



    struct MergeWithFn {
        OpStatus status_{OpStatus::OK};

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree* tree, MyType* other)
        {
            if (isOk(status_))
            {
                int32_t size = tree->size();

                if (size > 0)
                {
                    if (other->allocator()->is_empty(AllocatorIdx))
                    {
                        if(isFail(other->allocator()->template allocateEmpty<Tree>(AllocatorIdx))) {
                            status_ <<= OpStatus::FAIL;
                            return;
                        }
                    }

                    Tree* other_tree = other->allocator()->template get<Tree>(AllocatorIdx);
                    status_ <<= tree->mergeWith(other_tree);
                }
            }
        }
    };

    OpStatus mergeWith(MyType* other)
    {
        MergeWithFn fn;
        Dispatcher::dispatchNotEmpty(allocator(), fn, other);
        return fn.status_;
    }

    struct SplitToFn {
        OpStatus status_{OpStatus::OK};

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree* tree, MyType* other, const Position& indexes)
        {
            if (tree != nullptr && isOk(status_))
            {
                int32_t idx   = indexes[StreamIdx];
                int32_t size  = tree->size();

                MEMORIA_V1_ASSERT(idx, >=, 0);
                MEMORIA_V1_ASSERT(idx, <=, size);

                Tree* other_tree;

                if (!other->allocator()->is_empty(AllocatorIdx)) {
                	other_tree = other->allocator()->template get<Tree>(AllocatorIdx);
                }
                else {
                	other_tree = other->allocator()->template allocateEmpty<Tree>(AllocatorIdx);
                }

                if (isFail(other_tree)) {
                    status_ <<= OpStatus::FAIL;
                    return;
                }

                status_ <<= tree->splitTo(other_tree, idx);
            }
        }
    };


    OpStatus splitTo(MyType* other, const Position& from)
    {
        SplitToFn split_fn;
        this->processSubstreamGroups(split_fn, other, from);

        return split_fn.status_;
    }


    struct CopyToFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
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
        MEMORIA_V1_ASSERT_TRUE((copy_from + count).lteAll(sizes()));
        MEMORIA_V1_ASSERT_TRUE((copy_to + count).lteAll(other->max_sizes()));

        this->processSubstreamGroups(CopyToFn(), other, copy_from, count, copy_to);
    }



    struct BranchNodeEntryHandler
    {
        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, int32_t start, int32_t end)
        {
            if (obj != nullptr)
            {
                obj->template sum<Offset>(start, end, accum);
            }
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
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

        template <int32_t Offset, bool StreamStart, int32_t ListIdx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, const Position& start, const Position& end)
        {
            const int32_t StreamIdx = FindTopLevelIdx<LeafSubstreamsStructList, ListIdx>::Value;

            int32_t startIdx    = start[StreamIdx];
            int32_t endIdx      = end[StreamIdx];

            stream<Offset, StreamStart, ListIdx>(obj, accum, startIdx, endIdx);
        }
    };


    struct BranchNodeEntryMaxHandler
    {

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum)
        {
            if (obj != nullptr)
            {
                obj->template max<Offset>(accum);
            }
        }
    };



    void sums(int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end);
    }


    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end);
    }


    struct LeafSumsFn {
        template <typename StreamType>
        auto stream(const StreamType* obj, int32_t start, int32_t end)
        {
            return obj ? obj->sum(start, end) : decltype(obj->sum(start, end))();
        }

        template <typename StreamType>
        auto stream(const StreamType* obj, int32_t block, int32_t start, int32_t end)
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
        template <int32_t ListIdx, typename Tree>
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
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename... TupleTypes>
        void stream(const Tree* tree, std::tuple<TupleTypes...>& entropy, const Position& start, const Position& end)
        {
            if (tree != nullptr) tree->estimateEntropy(std::get<StreamIdx>(entropy), start[StreamIdx], end[StreamIdx]);
        }

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename... TupleTypes>
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



    struct ProcessSubstreamsAccFnAdaptor
    {
        template <
            int32_t BranchNodeEntryIdx,
            int32_t ListIdx,
            typename StreamType,
            typename Accum,
            typename Fn,
            typename... Args
        >
        void stream(StreamType* obj, Fn&& fn, Accum&& accum, Args&&... args)
        {
            const int32_t LeafIdx = BranchNodeEntryIdx - SubstreamsStart;

            const int32_t BranchStructIdx   = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::BranchStructIdx;
            const int32_t LeafOffset        = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::LeafOffset;
            const bool IsStreamStart    = LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::IsStreamStart;

            fn.template stream<LeafOffset, IsStreamStart, ListIdx>(
                    obj,
                    std::get<BranchStructIdx>(accum),
                    std::forward<Args>(args)...
            );
        }
    };



    template <
        int32_t Stream,
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
        int32_t Stream,
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
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
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
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>::dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        int32_t Stream,
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
        int32_t Stream,
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
        int32_t Stream,
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
        int32_t Stream,
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
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;

        if (!this->allocator()->is_empty(SubstreamIdx + SubstreamsStart))
        {
        	return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
        }
        else {
        	return T2T<T*>(nullptr);
        }
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const int32_t SubstreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;

        if (!this->allocator()->is_empty(SubstreamIdx + SubstreamsStart))
        {
        	return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
        }
        else {
        	return T2T<const T*>(nullptr);
        }
    }

    template <int32_t SubstreamIdx>
    auto substream_by_idx()
    {
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }

    template <int32_t SubstreamIdx>
    auto substream_by_idx() const
    {
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }





    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
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

    void set_children_count(int32_t)
    {
        MMA1_THROW(Exception()) << WhatCInfo("Deprecated method set_children_count()");
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


    struct DumpBlockSizesFn {
    	template <typename Tree>
    	void stream(Tree* tree)
    	{
    		std::cout << tree->block_size() << std::endl;
    	}
    };

    void dumpBlockSizes() const {
    	Dispatcher::dispatchNotEmpty(allocator(), DumpBlockSizesFn());
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


}}
