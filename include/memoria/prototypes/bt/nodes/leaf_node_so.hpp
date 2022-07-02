
// Copyright 2019-2022 Victor Smirnov
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


#include <memoria/prototypes/bt/tools/bt_tools_batch_input.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>

#include <memoria/prototypes/bt/pkd_adapters/bt_pkd_adapter_generic.hpp>


#include <memoria/core/packed/tools/packed_stateful_dispatcher.hpp>
#include <memoria/core/packed/tools/packed_stateful_dispatcher_res.hpp>

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/checks.hpp>


namespace memoria {


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

    using DispatcherWithResult = PackedStatefulDispatcherWithResult<
        LeafExtData,
        StreamDispatcherStructList,
        NodeType_::StreamsStart
    >;

    template <size_t StartIdx, size_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;

    template <size_t StartIdx, size_t EndIdx>
    using SubrangeDispatcherWithResult = typename DispatcherWithResult::template SubrangeDispatcher<StartIdx, EndIdx>;

    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            list_tree::LeafCountInf<LeafSubstreamsStructList, SubstreamsPath>,
            list_tree::LeafCountSup<LeafSubstreamsStructList, SubstreamsPath>
    >;

    template <typename SubstreamsPath>
    using SubstreamsDispatcherWithResult = SubrangeDispatcherWithResult<
            list_tree::LeafCountInf<LeafSubstreamsStructList, SubstreamsPath>,
            list_tree::LeafCountSup<LeafSubstreamsStructList, SubstreamsPath>
    >;

    template <size_t StreamIdx>
    using StreamDispatcher = SubstreamsDispatcher<IntList<StreamIdx>>;

    template <size_t StreamIdx>
    using StreamDispatcherWithResult = SubstreamsDispatcherWithResult<IntList<StreamIdx>>;

    template <size_t StreamIdx>
    using StreamStartIdx = IntValue<
            list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;

    template <size_t StreamIdx>
    using StreamSize = IntValue<
            list_tree::LeafCountSup<LeafSubstreamsStructList, IntList<StreamIdx>> -
            list_tree::LeafCountInf<LeafSubstreamsStructList, IntList<StreamIdx>>
    >;


    template <size_t Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcher = typename Dispatcher::template SubsetDispatcher<
            list_tree::AddToValueList<
                list_tree::LeafCount<LeafSubstreamsStructList, IntList<Stream>>,
                SubstreamIdxList
            >,
            Stream
    >;

    template <size_t Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcherWithResult = typename DispatcherWithResult::template SubsetDispatcher<
            list_tree::AddToValueList<
                list_tree::LeafCount<LeafSubstreamsStructList, IntList<Stream>>,
                SubstreamIdxList
            >,
            Stream
    >;



    template <size_t Stream>
    using BTTLStreamDataDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1
    >;

    template <size_t Stream>
    using BTTLStreamDataDispatcherWithResult = SubrangeDispatcherWithResult<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1
    >;


    template <size_t Stream>
    using BTTLLastStreamDataDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;

    template <size_t Stream>
    using BTTLLastStreamDataDispatcherWithResult = SubrangeDispatcherWithResult<
            StreamStartIdx<Stream>::Value,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;


    template <size_t Stream>
    using BTTLStreamSizesDispatcher = SubrangeDispatcher<
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;


    template <size_t Stream>
    using BTTLStreamSizesDispatcherWithResult = SubrangeDispatcherWithResult<
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value - 1,
            StreamStartIdx<Stream>::Value + StreamSize<Stream>::Value
    >;


    template <size_t SubstreamIdx>
    using LeafPathT = typename list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <size_t SubstreamIdx>
    using BranchPathT = typename list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;



    template <size_t Stream, typename SubstreamIdxList, template <typename> class MapFn>
    using MapSubstreamsStructs  = typename SubstreamsByIdxDispatcher<Stream, SubstreamIdxList>::template ForAllStructs<MapFn>;

    template <size_t Stream, template <typename> class MapFn>
    using MapStreamStructs      = typename StreamDispatcher<Stream>::template ForAllStructs<MapFn>;


