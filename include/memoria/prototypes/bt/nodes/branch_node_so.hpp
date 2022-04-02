
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
#include <memoria/prototypes/bt/pkd_adapters/bt_pkd_adapter_generic.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/core/packed/tools/packed_dispatcher_res.hpp>

#include <memoria/core/packed/tools/packed_stateful_dispatcher.hpp>
#include <memoria/core/packed/tools/packed_stateful_dispatcher_res.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/checks.hpp>

namespace memoria {

template <typename CtrT, typename NodeType_>
class BranchNodeSO: public NodeCommonSO<CtrT, NodeType_> {
    using Base = NodeCommonSO<CtrT, NodeType_>;

    using Base::node_;
    using Base::ctr_;

    using Position = typename NodeType_::TypesT::Position;

public:

    using MyType = BranchNodeSO;

    using BranchNodeEntry = typename NodeType_::TypesT::BranchNodeEntry;
    using Value = typename NodeType_::TypesT::ID;

    using BranchSubstreamsStructList    = typename NodeType_::TypesT::BranchStreamsStructList;
    using LeafSubstreamsStructList      = typename NodeType_::TypesT::LeafStreamsStructList;

    template <typename PkdT>
    using PkdExtDataT = typename PkdT::ExtData;
    using BranchSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<BranchSubstreamsStructList>>;
    using BranchExtData = MakeTuple<BranchSubstreamExtensionsList>;

    using LeafSubstreamExtensionsList = boost::mp11::mp_transform<PkdExtDataT, Linearize<LeafSubstreamsStructList>>;
    using LeafExtData = MakeTuple<LeafSubstreamExtensionsList>;

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
            bt::FlattenBranchTree<BranchSubstreamsStructList>, NodeType_::Base::StreamsStart
    >::Type;

    using Dispatcher = PackedStatefulDispatcher<BranchExtData, StreamDispatcherStructList, NodeType_::StreamsStart>;
    using DispatcherWithResult = PackedStatefulDispatcherWithResult<BranchExtData, StreamDispatcherStructList, NodeType_::StreamsStart>;

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcherWithResult = typename DispatcherWithResult::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <int32_t StreamIdx>
    using StreamStartIdx = IntValue<
        list_tree::LeafCountInf<BranchSubstreamsStructList, IntList<StreamIdx>>
    >;

    template <int32_t StreamIdx>
    using StreamSize = IntValue<
            list_tree::LeafCountSup<BranchSubstreamsStructList, IntList<StreamIdx>> -
            list_tree::LeafCountInf<BranchSubstreamsStructList, IntList<StreamIdx>>
    >;



    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
            list_tree::LeafCountInf<BranchSubstreamsStructList, SubstreamsPath>,
            list_tree::LeafCountSup<BranchSubstreamsStructList, SubstreamsPath>
    >;

    template <typename SubstreamsPath>
    using SubstreamsDispatcherWithResult = SubrangeDispatcherWithResult<
            list_tree::LeafCountInf<BranchSubstreamsStructList, SubstreamsPath>,
            list_tree::LeafCountSup<BranchSubstreamsStructList, SubstreamsPath>
    >;

    template <int32_t SubstreamIdx>
    using LeafPathT = typename list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <int32_t SubstreamIdx>
    using BranchPathT = typename list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;


    static const int32_t Streams            = ListSize<BranchSubstreamsStructList>;

    static const int32_t Substreams         = Dispatcher::Size;

    static const int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static const int32_t ValuesBlockIdx     = SubstreamsEnd;

    static const PackedDataTypeSize SizeType = PackedListStructSizeType<Linearize<BranchSubstreamsStructList>>::Value;


    template <typename LeafPath>
    using BuildBranchPath = typename list_tree::BuildTreePath<
            BranchSubstreamsStructList,
            list_tree::LeafCountInf<LeafSubstreamsStructList, LeafPath, 2> -
                bt::FindLocalLeafOffsetV<
                    bt::FlattenLeafTree<LeafSubstreamsStructList>,
                    list_tree::LeafCount<LeafSubstreamsStructList, LeafPath>
                >::Value
    >::Type;

    template <typename BranchPath>
    using SubstreamByBranchPath = typename Dispatcher::template StreamTypeT<
        list_tree::LeafCount<BranchSubstreamsStructList, BranchPath>
    >::Type;

