
// Copyright 2019 Victor Smirnov
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

#include <memoria/prototypes/bt/nodes/node_common_so.hpp>

#include <memoria/core/iovector/io_vector.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_iovector.hpp>

#include <memoria/core/packed/tools/packed_stateful_dispatcher.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {


namespace _ {

template <int32_t Streams>
class ConfigureIOVectorViewFn {

    int32_t stream_idx_{};
    int32_t data_substreams_;
    io::IOVector& io_vector_;
public:
    ConfigureIOVectorViewFn(io::IOVector& io_vector):
        data_substreams_(io_vector.substreams()),
        io_vector_(io_vector)
    {}

    template <typename Stream>
    void stream(const Stream& stream)
    {
        if (stream_idx_ < data_substreams_)
        {
            stream.configure_io_substream(io_vector_.substream(stream_idx_));
            stream_idx_++;
        }
        else
        {
            stream.configure_io_substream(io_vector_.symbol_sequence());
        }
    }

    template <typename ExtData, typename PkdStruct>
    void stream(const PackedSizedStructSO<ExtData, PkdStruct>& stream)
    {}
};


template <>
class ConfigureIOVectorViewFn<1> {

    int32_t stream_idx_{};
    io::IOVector& io_vector_;
public:
    ConfigureIOVectorViewFn(io::IOVector& io_vector):
        io_vector_(io_vector)
    {}

    template <typename Stream>
    void stream(const Stream& stream)
    {
        stream.configure_io_substream(io_vector_.substream(stream_idx_));
        stream_idx_++;
    }

    template <typename ExtData, typename PkdStruct>
    void stream(const PackedSizedStructSO<ExtData, PkdStruct>& stream)
    {
        io_vector_.symbol_sequence().configure(reinterpret_cast<void*>(stream.size()));
    }
};


}


template <typename CtrT, typename NodeType_>
class LeafNodeSO: public NodeCommonSO<CtrT, NodeType_> {
    using Base = NodeCommonSO<CtrT, NodeType_>;

    using Base::node_;
    using Base::ctr_;

    using MyType = LeafNodeSO;

public:

    using BranchNodeEntry = typename NodeType_::TypesT::BranchNodeEntry;
    using Position = typename NodeType_::TypesT::Position;

    template <template <typename> class, typename>
    friend class NodePageAdaptor;

    using BranchSubstreamsStructList    = typename NodeType_::TypesT::BranchStreamsStructList;
    using LeafSubstreamsStructList      = typename NodeType_::TypesT::LeafStreamsStructList;

    template <typename PkdT>
    using PkdExtDataT = typename PkdT::ExtData;

    using LeafSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<LeafSubstreamsStructList>>;
    using LeafExtData = MakeTuple<LeafSubstreamExtensionsList>;

    using BranchSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<BranchSubstreamsStructList>>;
    using BranchExtData = MakeTuple<BranchSubstreamExtensionsList>;

    using CtrPropertiesMap = PackedMap<Varchar, Varchar>;
    using CtrReferencesMap = PackedMap<Varchar, ProfileCtrID<typename NodeType_::TypesT::Profile>>;

    using RootMetadataList = MergeLists<
        typename NodeType_::TypesT::Metadata,
        PackedTuple<BranchExtData>,
        PackedTuple<LeafExtData>,
        CtrPropertiesMap,
        CtrReferencesMap
    >;

    using StreamDispatcherStructList = typename PackedStatefulDispatchersListBuilder<
            Linearize<LeafSubstreamsStructList>,
            NodeType_::StreamsStart
    >::Type;

    using Dispatcher = PackedStatefulDispatcher<
        LeafExtData,
        StreamDispatcherStructList,
        NodeType_::StreamsStart
    >;

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            list_tree::LeafCountInf<LeafSubstreamsStructList, SubstreamsPath>,
            list_tree::LeafCountSup<LeafSubstreamsStructList, SubstreamsPath>
    >;

    template <int32_t StreamIdx>
    using StreamDispatcher = SubstreamsDispatcher<IntList<StreamIdx>>;