    template <typename SubstreamPath>
    using PackedStruct = typename Dispatcher::template StreamTypeT<
            list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>
    >::Type;


    template <typename LeafPath>
    using SubstreamByLeafPath = PackedStruct<LeafPath>;

    template <typename LeafPath>
    static constexpr size_t SubstreamIdxByLeafPath = list_tree::LeafCount<LeafSubstreamsStructList, LeafPath>;

    template <typename PkdStruct>
    using PkdStructToUpdateStateMap = HasType<typename PkdStruct::Type::SparseObject::UpdateState>;

    template <typename LeafPath = IntList<>>
    using UpdateState = AsTuple<
        MergeLists<
            typename SubstreamsDispatcher<LeafPath>::template ForAllStructs<PkdStructToUpdateStateMap>,
            PackedAllocatorUpdateState
        >
    >;

    static constexpr size_t Streams            = ListSize<LeafSubstreamsStructList>;

    static constexpr size_t Substreams         = Dispatcher::Size;
    static constexpr size_t DataStreams        = Streams == 1 ? Streams : Streams - 1;

    static constexpr size_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static constexpr size_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static const PackedDataTypeSize SizeType = PackedListStructSizeType<Linearize<LeafSubstreamsStructList>>::Value;

    LeafNodeSO() : Base() {}
    LeafNodeSO(CtrT* ctr) : Base(ctr, nullptr) {}
    LeafNodeSO(CtrT* ctr, NodeType_* node) :
        Base(ctr, node)
    {}

    void setup()  {
        ctr_ = nullptr;
        node_ = nullptr;
    }

    void setup(CtrT* ctr)  {
        ctr_ = ctr;
        node_ = nullptr;
    }

    void setup(CtrT* ctr, NodeType_* node)  {
        ctr_ = ctr;
        node_ = node;
    }

    void setup(NodeType_* node)  {
        node_ = node;
    }

    LeafExtData& state() const  {
        return ctr_->leaf_node_ext_data();
    }

    template <typename LeafPath, typename ExtData>
    void set_ext_data(ExtData&& data) const
    {
        constexpr size_t substream_idx = SubstreamIdxByLeafPath<LeafPath>;
        std::get<substream_idx>(ctr_->leaf_node_ext_data()) = std::forward<ExtData>(data);
    }


    template <typename OtherNode>
    void copy_node_data_to(OtherNode&& other) const
    {
        PackedAllocator* other_alloc = other.allocator();
        const PackedAllocator* my_alloc = this->allocator();

        for (size_t c = 0; c < SubstreamsEnd; c++)
        {
            other_alloc->import_block(c, my_alloc, c);
        }
    }


    struct CoWRefChildrenFn {
        template <typename Tree, typename WCtrT>
        void stream(Tree&&, WCtrT&) const
        {
        }

        template <typename ExtData, bool Indexed, typename ValueHolder, typename WCtrT>
        void stream(
                PackedDataTypeBufferSO<
                    ExtData,
                    PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<ValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                    >
                >& buffer,
                WCtrT& ctr
        ) const {
            auto ii = buffer.begin(0);
            auto end = buffer.end(0);

            for (; ii != end; ii++)
            {
                ctr.store().ref_block(*ii);
            }
        }
    };

    template <typename WCtrT>
    void cow_ref_children(WCtrT& ctr)  {
        return processAll(CoWRefChildrenFn(), ctr);
    }


    struct CoWUnRefChildrenFn {

        template <typename Tree, typename Store>
        void stream(Tree&& tree, Store&) const
        {
            (void)tree;
        }

        // Note that the way how this handler is implemented
        // is fallible to one hard-to-spot error. If, because of
        // whatever reason, PackedDataTypeBufferSO<...> has different
        // type than it is specified in the signature below,
        // the compilation sielently will go to the generic method
        // above, that does nothing.
        //
        // As the result, underling blocks will not be unref()-ed.