    template <typename LeafPath>
    using SubstreamByLeafPath = typename Dispatcher::template StreamTypeT<
        list_tree::LeafCount<BranchSubstreamsStructList, BuildBranchPath<LeafPath>>
    >::Type;


    template <typename LeafPath>
    static constexpr int32_t SubstreamIdxByLeafPath = list_tree::LeafCount<
        BranchSubstreamsStructList, BuildBranchPath<LeafPath>
    >;


    template <typename PkdStruct>
    using PkdStructToUpdateStateMap = HasType<typename PkdStruct::Type::SparseObject::UpdateState>;

    using UpdateState = AsTuple<
        MergeLists<
            typename Dispatcher::template ForAllStructs<PkdStructToUpdateStateMap>,
            PackedAllocatorUpdateState
        >
    >;


    template <typename LeafPath>
    static const int32_t translateLeafIndexToBranchIndex(int32_t leaf_index) {
        return bt::LeafToBranchIndexTranslator<LeafSubstreamsStructList, LeafPath, 0>::translate(leaf_index);
    }


    BranchNodeSO() : Base() {}
    BranchNodeSO(CtrT* ctr) : Base(ctr, nullptr) {}
    BranchNodeSO(CtrT* ctr, NodeType_* node) :
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

    template <typename LeafPath, typename ExtData>
    void set_ext_data(ExtData&& data) const
    {
        constexpr int32_t substream_idx = SubstreamIdxByLeafPath<LeafPath>;
        std::get<substream_idx>(ctr_->branch_node_ext_data()) = std::forward<ExtData>(data);
    }

    const PackedAllocator* allocator() const  {
        return node_->allocator();
    }

    PackedAllocator* allocator()  {
        return node_->allocator();
    }

    BranchExtData& state() const  {
        return ctr_->branch_node_ext_data();
    }

    void prepare()
    {
        return node_->prepare();
    }

    template <typename V>
    std::vector<V> values_as_vector(int32_t start, int32_t end) const
    {
        std::vector<V> vals;

        const auto* vv = node_->values();

        for (int32_t c = start; c < end; c++)
        {
            vals.emplace_back(vv[c]);
        }

        return std::move(vals);
    }

    template <typename V>
    std::vector<V> values_as_vector() const
    {
        auto size = this->size();
        return values_as_vector<V>(0, size);
    }



    template <typename OtherNode>
    void copy_node_data_to(OtherNode&& other) const
    {
        PackedAllocator* other_alloc = other.allocator();
        const PackedAllocator* my_alloc = this->allocator();

        for (int32_t c = 0; c <= ValuesBlockIdx; c++)
        {
            other_alloc->import_block(c, my_alloc, c);
        }
    }

    template <typename V>
    void forAllValues(int32_t start, int32_t end, const std::function<void (const V&)>& fn) const
    {
        const Value* v = node_->values();
        for (int32_t c = start; c < end; c++)
        {
            fn(v[c]);
        }
    }

    template <typename V>
    void forAllValues(int32_t start, const std::function<void (const V&)>& fn) const
    {
        auto end = size();
        return forAllValues(start, end, fn);
    }

    template <typename V>
    void forAllValues(const std::function<void (const V&)>& fn) const
    {
        return forAllValues(0, fn);
    }

    struct LayoutFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename StreamType>
        void stream(StreamType&, PackedAllocator* allocator)
        {
            if (allocator->is_empty(AllocatorIdx)) {
                allocator->template allocate_empty<
                    typename StreamType::PkdStructT
                >(AllocatorIdx);
            }
        }
    };


    void layout()
    {
        return Dispatcher::dispatchAllStatic(LayoutFn(), allocator());
    }

    struct MaxFn {
        template <int32_t Idx, typename StreamType>
        void stream(const StreamType& obj, BranchNodeEntry& accum)
        {
            bt::BTPkdStructAdaper<StreamType> adapter(obj);
            adapter.branch_max_entry(std::get<Idx>(accum));
        }
    };