    template <int32_t StreamIdx>
    using StreamStartIdx = IntValue<
            list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;

    template <int32_t StreamIdx>
    using StreamSize = IntValue<
            list_tree::LeafCountSup<LeafSubstreamsStructList, IntList<StreamIdx>> -
            list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;


    template <int32_t Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcher = typename Dispatcher::template SubsetDispatcher<
            list_tree::AddToValueList<
                list_tree::LeafCount<LeafSubstreamsStructList, IntList<Stream>>,
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
    using LeafPathT = typename list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <int32_t SubstreamIdx>
    using BranchPathT = typename list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;



    template <int32_t Stream, typename SubstreamIdxList, template <typename> class MapFn>
    using MapSubstreamsStructs  = typename SubstreamsByIdxDispatcher<Stream, SubstreamIdxList>::template ForAllStructs<MapFn>;

    template <int32_t Stream, template <typename> class MapFn>
    using MapStreamStructs      = typename StreamDispatcher<Stream>::template ForAllStructs<MapFn>;


    template <typename SubstreamPath>
    using PackedStruct = typename Dispatcher::template StreamTypeT<
            list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>
    >::Type;


    template <typename LeafPath>
    using SubstreamByLeafPath = PackedStruct<LeafPath>;

    template <typename LeafPath>
    static constexpr int32_t SubstreamIdxByLeafPath = list_tree::LeafCount<LeafSubstreamsStructList, LeafPath>;



    static constexpr int32_t Streams            = ListSize<LeafSubstreamsStructList>;

    static constexpr int32_t Substreams         = Dispatcher::Size;
    static constexpr int32_t DataStreams        = Streams == 1 ? Streams : Streams - 1;

    static constexpr int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static constexpr int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    using IOVectorT     = typename bt::_::IOVectorsTF<Streams, LeafSubstreamsStructList>::IOVectorT;
    using IOVectorViewT = typename bt::_::IOVectorViewTF<Streams, LeafSubstreamsStructList>::IOVectorT;



    LeafNodeSO(): Base() {}
    LeafNodeSO(CtrT* ctr): Base(ctr, nullptr) {}
    LeafNodeSO(CtrT* ctr, NodeType_* node):
        Base(ctr, node)
    {}

    void setup() {
        ctr_ = nullptr;
        node_ = nullptr;
    }

    void setup(CtrT* ctr) {
        ctr_ = ctr;
        node_ = nullptr;
    }

    void setup(CtrT* ctr, NodeType_* node) {
        ctr_ = ctr;
        node_ = node;
    }

    void setup(NodeType_* node) {
        node_ = node;
    }

    const LeafExtData& state() const {
        return ctr_->leaf_node_ext_data();
    }

    template <typename LeafPath, typename ExtData>
    void set_ext_data(ExtData&& data) const
    {
        constexpr int32_t substream_idx = SubstreamIdxByLeafPath<LeafPath>;
        std::get<substream_idx>(ctr_->leaf_node_ext_data()) = std::forward<ExtData>(data);
    }


    static std::unique_ptr<io::IOVector> create_iovector()
    {
        return std::make_unique<IOVectorT>();
    }

    std::unique_ptr<io::IOVector> create_iovector_view() const
    {
        auto iov = std::make_unique<IOVectorViewT>();
        configure_iovector_view(*iov.get());
        return iov;
    }

    void configure_iovector_view(io::IOVector& io_vector) const
    {
        Dispatcher(state()).dispatchAll(allocator(), _::ConfigureIOVectorViewFn<Streams>(io_vector)).get_or_throw();
    }



    VoidResult prepare() noexcept
    {
        return node_->initAllocator(SubstreamsStart + Substreams); // FIXME +1?
    }


    VoidResult layout(const Position& sizes) noexcept
    {
        return layout(-1ull);
    }