        template <typename ExtData, bool Indexed, typename Store, typename ValueHolder>
        void stream(
                const PackedDataTypeBufferSO<
                    ExtData,
                    PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<ValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                    >
                >& buffer,
                Store& store
        ) const
        {
            auto ii = buffer.begin(0);
            auto end = buffer.end(0);

            for (; ii != end; ii++) {
                store.unref_block(*ii);
            }
        }
    };

    template <typename Store>
    void cow_unref_children(Store& store) const  {
        return processAll(CoWUnRefChildrenFn(), store);
    }


    struct ForAllCtrRootIDsFn {
        template <typename Tree, typename BlockProcessorFn>
        void stream(const Tree&, const BlockProcessorFn&) const
        {

        }

        template <
                typename ExtData,
                bool Indexed,
                typename ValueHolder,
                typename BlockProcessorFn
        >
        void stream(
                const PackedDataTypeBufferSO<
                    ExtData,
                    PackedDataTypeBuffer<
                        PackedDataTypeBufferTypes<
                            CowBlockID<ValueHolder>,
                            Indexed,
                            1,
                            DTOrdering::UNORDERED
                        >
                    >
                >& buffer,
                const BlockProcessorFn& fn
        ) const
        {
            auto ii = buffer.begin(0);
            auto end = buffer.end(0);

            for (; ii != end; ii++)
            {
                fn(*ii);
            }
        }
    };

    template <typename BlockID>
    void for_all_ctr_root_ids(const std::function<void (const BlockID&)>& fn) const  {
        return processAll(ForAllCtrRootIDsFn(), fn);
    }


    void prepare()
    {
        return node_->initAllocator(SubstreamsStart + Substreams); // FIXME +1?
    }

    struct LayoutFn
    {
        template <size_t AllocatorIdx, size_t Idx, typename Stream>
        void stream(Stream&, PackedAllocator* alloc)
        {            
            if (alloc->is_empty(AllocatorIdx))
            {
                alloc->template allocate_default<
                    typename Stream::PkdStructT
                >(AllocatorIdx);
            }
        }
    };