    void max(BranchNodeEntry& entry) const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), MaxFn(), entry);
    }


    Value& value(int32_t idx)
    {
        //MEMORIA_ASSERT(idx, >=, 0);
        //MEMORIA_ASSERT(idx, <, size());

        return *(node_->values() + idx);
    }

    const Value& value(int32_t idx) const
    {
        //MEMORIA_ASSERT(idx, >=, 0);
        //MEMORIA_ASSERT(idx, <, size());

        return *(node_->values() + idx);
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

        std::unordered_map<Value, int32_t> map;

        auto node_size = size();
        for (int32_t c = 0; c < node_size; c++) {
            Value vv = value(c);
            auto ii = map.find(vv);
            if (ii == map.end()) {
                map[vv] = c;
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Duplicate child id found: {} :: {} :: {}", vv, ii->second, c).do_throw();
            }
        }
    }


    int32_t capacity() const
    {
        int32_t free_space  = node_->compute_streams_available_space();
        auto max_size = node_->max_tree_size1(free_space, -1ull);

        auto size = this->size();
        int32_t cap = max_size - size;

        return cap >= 0 ? cap : 0;
    }


    struct SizeFn {
        int32_t size_ = 0;

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree&& tree)
        {
            size_ = tree ? tree.size() : 0;
        }
    };

    int32_t size() const
    {
        SizeFn fn;
        Dispatcher(state()).dispatch(0, allocator(), fn);
        return fn.size_;
    }

    int32_t size(int32_t stream) const
    {
        SizeFn fn;
        Dispatcher(state()).dispatch(stream, allocator(), fn);
        return fn.size_;
    }

    struct SizesFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree>
        void stream(Tree&& tree, Position& pos)
        {
            pos.value(StreamIdx) = tree.size();
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
        void stream(Tree&& tree, Position& sizes)
        {
            sizes[ListIdx] = tree ? tree.sum(0) : 0;
        }
    };

    Position size_sums() const
    {
        Position sums;
        processStreamsStart(SizeSumsFn(), sums);
        return sums;
    }


    struct PrepareInsertFn {
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <int32_t Idx, typename StreamType>
        void stream(StreamType&& obj, int32_t idx, const BranchNodeEntry& keys, UpdateState& update_state)
        {
            if (is_success(status)) {
                status = obj.prepare_insert(idx, 1, std::get<Idx>(update_state), [&](size_t column, size_t)  {
                    return std::get<Idx>(keys)[column];
                });
            }
        }
    };

    struct CommitInsertFn {
        template <int32_t Idx, typename StreamType>
        void stream(StreamType&& obj, int32_t idx, const BranchNodeEntry& keys, UpdateState& update_state)
        {
            return obj.commit_insert(idx, 1, std::get<Idx>(update_state), [&](size_t column, size_t) {
                return std::get<Idx>(keys)[column];
            });
        }
    };


    void commit_insert(int32_t idx, const BranchNodeEntry& keys, const Value& value, UpdateState& update_state)
    {
        auto size = this->size();

        CommitInsertFn commit_insert_fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), commit_insert_fn, idx, keys, update_state);

        int32_t requested_block_size = (size + 1) * sizeof(Value);

        allocator()->resize_block(ValuesBlockIdx, requested_block_size);

        Value* values = node_->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value;
    }


    PkdUpdateStatus insert(int32_t idx, const BranchNodeEntry& keys, const Value& value)
    {
        auto size = this->size();

        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size);

        UpdateState update_state = make_update_state();

        PrepareInsertFn prepare_insert_fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), prepare_insert_fn, idx, keys, update_state);

        if (is_success(prepare_insert_fn.status))
        {
            // FIXME!
            // calculate child_id size here!
            commit_insert(idx, keys, value, update_state);
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }


    void insertValuesSpace(int32_t old_size, int32_t room_start, int32_t room_length)
    {
        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_start, <=, old_size);

        int32_t requested_block_size = (old_size + room_length) * sizeof(Value);

        allocator()->resize_block(ValuesBlockIdx, requested_block_size);

        Value* values = node_->values();

        CopyBuffer(values + room_start, values + room_start + room_length, old_size - room_start);

        for (int32_t c = room_start; c < room_start + room_length; c++)
        {
            values[c] = Value();
        }
    }


    void commit_insert_values(int32_t old_size, int32_t idx, int32_t length, PackedAllocatorUpdateState&, std::function<Value()> provider)
    {
        insertValuesSpace(old_size, idx, length);

        Value* values = node_->values();

        for (int32_t c = idx; c < idx + length; c++)
        {
            values[c] = provider();
        }
    }


    struct PrepareRemoveSpaceFn {
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <int32_t Idx, typename Tree>
        void stream(Tree&& tree, size_t room_start, size_t room_end, UpdateState& update_state)
        {
            if (status == PkdUpdateStatus::SUCCESS) {
                status = tree.prepare_remove(room_start, room_end, std::get<Idx>(update_state));
            }
        }
    };


    PkdUpdateStatus prepare_remove(size_t room_start, size_t room_end, UpdateState& update_state) const
    {
        if (SizeType == PackedDataTypeSize::VARIABLE) {
            PrepareRemoveSpaceFn remove_fn;
            Dispatcher(state()).dispatchNotEmpty(allocator(), remove_fn, room_start, room_end, update_state);
            return remove_fn.status;
        }
        else {
            return PkdUpdateStatus::SUCCESS;
        }
    }


    struct CommitRemoveSpaceFn {
        template <int32_t Idx, typename Tree>
        void stream(Tree&& tree, size_t room_start, size_t room_end, UpdateState& update_state)
        {
            return tree.commit_remove(room_start, room_end, std::get<Idx>(update_state));
        }
    };


    void commit_remove(size_t start, size_t end, UpdateState& update_state)
    {
        auto old_size = this->size();

        CommitRemoveSpaceFn remove_fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), remove_fn, start, end, update_state);

        Value* values = node_->values();
        CopyBuffer(values + end, values + start, old_size - end);

        // FIXME: We don't need reindexing here!
        reindex();
        MEMORIA_ASSERT(old_size, >=, end - start);

        size_t requested_block_size = (old_size - (end - start)) * sizeof(Value);
        allocator()->resize_block(ValuesBlockIdx, requested_block_size);
    }



    PkdUpdateStatus try_remove_entries(size_t start, size_t end)
    {
        UpdateState update_state = make_update_state();
        PkdUpdateStatus status = prepare_remove(start, end, update_state);
        if (is_success(status)) {
            commit_remove(start, end, update_state);
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }

    void remove_entries(size_t start, size_t end) {
        UpdateState update_state = make_update_state();
        (void)commit_remove(start, end, update_state);
    }


    struct ReindexFn {
        template <typename Tree>
        void stream(Tree& tree)
        {
            return tree.reindex();
        }
    };

    // Not used by BTree directly
    void reindex()
    {
        ReindexFn fn;
        return Dispatcher(state()).dispatchNotEmpty(allocator(), fn);
    }



    bool shouldBeMergedWithSiblings() const  {
        return node_->shouldBeMergedWithSiblings();
    }

//    struct CanMergeWithFn {
//        int32_t mem_used_ = 0;

//        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
//        void stream(Tree&& tree, OtherNodeT&& other)
//        {
//            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
//            if (tree)
//            {
//                if (other.allocator()->is_empty(AllocatorIdx))
//                {
//                    mem_used_ += tree.data()->block_size();
//                }
//                else {
//                    const PkdTree* other_tree = other.allocator()->template get<PkdTree>(AllocatorIdx);
//                    mem_used_ += tree.data()->block_size_for(other_tree);
//                }
//            }
//            else {
//                if (!other.allocator()->is_empty(AllocatorIdx))
//                {
//                    int32_t element_size = other.allocator()->element_size(AllocatorIdx);
//                    mem_used_ += element_size;
//                }
//            }
//        }
//    };

//    template <typename OtherNodeT>
//    BoolResult canBeMergedWith(OtherNodeT&& other) const
//    {
//        CanMergeWithFn fn;
//        MEMORIA_TRY_VOID(DispatcherWithResult(state()).dispatchAll(allocator(), fn, std::forward<OtherNodeT>(other)));

//        int32_t client_area = other.allocator()->client_area();

//        int32_t my_data_size    = allocator()->element_size(ValuesBlockIdx);
//        int32_t other_data_size = other.allocator()->element_size(ValuesBlockIdx);

//        fn.mem_used_ += my_data_size;
//        fn.mem_used_ += other_data_size;

//        // FIXME +10 is an extra safety gap
//        return BoolResult::of(client_area >= fn.mem_used_); //+ 10
//    }

    struct CommitMergeWithFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        void stream(const Tree& tree, OtherNodeT&& other, UpdateState& update_state)
        {
            int32_t size = tree.size();

            if (size > 0)
            {
                Dispatcher other_disp = other.dispatcher();
                Tree other_tree = other_disp.template get<Idx>(other.allocator());
                return tree.commit_merge_with(other_tree, std::get<Idx>(update_state));
            }
        }
    };

    template <typename OtherNodeT>
    void commit_merge_with(OtherNodeT&& other, UpdateState& update_state) const
    {
        auto other_size = other.size();
        auto my_size = size();

        CommitMergeWithFn fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other), update_state);

        int32_t other_values_block_size          = other.allocator()->element_size(ValuesBlockIdx);
        int32_t required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            other.allocator()->resize_block(other.node()->values(), required_other_values_block_size);
        }

        CopyBuffer(node_->values(), other.node()->values() + other_size, my_size);
    }

    struct PrepareMergeWithFn {
        PkdUpdateStatus status_{PkdUpdateStatus::SUCCESS};

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT, typename UpdateState>
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
        if (fn.status_ == PkdUpdateStatus::SUCCESS)
        {
            // check children pointers block capacity here
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }

    template <typename OtherNodeT>
    PkdUpdateStatus merge_with(OtherNodeT&& other) const
    {
        auto update_state = make_update_state();
        if (is_success(prepare_merge_with(std::forward<OtherNodeT>(other), update_state))) {
            commit_merge_with(std::forward<OtherNodeT>(other), update_state);
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }

    struct SplitToFn {
        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        void stream(Tree& tree, OtherNodeT&& other, int32_t idx)
        {
            int32_t size = tree.size();
            if (size > 0)
            {
                Dispatcher other_disp = other.dispatcher();

                Tree other_tree = other_disp.template get<Idx>(other.allocator());
                if (!other_tree.data())
                {
                    other_tree = other_disp.template allocate_empty<Idx>(other.allocator()).get_or_throw();
                }

                return tree.split_to(other_tree, idx);
            }
        }
    };


    template <typename OtherNodeT>
    void split_to(OtherNodeT&& other, int32_t split_idx)
    {
        auto size = this->size();
        int32_t remainder = size - split_idx;

        MEMORIA_ASSERT(split_idx, <=, size);

        SplitToFn fn;
        Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other), split_idx);
        other.allocator()->template allocate_array_by_size<Value>(ValuesBlockIdx, remainder);

        Value* other_values = other.node()->values();
        Value* my_values    = node_->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);
    }



    struct KeysAtFn {
        template <int32_t Idx, typename Tree>
        void stream(const Tree& tree, int32_t idx, BranchNodeEntry* acc)
        {
            bt::BTPkdStructAdaper<Tree> adapter(tree);
            adapter.assign_to(std::get<Idx>(*acc), idx);
        }
    };

    BranchNodeEntry keysAt(int32_t idx) const
    {
        BranchNodeEntry acc;
        Dispatcher(state()).dispatchNotEmpty(allocator(), KeysAtFn(), idx, &acc);
        return acc;
    }


    struct SumsFn {
        template <int32_t Idx, typename StreamType>
        void stream(const StreamType& obj, int32_t start, int32_t end, BranchNodeEntry& accum)
        {
            obj.sums(start, end, std::get<Idx>(accum));
        }

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamType>
        void stream(const StreamType& obj, const Position& start, const Position& end, BranchNodeEntry& accum)
        {
            obj.sums(start[StreamIdx], end[StreamIdx], std::get<AllocatorIdx - SubstreamsStart>(accum));
        }

        template <int32_t Idx, typename StreamType>
        void stream(const StreamType& obj, BranchNodeEntry& accum)
        {
            obj.sums(std::get<Idx>(accum));
        }

        template <typename StreamType>
        void stream(const StreamType& obj, int32_t block, int32_t start, int32_t end, int64_t& accum)
        {
            accum += obj.sum(block, start, end);
        }

        template <typename StreamType>
        void stream(const StreamType& obj, int32_t block, int64_t& accum)
        {
            accum += obj.sum(block);
        }
    };

    void sums(int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), start, end, sums);
    }

    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processSubstreamGroups(SumsFn(), start, end, sums);
    }

    VoidResult sums(BranchNodeEntry& sums) const
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), sums);
    }

    BranchNodeEntry sums() const
    {
        BranchNodeEntry sums;
        Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), sums);
        return sums;
    }

    void sum(int32_t stream, int32_t block_num, int32_t start, int32_t end, int64_t& accum) const
    {
        Dispatcher(state()).dispatch(stream, allocator(), SumsFn(), block_num, start, end, accum);
    }

    void sum(int32_t stream, int32_t block_num, int64_t& accum) const
    {
        Dispatcher(state()).dispatch(stream, allocator(), SumsFn(), block_num, accum);
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
        return processStream<BranchPath>(SumsFn(), index, start, end, accum);
    }


    struct CommitUpdateFn {
        template <int32_t Idx, typename StreamType, typename UpdateState>
        void stream(StreamType&& tree, int32_t idx, const BranchNodeEntry& accum, UpdateState& update_state)
        {
            return tree.commit_update(idx, 1, std::get<Idx>(update_state), [&](psize_t col, psize_t)  {
                return std::get<Idx>(accum)[col];
            });
        }
    };

    struct PrepareUpdateFn {
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <int32_t Idx, typename StreamType, typename UpdateState>
        void stream(StreamType&& tree, int32_t idx, const BranchNodeEntry& accum, UpdateState& update_state)
        {
            if (is_success(status)) {
                status = tree.prepare_update(idx, 1, std::get<Idx>(update_state), [&](psize_t col, psize_t)  {
                    return std::get<Idx>(accum)[col];
                });
            }
        }
    };


    void commit_update(int32_t idx, const BranchNodeEntry& keys, UpdateState& update_state) {
        CommitUpdateFn fn2;
        Dispatcher(state()).dispatchNotEmpty(allocator(), fn2, idx, keys, update_state);
    }

    PkdUpdateStatus update(int32_t idx, const BranchNodeEntry& keys)
    {
        UpdateState update_state = make_update_state();

        PrepareUpdateFn fn1;
        if (SizeType == PackedDataTypeSize::VARIABLE) {
            Dispatcher(state()).dispatchNotEmpty(allocator(), fn1, idx, keys, update_state);
        }

        if (is_success(fn1.status)) {
            commit_update(idx, keys, update_state);
            return PkdUpdateStatus::SUCCESS;
        }
        else {
            return PkdUpdateStatus::FAILURE;
        }
    }


    bool checkCapacities(const Position& sizes) const
    {
        auto val = capacity();
        return val >= sizes.get();
    }


    struct GenerateDataEventsFn {
        template <int32_t Idx, typename Tree>
        auto stream(Tree&& tree, IBlockDataEventHandler* handler)
        {
            return tree.generateDataEvents(handler);
        }
    };

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        node_->template generateDataEvents<RootMetadataList>(handler);
        Dispatcher(state()).dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);

        auto size = this->size();

        handler->startGroup("TREE_VALUES", size);

        for (int32_t idx = 0; idx < size; idx++) {
            ValueHelper<Value>::setup(handler, "CHILD_ID", value(idx));
        }

        handler->endGroup();
    }

    void init_root_metadata()  {
        return node_->template init_root_metadata<RootMetadataList>();
    }

    int32_t find_child_idx(const Value& child_id) const
    {
        int32_t idx = -1;

        const Value* values = node_->values();
        auto size = node_->size();

        for (int32_t c = 0; c < size; c++) {
            if (child_id == values[c]) {
                return c;
            }
        }

        return idx;
    }



    /******************************************************************/

    Dispatcher dispatcher() const  {
        return Dispatcher(state());
    }

    template <typename Fn, typename... Args>
    void dispatchAll(Fn&& fn, Args&&... args) const
    {
        Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamPath>
    auto substream()
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state()).template get<SubstreamIdx>(allocator());
    }


    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }

    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args)
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }


    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state()).dispatch(
                stream,
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto process(int32_t stream, Fn&& fn, Args&&... args)
    {
        return Dispatcher(state()).dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args)
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    auto processAllVoidRes(Fn&& fn, Args&&... args)
    {
        return DispatcherWithResult(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    VoidResult processSubstreamsVoidRes(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAllVoidRes(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    auto processSubstreams(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    VoidResult processSubstreamsVoidRes(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAllVoidRes(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const int32_t StreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const int32_t StreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto processStreamByIdx(Fn&& fn, Args&&... args) const
    {
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t StreamIdx, typename Fn, typename... Args>

    auto processStreamByIdx(Fn&& fn, Args&&... args)
    {
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args)
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
                state(),
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

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
        using GroupsList = bt::BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    {
        using Subset = bt::StreamsStartSubset<BranchSubstreamsStructList>;
        using SD = typename Dispatcher::template SubsetDispatcher<Subset>;
        return SD(state()).dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const
    {
        using Subset = bt::StreamsStartSubset<BranchSubstreamsStructList>;

        using SD = typename Dispatcher::template SubsetDispatcher<Subset>;

        return SD(state()).dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }


    static UpdateState make_update_state() {
        UpdateState state;
        bt::UpdateStateInitializer<UpdateState>::process(state);
        return state;
    }
};

}