    struct LayoutFn
    {
        template <int32_t AllocatorIdx, int32_t Idx, typename Stream>
        VoidResult stream(Stream&, PackedAllocator* alloc, uint64_t streams) noexcept
        {
            if (streams & (1<<Idx))
            {
                if (alloc->is_empty(AllocatorIdx))
                {
                    MEMORIA_TRY_VOID(
                        alloc->template allocateEmpty<
                                typename Stream::PkdStructT
                        >(AllocatorIdx)
                    );
                }
            }

            return VoidResult::of();
        }
    };


    VoidResult layout(uint64_t streams) noexcept
    {
        return Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator(), streams);
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

    struct CheckFn {
        template <typename Tree>
        void stream(Tree&& tree)
        {
            tree.check();
        }
    };

    void check() const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), CheckFn()).get_or_throw();
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
        return bt::ForEachStream<Streams - 1>::process(stream, Size2Fn(), this);
    }

    struct SizeFn {
        template <typename Tree>
        int32_t stream(Tree&& tree)
        {
            return tree ? tree.size() : 0;
        }
    };

    template <int32_t StreamIdx>
    int32_t streamSize() const
    {
        return this->processStream<IntList<StreamIdx>>(SizeFn()).get_or_throw();
    }


    struct MemUsedFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree&& tree, const Position& sizes, int32_t* mem_used, int32_t except)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            if (StreamIdx != except)
            {
                int32_t size = sizes[StreamIdx];

                if (tree || size > 0)
                {
                    *mem_used += PkdTree::packed_block_size(size);
                }
            }
        }
    };



    struct CheckCapacitiesFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree&& tree, const Position& sizes, int32_t* mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            int32_t size = sizes[StreamIdx];

            if (tree || size > 0)
            {
                *mem_size += PkdTree::packed_block_size(size);
            }
        }


        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename Entropy>
        void stream(Tree&& tree, const Entropy& entropy, const Position& sizes, int32_t* mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            int32_t size = sizes[StreamIdx];

            if (tree || size > 0)
            {
                *mem_size += PkdTree::packed_block_size(size);
            }
        }
    };

    static int32_t free_space(int32_t block_size, bool root)
    {
        int32_t fixed_block_size = block_size - sizeof(NodeType_) + PackedAllocator::my_size();
        int32_t client_area = PackedAllocator::client_area(fixed_block_size, SubstreamsStart + Substreams + 1);

        return client_area - root * PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(typename NodeType_::TypesT::Metadata));
    }

    static int32_t client_area(int32_t block_size, bool root)
    {
        int32_t free_space = MyType::free_space(block_size, root);

        //FIXME Check if Streams value below is correct.
        return PackedAllocator::client_area(free_space, Streams);
    }

    bool checkCapacities(const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (int32_t c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        int32_t mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), fillment, &mem_size).get_or_throw();

        int32_t free_space      = this->free_space(node_->header().memory_block_size(), node_->is_root());
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        // FIXME: fix block size estimation and remove '350'
        return client_area >= mem_size + 350;
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

        processSubstreamGroups(CheckCapacitiesFn(), entropy, fillment, &mem_size).get_or_throw();

        int32_t free_space      = node_->free_space(node_->header().memory_block_size(), node_->is_root());
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }

    struct SizesFn {
        template <int32_t StreamIdx, typename Tree>
        void stream(Tree&& tree, Position& pos)
        {
            pos[StreamIdx] = tree ? tree.size() : 0;
        }
    };

    Position sizes() const
    {
        Position pos;
        processStreamsStart(SizesFn(), pos).get_or_throw();
        return pos;
    }

    int32_t single_stream_capacity(int32_t max_hops) const
    {
        int32_t min = sizes()[0];
        int32_t max = node_->header().memory_block_size() * 8;

        int32_t free_space      = this->free_space(node_->header().memory_block_size(), node_->is_root()) - 350;
        int32_t client_area     = PackedAllocator::client_area(free_space, Streams);

        int32_t total = FindTotalElementsNumber(min, max, client_area, max_hops, [&](int32_t stream_size){
            return stream_block_size(stream_size);
        });

        return total - min;
    }

    struct SingleStreamCapacityFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree&& tree, int32_t size, int32_t& mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            mem_size += PkdTree::packed_block_size(size);
        }
    };


    int32_t stream_block_size(int32_t size) const
    {
        int32_t mem_size = 0;

        StreamDispatcher<0>::dispatchAllStatic(SingleStreamCapacityFn(), size, mem_size).get_or_throw();

        return mem_size;
    }



    struct BranchNodeEntryHandler
    {
        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
        VoidResult stream(StreamType&& obj, TupleItem& accum, int32_t start, int32_t end)
        {
            if (obj)
            {
                obj.template sum<Offset>(start, end, accum);
            }

            return VoidResult::of();
        }

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
        VoidResult stream(StreamType&& obj, TupleItem& accum)
        {
            if (obj)
            {
                if (StreamStart)
                {
                    accum[Offset - 1] += obj.size();
                }

                obj.template sum<Offset>(accum);
            }

            return VoidResult::of();
        }

        template <int32_t Offset, bool StreamStart, int32_t ListIdx, typename StreamType, typename TupleItem>
        VoidResult stream(StreamType&& obj, TupleItem& accum, const Position& start, const Position& end)
        {
            const int32_t StreamIdx = bt::FindTopLevelIdx<LeafSubstreamsStructList, ListIdx>::Value;

            int32_t startIdx    = start[StreamIdx];
            int32_t endIdx      = end[StreamIdx];

            stream<Offset, StreamStart, ListIdx>(obj, accum, startIdx, endIdx);

            return VoidResult::of();
        }
    };

    struct BranchNodeEntryMaxHandler
    {

        template <int32_t Offset, bool StreamStart, int32_t Idx, typename StreamType, typename TupleItem>
        VoidResult stream(const StreamType& obj, TupleItem& accum) noexcept
        {
            if (obj)
            {
                obj.template max<Offset>(accum);
            }

            return VoidResult::of();
        }
    };

    void max(BranchNodeEntry& entry) const
    {
        processAllSubstreamsAcc(BranchNodeEntryMaxHandler(), entry).get_or_throw();
    }

    void sums(int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end).get_or_throw();
    }


    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processAllSubstreamsAcc(BranchNodeEntryHandler(), sums, start, end).get_or_throw();
    }






    struct RemoveSpaceFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        VoidResult stream(Tree&& tree, const Position& room_start, const Position& room_end) noexcept
        {
            return tree.removeSpace(room_start[StreamIdx], room_end[StreamIdx]);
        }

        template <typename Tree>
        VoidResult stream(Tree&& tree, int32_t room_start, int32_t room_end) noexcept
        {
            return tree.removeSpace(room_start, room_end);
        }
    };

    VoidResult removeSpace(int32_t stream, int32_t room_start, int32_t room_end) noexcept
    {
        RemoveSpaceFn fn;
        return Dispatcher(state()).dispatch(stream, allocator(), fn, room_start, room_end);
    }

    VoidResult removeSpace(const Position& room_start, const Position& room_end) noexcept
    {
        RemoveSpaceFn fn;
        return processSubstreamGroups(fn, room_start, room_end);
    }



    const PackedAllocator* allocator() const {
        return node_->allocator();
    }

    PackedAllocator* allocator() {
        return node_->allocator();
    }

    struct LeafSumsFn {
        template <typename StreamType>
        auto stream(const StreamType& obj, int32_t start, int32_t end)
        {
            return obj ? obj.sum(start, end) : decltype(obj.sum(start, end))();
        }

        template <typename StreamType>
        auto stream(const StreamType& obj, int32_t block, int32_t start, int32_t end)
        {
            return obj ? obj.sum(block, start, end) : 0;
        }
    };


    template <typename Path, typename... Args>
    auto leaf_sums(Args&&... args) const
    {
        return processStream<Path>(LeafSumsFn(), std::forward<Args>(args)...);
    }

    struct MergeWithFn {

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        VoidResult stream(Tree& tree, OtherNodeT&& other) noexcept
        {
            int32_t size = tree.size();

            if (size > 0)
            {
                Dispatcher other_disp = other.dispatcher();

                if (other.allocator()->is_empty(AllocatorIdx))
                {
                    MEMORIA_TRY_VOID(other_disp.template allocateEmpty<Idx>(other.allocator()));
                }

                Tree other_tree = other_disp.template get<Idx>(other.allocator());
                return tree.mergeWith(other_tree);
            }

            return VoidResult::of();
        }
    };

    template <typename OtherNodeT>
    VoidResult mergeWith(OtherNodeT&& other) noexcept
    {
        MergeWithFn fn;
        return Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other));
    }


    struct CanMergeWithFn {
        int32_t mem_used_ = 0;

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        void stream(Tree& tree, OtherNodeT&& other)
        {
            using PkdTree = typename Tree::PkdStructT;

            if (tree)
            {
                if (other.allocator()->is_empty(AllocatorIdx))
                {
                    mem_used_ += tree.data()->block_size();
                }
                else {
                    const PkdTree* other_tree = other.allocator()->template get<PkdTree>(AllocatorIdx);
                    mem_used_ += tree.data()->block_size(other_tree);
                }
            }
            else {
                if (!other.allocator()->is_empty(AllocatorIdx))
                {
                    int32_t element_size = other.allocator()->element_size(AllocatorIdx);
                    mem_used_ += element_size;
                }
            }
        }
    };

    template <typename OtherNodeT>
    BoolResult canBeMergedWith(OtherNodeT&& other) const noexcept
    {
        CanMergeWithFn fn;
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchAll(allocator(), fn, std::forward<OtherNodeT>(other)));

        int32_t client_area = other.allocator()->client_area();

        return BoolResult::of(client_area >= fn.mem_used_);
    }

    bool shouldBeMergedWithSiblings() const
    {
        return node_->shouldBeMergedWithSiblings();
    }


    struct CopyToFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(
                Tree&& tree,
                MyType& other,
                const Position& copy_from,
                const Position& count,
                const Position& copy_to
        )
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            tree->copyTo(
                    other.allocator()->template get<PkdTree>(AllocatorIdx),
                    copy_from[StreamIdx],
                    count[StreamIdx],
                    copy_to[StreamIdx]
            );
        }
    };


    template <typename OtherNodeT>
    void copyTo(OtherNodeT&& other, const Position& copy_from, const Position& count, const Position& copy_to) const
    {
        MEMORIA_V1_ASSERT_TRUE((copy_from + count).lteAll(sizes()));
        MEMORIA_V1_ASSERT_TRUE((copy_to + count).lteAll(other->max_sizes()));

        processSubstreamGroups(CopyToFn(), std::forward<OtherNodeT>(other), copy_from, count, copy_to);
    }

    struct SplitToFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        VoidResult stream(Tree& tree, OtherNodeT&& other, const Position& indexes) noexcept
        {
            if (tree)
            {
                int32_t idx   = indexes[StreamIdx];
                int32_t size  = tree.size();

                MEMORIA_V1_ASSERT(idx, >=, 0);
                MEMORIA_V1_ASSERT(idx, <=, size);

                Dispatcher other_disp = other.dispatcher();

                Tree other_tree;

                constexpr int32_t ListIdx = AllocatorIdx - NodeType_::StreamsStart;

                if (!other.allocator()->is_empty(AllocatorIdx))
                {
                    other_tree = other_disp.template get<ListIdx>(other.allocator());
                }
                else {
                    MEMORIA_TRY(other_tree_tmp, other_disp.template allocateEmpty<ListIdx>(other.allocator()));
                    other_tree = std::move(other_tree_tmp);
                }

                return tree.splitTo(other_tree, idx);
            }

            return VoidResult::of();
        }
    };


    template <typename OtherNodeT>
    VoidResult splitTo(OtherNodeT&& other, const Position& from) noexcept
    {
        SplitToFn split_fn;
        return processSubstreamGroups(split_fn, std::forward<OtherNodeT>(other), from);
    }


    struct SizeSumsFn {
        template <int32_t ListIdx, typename Tree>
        void stream(Tree&& tree, Position& sizes)
        {
            sizes[ListIdx] = tree ? tree.size() : 0;
        }
    };

    Position size_sums() const
    {
        Position sums;
        processStreamsStart(SizeSumsFn(), sums).get_or_throw();
        return sums;
    }


    struct GenerateDataEventsFn {
        template <int32_t Idx, typename Tree>
        VoidResult stream(Tree&& tree, IBlockDataEventHandler* handler) noexcept
        {
            return tree.generateDataEvents(handler);
        }
    };

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        MEMORIA_TRY_VOID(node_->template generateDataEvents<RootMetadataList>(handler));
        return Dispatcher(state()).dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);
    }

    VoidResult init_root_metadata() noexcept {
        return node_->template init_root_metadata<RootMetadataList>();
    }



    struct DumpFn {
        template <typename Tree>
        void stream(Tree&& tree)
        {
            tree->dump();
        }
    };


    void dump() const {
        Dispatcher(state()).dispatchNotEmpty(allocator(), DumpFn()).get_or_throw();
    }


    struct DumpBlockSizesFn {
        template <typename Tree>
        void stream(Tree&& tree)
        {
            std::cout << tree->memory_block_size() << std::endl;
        }
    };

    void dumpBlockSizes() const {
        Dispatcher(state()).dispatchNotEmpty(allocator(), DumpBlockSizesFn()).get_or_throw();
    }


    /*********************************************************/

    Dispatcher dispatcher() const {
        return Dispatcher(state());
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchAll(Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state()).dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args) noexcept
    {
        return Dispatcher(state())
                .dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) noexcept
    {
        return Dispatcher(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args) const noexcept
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args) noexcept
    {
        return SubstreamsDispatcher<SubstreamsPath>()
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
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
        VoidResult stream(StreamType&& obj, Fn&& fn, Accum&& accum, Args&&... args) noexcept
        {
            constexpr int32_t LeafIdx = BranchNodeEntryIdx - SubstreamsStart;

            constexpr int32_t BranchStructIdx   = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::BranchStructIdx;
            constexpr int32_t LeafOffset        = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::LeafOffset;
            constexpr bool IsStreamStart        = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::IsStreamStart;

            return fn.template stream<LeafOffset, IsStreamStart, ListIdx>(
                    std::forward<StreamType>(obj),
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
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const noexcept
    {
        return StreamDispatcher<Stream>(state()).dispatchAll(
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
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) noexcept
    {
        return StreamDispatcher<Stream>(state()).dispatchAll(
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
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const noexcept
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>(state()).dispatchAll(
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
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args) noexcept
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>(state()).dispatchAll(
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
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const noexcept
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>(state()).dispatchAll(
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
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) noexcept
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>(state()).dispatchAll(
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
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const noexcept
    {
        return Dispatcher(state()).dispatchAll(
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
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) noexcept
    {
        return Dispatcher(state()).dispatchAll(
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
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) const noexcept
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>(state()).dispatchAll(
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
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) noexcept
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>(state()).dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }



    template <typename SubstreamPath>
    auto substream()
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;

        if (!this->allocator()->is_empty(SubstreamIdx + SubstreamsStart))
        {
            return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
        }
        else {
            return static_cast<T*>(nullptr);
        }
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;

        if (!this->allocator()->is_empty(SubstreamIdx + SubstreamsStart))
        {
            return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
        }
        else {
            return static_cast<const T*>(nullptr);
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
    auto processStream(Fn&& fn, Args&&... args) const noexcept
    {
        const int32_t StreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) noexcept
    {
        const int32_t StreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) noexcept
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                state(),
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const noexcept
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                state(),
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args) noexcept
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;
        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) noexcept
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const noexcept
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    static auto processStreamsStartStatic(Fn&& fn, Args&&... args) noexcept
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename Accum, typename... Args>
    static auto processStreamsStartStaticAcc(Fn&& fn, Accum&& accum, Args&&... args) noexcept
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                std::forward<Accum>(accum),
                std::forward<Args>(args)...
        );
    }
};

}