    void layout()
    {
        return Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator());
    }


    struct CheckFn {
        template <typename Tree>
        void stream(Tree&& tree)
        {
            return tree.check();
        }
    };

    void check(const CheckResultConsumerFn& consumer) const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), CheckFn());
    }

    struct Size2Fn {
        template <size_t StreamIdx, typename T>
        size_t process(const T* node)
        {
            return node->template streamSize<StreamIdx>();
        }
    };


    size_t size(size_t stream) const
    {
        return bt::ForEachStream<Streams - 1>::process(stream, Size2Fn(), this);
    }

    struct SizeFn {
        template <typename Tree>
        size_t stream(Tree&& tree)
        {
            return tree ? tree.size() : 0;
        }
    };

    template <size_t StreamIdx>
    size_t streamSize() const
    {
        return this->processStream<IntList<StreamIdx>>(SizeFn());
    }


    struct MemUsedFn {
        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree>
        void stream(Tree&& tree, const Position& sizes, size_t* mem_used, size_t except)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            if (StreamIdx != except)
            {
                size_t size = sizes[StreamIdx];

                if (tree || size > 0)
                {
                    *mem_used += PkdTree::packed_block_size(size);
                }
            }
        }
    };



    struct CheckCapacitiesFn {

        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree>
        void stream(Tree&& tree, const Position& sizes, size_t* mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            size_t size = sizes[StreamIdx];

            if (tree || size > 0)
            {
                *mem_size += PkdTree::packed_block_size(size);
            }
        }


        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree, typename Entropy>
        void stream(Tree&& tree, const Entropy& entropy, const Position& sizes, size_t* mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            size_t size = sizes[StreamIdx];

            if (tree || size > 0)
            {
                *mem_size += PkdTree::packed_block_size(size);
            }
        }
    };


    bool checkCapacities(const Position& sizes) const
    {
        auto fillment = this->sizes();

        for (size_t c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        size_t mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), fillment, &mem_size);

        size_t client_area = node_->compute_streams_available_space();

        return client_area >= mem_size;
    }


    template <typename Entropy>
    bool checkCapacities(const Entropy& entropy, const Position& sizes) const
    {
        auto fillment = this->sizes();

        for (size_t c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        size_t mem_size = 0;

        processSubstreamGroups(CheckCapacitiesFn(), entropy, fillment, &mem_size);

        size_t client_area = node_->compute_streams_available_space();
        return client_area >= mem_size;
    }

    struct SizesFn {
        template <size_t StreamIdx, typename Tree>
        void stream(Tree&& tree, Position& pos)
        {
            pos[StreamIdx] = tree ? tree.size() : 0;
        }
    };

    Position sizes() const
    {
        Position pos;
        processStreamsStart(SizesFn(), pos);
        return pos;
    }

    size_t single_stream_capacity(size_t max_hops) const
    {
        auto sizes = this->sizes();
        size_t min = sizes[0];
        size_t max = node_->header().memory_block_size() * 8;

        size_t client_area = node_->compute_streams_available_space();

        auto total = FindTotalElementsNumber(min, max, client_area, max_hops, [&](size_t stream_size){
            return stream_block_size(stream_size);
        });

        return total - min;
    }

    struct SingleStreamCapacityFn {
        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree>
        void stream(Tree&& tree, size_t size, size_t& mem_size)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
            mem_size += PkdTree::compute_block_size(size);
        }
    };


    size_t stream_block_size(size_t size) const
    {
        size_t mem_size = 0;
        StreamDispatcher<0>::dispatchAllStatic(SingleStreamCapacityFn(), size, mem_size);
        return mem_size;
    }



    struct BranchNodeEntriesSumHandler
    {
        template <size_t Offset, bool StreamStart, size_t ListIdx, typename StreamType, typename TupleItem>
        void stream(StreamType&& obj, TupleItem& accum, const Position& start, const Position& end)
        {
            if (obj)
            {
                bt::BTPkdStructAdaper<StreamType> adapter(obj);
                adapter.template sum<Offset>(start, end, accum);
            }
        }
    };

    struct BranchNodeEntryMaxHandler
    {
        template <size_t Offset, bool StreamStart, size_t Idx, typename StreamType, typename TupleItem>
        void stream(const StreamType& obj, TupleItem& accum)
        {
            if (obj)
            {
                bt::BTPkdStructAdaper<StreamType> adapter(obj);
                adapter.template leaf_max_entry<Offset>(accum);
            }
        }
    };

    void max(BranchNodeEntry& entry) const
    {
        processAllSubstreamsAcc(BranchNodeEntryMaxHandler(), entry);
    }


    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        return processAllSubstreamsAcc(BranchNodeEntriesSumHandler(), sums, start, end);
    }

    struct PrepareRemoveSpaceFn {
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree, typename UpdateState>
        void stream(Tree&& tree, const Position& start, const Position& end, UpdateState& update_state)
        {
            if (is_success(status)) {
                status = tree.prepare_remove(
                        start[StreamIdx],
                        end[StreamIdx],
                        std::get<AllocatorIdx - NodeType_::StreamsStart>(update_state)
                );
            }
        }


        template <size_t ListIdx, typename Tree, typename UpdateState>
        void stream(Tree&& tree, size_t start, size_t end, UpdateState& update_state)
        {
            if (is_success(status)) {
                status = tree.prepare_remove(
                        start,
                        end,
                        std::get<ListIdx>(update_state)
                );
            }
        }
    };


    PkdUpdateStatus prepare_remove(const Position& start, const Position& end, UpdateState<IntList<>>& update_state)
    {
        if (SizeType == PackedDataTypeSize::VARIABLE) {
            PrepareRemoveSpaceFn fn;
            processSubstreamGroups(fn, start, end, update_state);
            return fn.status;
        }
        else {
            return PkdUpdateStatus::SUCCESS;
        }
    }

    template <size_t Stream>
    PkdUpdateStatus prepare_remove(SizeTList<Stream> tag, size_t start, size_t end, UpdateState<IntList<Stream>>& update_state) const
    {
        PrepareRemoveSpaceFn fn;
        processSubstreams<IntList<Stream>>(fn, start, end, update_state);
        return fn.status;
    }


    struct CommitRemoveSpaceFn {
        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree, typename UpdateState>
        void stream(Tree&& tree, const Position& start, const Position& end, UpdateState& update_state)
        {
            return tree.commit_remove(
                        start[StreamIdx],
                        end[StreamIdx],
                        std::get<AllocatorIdx - NodeType_::StreamsStart>(update_state)
            );
        }

        template <size_t ListIdx, typename Tree, typename UpdateState>
        void stream(Tree&& tree, size_t start, size_t end, UpdateState& update_state)
        {
            return tree.commit_remove(
                        start,
                        end,
                        std::get<ListIdx>(update_state)
            );
        }
    };

    void commit_remove(const Position& start, const Position& end, UpdateState<IntList<>>& update_state)
    {
        CommitRemoveSpaceFn fn;
        return processSubstreamGroups(fn, start, end, update_state);
    }

    template <size_t Stream>
    void commit_remove(SizeTList<Stream>, size_t start, size_t end, UpdateState<IntList<Stream>>& update_state)
    {
        CommitRemoveSpaceFn fn;
        return processSubstreams<IntList<Stream>>(fn, start, end, update_state);
    }

    PkdUpdateStatus remove(const Position& start, const Position& end)
    {
        auto update_state = make_update_state<IntList<>>();
        if (is_success(prepare_remove(start, end, update_state))) {
            commit_remove(start, end, update_state);
            return PkdUpdateStatus::SUCCESS;
        }

        return PkdUpdateStatus::FAILURE;
    }


    template <size_t Stream>
    PkdUpdateStatus remove(SizeTList<Stream> tag, size_t start, size_t end)
    {
        auto update_state = make_update_state<IntList<Stream>>();
        if (is_success(prepare_remove(tag, start, end, update_state))) {
            commit_remove(tag, start, end, update_state);
            return PkdUpdateStatus::SUCCESS;
        }

        return PkdUpdateStatus::FAILURE;
    }


    const PackedAllocator* allocator() const {
        return node_->allocator();
    }

    PackedAllocator* allocator() {
        return node_->allocator();
    }

    struct LeafSumsFn {
        template <typename StreamType>
        auto stream(const StreamType& obj, size_t start, size_t end)
        {
            return obj ? obj.sum(start, end) : decltype(obj.sum(start, end))();
        }

        template <typename StreamType>
        auto stream(const StreamType& obj, size_t block, size_t start, size_t end)
        {
            return obj ? obj.sum(block, start, end) : 0;
        }
    };


    template <typename Path, typename... Args>
    auto leaf_sums(Args&&... args) const
    {
        return processStream<Path>(LeafSumsFn(), std::forward<Args>(args)...);
    }

    struct CommitMergeWithFn {

        template <size_t AllocatorIdx, size_t Idx, typename Tree, typename OtherNodeT, typename UpdateState>
        void stream(const Tree& tree, OtherNodeT&& other, UpdateState& update_state)
        {
            size_t size = tree.size();
            if (size)
            {
                Dispatcher other_disp = other.dispatcher();
                Tree other_tree = other_disp.template get<Idx>(other.allocator());
                return tree.commit_merge_with(other_tree, std::get<Idx>(update_state));
            }
        }
    };

    template <typename OtherNodeT, typename UpdateState>
    void commit_merge_with(OtherNodeT&& other, UpdateState& update_state) const
    {
        CommitMergeWithFn fn;
        return Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other), update_state);
    }

    template <typename OtherNodeT>
    PkdUpdateStatus merge_with(OtherNodeT&& other) const
    {
        auto update_state = other.template make_update_state<IntList<>>();
        if (is_success(prepare_merge_with(std::forward<OtherNodeT>(other), update_state)))
        {
            commit_merge_with(std::forward<OtherNodeT>(other), update_state);\
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }

    struct PrepareMergeWithFn {
        PkdUpdateStatus status_{PkdUpdateStatus::SUCCESS};

        template <size_t AllocatorIdx, size_t Idx, typename Tree, typename OtherNodeT, typename UpdateState>
        void stream(const Tree& tree, OtherNodeT&& other, UpdateState& update_state)
        {
            if (status_ == PkdUpdateStatus::SUCCESS)
            {
                size_t size = tree.size();
                if (size) {
                    Dispatcher other_disp = other.dispatcher();

                    Tree other_tree = other_disp.template get<Idx>(other.allocator());
                    status_ = tree.prepare_merge_with(other_tree, std::get<Idx>(update_state));
                }
            }
        }
    };

    template <typename OtherNodeT, typename UpdateState>
    PkdUpdateStatus prepare_merge_with(OtherNodeT&& other, UpdateState& update_state) const {
        PrepareMergeWithFn fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other), update_state);
        return fn.status_;
    }



    struct CanMergeWithFn {
        size_t mem_used_ = 0;

        template <size_t AllocatorIdx, size_t Idx, typename Tree, typename OtherNodeT>
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
                    mem_used_ += tree.data()->block_size_for(other_tree);
                }
            }
            else {
                if (!other.allocator()->is_empty(AllocatorIdx))
                {
                    size_t element_size = other.allocator()->element_size(AllocatorIdx);
                    mem_used_ += element_size;
                }
            }
        }
    };

    template <typename OtherNodeT>
    bool canBeMergedWith(OtherNodeT&& other) const
    {
        CanMergeWithFn fn;
        MEMORIA_TRY_VOID(DispatcherWithResult(state()).dispatchAll(allocator(), fn, std::forward<OtherNodeT>(other)));

        size_t client_area = other.allocator()->client_area();

        return client_area >= fn.mem_used_;
    }

    bool shouldBeMergedWithSiblings() const
    {
        return node_->shouldBeMergedWithSiblings();
    }


    struct SplitToFn {

        template <size_t StreamIdx, size_t AllocatorIdx, size_t Idx, typename Tree, typename OtherNodeT>
        void stream(Tree& tree, OtherNodeT&& other, const Position& indexes)
        {
            if (tree)
            {
                size_t idx   = indexes[StreamIdx];
                size_t size  = tree.size();

                MEMORIA_ASSERT(idx, >=, 0);
                MEMORIA_ASSERT(idx, <=, size);

                Dispatcher other_disp = other.dispatcher();

                Tree other_tree;

                constexpr size_t ListIdx = AllocatorIdx - NodeType_::StreamsStart;

                if (!other.allocator()->is_empty(AllocatorIdx))
                {
                    other_tree = other_disp.template get<ListIdx>(other.allocator());
                }
                else {
                    other_tree = other_disp.template allocate_empty<ListIdx>(other.allocator());
                }

                return tree.split_to(other_tree, idx);
            }
        }
    };


    template <typename OtherNodeT>
    void split_to(OtherNodeT&& other, const Position& from)
    {
        return processSubstreamGroups(SplitToFn(), std::forward<OtherNodeT>(other), from);
    }


    struct SizeSumsFn {
        template <size_t ListIdx, typename Tree>
        void stream(Tree&& tree, Position& sizes)
        {
            sizes[ListIdx] = tree ? tree.size() : 0;
        }
    };

    Position size_sums() const
    {
        Position sums;
        processStreamsStart(SizeSumsFn(), sums);
        return sums;
    }


    struct GenerateDataEventsFn {
        template <size_t Idx, typename Tree>
        void stream(Tree&& tree, IBlockDataEventHandler* handler)
        {
            return tree.generateDataEvents(handler);
        }
    };

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        node_->template generateDataEvents<RootMetadataList>(handler);
        Dispatcher(state()).dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);
    }

    void init_root_metadata() {
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
        Dispatcher(state()).dispatchNotEmpty(allocator(), DumpFn());
    }


    struct DumpBlockSizesFn {
        template <typename Tree>
        void stream(Tree&& tree)
        {
            std::cout << tree->memory_block_size() << std::endl;
        }
    };

    void dumpBlockSizes() const {
        Dispatcher(state()).dispatchNotEmpty(allocator(), DumpBlockSizesFn());
    }


    /*********************************************************/

    Dispatcher dispatcher() const {
        return Dispatcher(state());
    }

    template <typename Fn, typename... Args>
    void dispatchAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    auto process(size_t stream, Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state()).dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto process(size_t stream, Fn&& fn, Args&&... args)
    {
        return Dispatcher(state())
                .dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args)
    {
        return Dispatcher(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreamsVoidRes(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcherWithResult<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreamsVoidRes(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcherWithResult<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    struct ProcessSubstreamsAccFnAdaptor
    {
        template <
            size_t BranchNodeEntryIdx,
            size_t ListIdx,
            typename StreamType,
            typename Accum,
            typename Fn,
            typename... Args
        >
        auto stream(StreamType&& obj, Fn&& fn, Accum&& accum, Args&&... args)
        {
            constexpr size_t LeafIdx = BranchNodeEntryIdx - SubstreamsStart;

            constexpr size_t BranchStructIdx   = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::BranchStructIdx;
            constexpr size_t LeafOffset        = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::LeafOffset;
            constexpr bool IsStreamStart        = bt::LeafToBranchIndexByValueTranslator<LeafSubstreamsStructList, LeafIdx>::IsStreamStart;

            return fn.template stream<LeafOffset, IsStreamStart, ListIdx>(
                    std::forward<StreamType>(obj),
                    std::get<BranchStructIdx>(accum),
                    std::forward<Args>(args)...
            );
        }
    };



    template <
        size_t Stream,
        typename Fn,
        typename... Args
    >
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
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
        size_t Stream,
        typename Fn,
        typename... Args
    >
    auto processStreamAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
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
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
    {
        const size_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
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
    auto processStreamAccP(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
    {
        const size_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return SubrangeDispatcher<SubstreamIdx, SubstreamIdx + 1>(state()).dispatchAll(
                allocator(),
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                accum,
                std::forward<Args>(args)...
        );
    }


    template <
        size_t Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
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
        size_t Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdxAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
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
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args) const
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
    auto processAllSubstreamsAcc(Fn&& fn, BranchNodeEntry& accum, Args&&... args)
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
        size_t Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) const
    {
        return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>(state()).dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <
        size_t Stream,
        typename SubstreamsIdxList,
        typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args)
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
        const size_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const size_t SubstreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }

    template <size_t SubstreamIdx>
    auto substream_by_idx()
    {
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }

    template <size_t SubstreamIdx>
    auto substream_by_idx() const
    {
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }





    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const size_t StreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const size_t StreamIdx = list_tree::LeafCount<LeafSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args)
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
    auto processSubstreamGroupsVoidRes(Fn&& fn, Args&&... args)
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;

        return bt::GroupDispatcherWithResult<DispatcherWithResult, GroupsList>::dispatchGroups(
                state(),
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const
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
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<LeafSubstreamsStructList>;
        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamStart(size_t stream, Fn&& fn, Args&&... args)
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamStart(size_t stream, Fn&& fn, Args&&... args) const
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return typename Dispatcher::template SubsetDispatcher<Subset>(state()).template dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    static auto processStreamsStartStatic(Fn&& fn, Args&&... args)
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename Accum, typename... Args>
    static auto processStreamsStartStaticAcc(Fn&& fn, Accum&& accum, Args&&... args)
    {
        using Subset = bt::StreamsStartSubset<LeafSubstreamsStructList>;
        return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAllStatic(
                ProcessSubstreamsAccFnAdaptor(),
                std::forward<Fn>(fn),
                std::forward<Accum>(accum),
                std::forward<Args>(args)...
        );
    }

    template <typename LeafPath>
    UpdateState<LeafPath> make_update_state()
    {
        UpdateState<LeafPath> state;
        bt::get_allocator_update_state(state) = allocator()->make_allocator_update_state();

        bt::UpdateStateInitializer<UpdateState<LeafPath>>::process(state);
        return state;
    }
};

}
