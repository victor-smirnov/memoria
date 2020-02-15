
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
#include <memoria/prototypes/bt/pkd_adapters/bt_pkd_adapter_generic.hpp>

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

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;

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

    template <int32_t SubstreamIdx>
    using LeafPathT = typename list_tree::BuildTreePath<LeafSubstreamsStructList, SubstreamIdx>::Type;

    template <int32_t SubstreamIdx>
    using BranchPathT = typename list_tree::BuildTreePath<BranchSubstreamsStructList, SubstreamIdx>::Type;


    static const int32_t Streams            = ListSize<BranchSubstreamsStructList>;

    static const int32_t Substreams         = Dispatcher::Size;

    static const int32_t SubstreamsStart    = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd      = Dispatcher::AllocatorIdxEnd;

    static const int32_t ValuesBlockIdx     = SubstreamsEnd;


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


    template <typename LeafPath>
    static const int32_t translateLeafIndexToBranchIndex(int32_t leaf_index)
    {
        return bt::LeafToBranchIndexTranslator<LeafSubstreamsStructList, LeafPath, 0>::translate(leaf_index);
    }


    BranchNodeSO(): Base() {}
    BranchNodeSO(CtrT* ctr): Base(ctr, nullptr) {}
    BranchNodeSO(CtrT* ctr, NodeType_* node):
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

    template <typename LeafPath, typename ExtData>
    void set_ext_data(ExtData&& data) const
    {
        constexpr int32_t substream_idx = SubstreamIdxByLeafPath<LeafPath>;
        std::get<substream_idx>(ctr_->branch_node_ext_data()) = std::forward<ExtData>(data);
    }

    const PackedAllocator* allocator() const noexcept {
        return node_->allocator();
    }

    PackedAllocator* allocator() noexcept {
        return node_->allocator();
    }

    const BranchExtData& state() const noexcept {
        return ctr_->branch_node_ext_data();
    }

    VoidResult prepare() noexcept
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

        return vals;
    }

    template <typename V>
    std::vector<V> values_as_vector() const
    {
        return values_as_vector<V>(0, size());
    }


    template <typename V>
    VoidResult forAllValues(int32_t start, int32_t end, std::function<VoidResult (const V&)> fn) const noexcept
    {
        const Value* v = node_->values();
        for (int32_t c = start; c < end; c++)
        {
            MEMORIA_TRY_VOID(fn(v[c]));
        }

        return VoidResult::of();
    }

    template <typename V>
    VoidResult forAllValues(int32_t start, std::function<VoidResult (const V&)> fn) const noexcept
    {
        auto end = size();
        return forAllValues(start, end, fn);
    }

    template <typename V>
    VoidResult forAllValues(std::function<VoidResult (const V&)> fn) const noexcept
    {
        return forAllValues(0, fn);
    }

    struct LayoutFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename StreamType>
        VoidResult stream(StreamType&, PackedAllocator* allocator, uint64_t active_streams) noexcept
        {
            if (active_streams & (1 << Idx))
            {
                if (allocator->is_empty(AllocatorIdx))
                {
                    MEMORIA_TRY_VOID(
                        allocator->template allocateEmpty<
                            typename StreamType::PkdStructT
                        >(AllocatorIdx)
                    );
                }
            }

            return VoidResult::of();
        }
    };


    VoidResult layout(uint64_t active_streams) noexcept
    {
        return Dispatcher(state()).dispatchAllStatic(LayoutFn(), allocator(), active_streams);
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
        Dispatcher(state()).dispatchNotEmpty(allocator(), MaxFn(), entry).get_or_throw();
    }

    template <typename BranchNodeEntry>
    VoidResult updateUp(int32_t idx, const BranchNodeEntry& keys) noexcept {
        return node_->updateUp(idx, keys);
    }


    template <typename BranchNodeEntry, typename Value>
    VoidResult insert(int32_t idx, const BranchNodeEntry& keys, const Value& value) noexcept
    {
        return node_->insert(idx, keys, value);
    }

    Value& value(int32_t idx)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, size());

        return *(node_->values() + idx);
    }

    const Value& value(int32_t idx) const
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, size());

        return *(node_->values() + idx);
    }


    struct CheckFn {
        template <typename Tree>
        VoidResult stream(Tree&& tree) noexcept
        {
            return tree.check();
        }
    };

    VoidResult check() const noexcept
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), CheckFn());
    }



    uint64_t active_streams() const {
        return node_->active_streams();
    }



    int32_t capacity(uint64_t active_streams) const
    {
        int32_t free_space  = node_->free_space(node_->header().memory_block_size(), node_->is_root());
        int32_t max_size    = node_->max_tree_size1(free_space, active_streams);
        int32_t cap         = max_size - size();

        return cap >= 0 ? cap : 0;
    }

    int32_t capacity() const
    {
        return capacity(active_streams());
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
        //FIXME: use correct procedure to get number of children
        Dispatcher(state()).dispatch(0, allocator(), fn).get_or_throw();
        return fn.size_;
    }

    int32_t size(int32_t stream) const
    {
        SizeFn fn;
        Dispatcher(state()).dispatch(stream, allocator(), fn).get_or_throw();
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
        processSubstreamGroups(SizesFn(), pos).get_or_throw();
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
        processStreamsStart(SizeSumsFn(), sums).get_or_throw();
        return sums;
    }





    struct InsertFn {
        template <int32_t Idx, typename StreamType>
        VoidResult stream(StreamType&& obj, int32_t idx, const BranchNodeEntry& keys) noexcept
        {
            return obj.insert_entries(idx, 1, [&](size_t column, size_t) noexcept {
                return std::get<Idx>(keys)[column];
            });
        }
    };

    VoidResult insert(int32_t idx, const BranchNodeEntry& keys, const Value& value) noexcept
    {
        int32_t size = this->size();

        MEMORIA_ASSERT_RTN(idx, >=, 0);
        MEMORIA_ASSERT_RTN(idx, <=, size);

        InsertFn insert_fn;
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchNotEmpty(allocator(), insert_fn, idx, keys));

        int32_t requested_block_size = (size + 1) * sizeof(Value);

        MEMORIA_TRY_VOID(allocator()->resizeBlock(ValuesBlockIdx, requested_block_size));

        Value* values = node_->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value;

        return VoidResult::of();
    }




    VoidResult insertValuesSpace(int32_t old_size, int32_t room_start, int32_t room_length) noexcept
    {
        MEMORIA_ASSERT_RTN(room_start, >=, 0);
        MEMORIA_ASSERT_RTN(room_start, <=, old_size);

        int32_t requested_block_size = (old_size + room_length) * sizeof(Value);

        MEMORIA_TRY_VOID(allocator()->resizeBlock(ValuesBlockIdx, requested_block_size));

        Value* values = node_->values();

        CopyBuffer(values + room_start, values + room_start + room_length, old_size - room_start);

        for (int32_t c = room_start; c < room_start + room_length; c++)
        {
            values[c] = Value();
        }

        return VoidResult::of();
    }

    VoidResult insertValues(int32_t old_size, int32_t idx, int32_t length, std::function<Value()> provider) noexcept
    {
        MEMORIA_TRY_VOID(insertValuesSpace(old_size, idx, length));

        Value* values = node_->values();

        for (int32_t c = idx; c < idx + length; c++)
        {
            values[c] = provider();
        }

        return VoidResult::of();
    }




    struct RemoveSpaceFn {
        template <typename Tree>
        VoidResult stream(Tree&& tree, int32_t room_start, int32_t room_end) noexcept
        {
            return tree.removeSpace(room_start, room_end);
        }
    };

    VoidResult removeSpace(const Position& from_pos, const Position& end_pos) noexcept
    {
        return this->removeSpace(from_pos.get(), end_pos.get());
    }

    VoidResult removeSpaceAcc(int32_t room_start, int32_t room_end) noexcept
    {
        return removeSpace(room_start, room_end);
    }

    struct ReindexFn {
        template <typename Tree>
        VoidResult stream(Tree& tree) noexcept
        {
            return tree.reindex();
        }
    };

    // Not used by BTree directly
    VoidResult reindex() noexcept
    {
        ReindexFn fn;
        return Dispatcher(state()).dispatchNotEmpty(allocator(), fn);
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) noexcept
    {
        int32_t old_size = this->size();

        RemoveSpaceFn remove_fn;
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchNotEmpty(allocator(), remove_fn, room_start, room_end));

        Value* values = node_->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        MEMORIA_TRY_VOID(reindex());
        MEMORIA_ASSERT(old_size, >=, room_end - room_start);

        int32_t requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);

        MEMORIA_TRY_VOID(allocator()->resizeBlock(values, requested_block_size));

        return VoidResult::of();
    }

    VoidResult removeSpace(int32_t stream, int32_t room_start, int32_t room_end) noexcept
    {
        return removeSpace(room_start, room_end);
    }



    bool shouldBeMergedWithSiblings() const {
        return node_->shouldBeMergedWithSiblings();
    }

    struct CanMergeWithFn {
        int32_t mem_used_ = 0;

        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        void stream(Tree&& tree, OtherNodeT&& other)
        {
            using PkdTree = typename std::decay_t<Tree>::PkdStructT;
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

        int32_t my_data_size    = allocator()->element_size(ValuesBlockIdx);
        int32_t other_data_size = other.allocator()->element_size(ValuesBlockIdx);

        fn.mem_used_ += my_data_size;
        fn.mem_used_ += other_data_size;

        // FIXME +10 is an extra safety gap
        return BoolResult::of(client_area >= fn.mem_used_ + 10);
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
        int32_t other_size  = other.size();
        int32_t my_size     = size();

        MergeWithFn fn;
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other)));

        int32_t other_values_block_size          = other.allocator()->element_size(ValuesBlockIdx);
        int32_t required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            MEMORIA_TRY_VOID(other.allocator()->resizeBlock(other.node()->values(), required_other_values_block_size));
        }

        CopyBuffer(node_->values(), other.node()->values() + other_size, my_size);

        return VoidResult::of();
    }



    struct SplitToFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Tree, typename OtherNodeT>
        VoidResult stream(Tree& tree, OtherNodeT& other, int32_t idx) noexcept
        {
            int32_t size = tree.size();
            if (size > 0)
            {
                Dispatcher other_disp = other.dispatcher();

                Tree other_tree = other_disp.template get<Idx>(other.allocator());
                if (!other_tree.data())
                {
                    MEMORIA_TRY(other_tree_tmp, other_disp.template allocateEmpty<Idx>(other.allocator()));
                    other_tree = std::move(other_tree_tmp);
                }

                return tree.splitTo(other_tree, idx);
            }

            return VoidResult::of();
        }
    };


    template <typename OtherNodeT>
    VoidResult splitTo(OtherNodeT&& other, int32_t split_idx)
    {
        int32_t size        = this->size();
        int32_t remainder   = size - split_idx;

        MEMORIA_ASSERT(split_idx, <=, size);

        SplitToFn fn;
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchNotEmpty(allocator(), fn, std::forward<OtherNodeT>(other), split_idx));


        MEMORIA_TRY_VOID(other.allocator()->template allocateArrayBySize<Value>(ValuesBlockIdx, remainder));

        Value* other_values = other.node()->values();
        Value* my_values    = node_->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);

        return VoidResult::of();
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

        Dispatcher(state()).dispatchNotEmpty(allocator(), KeysAtFn(), idx, &acc).get_or_throw();

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
    };

    void sums(int32_t start, int32_t end, BranchNodeEntry& sums) const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), start, end, sums).get_or_throw();
    }

    void sums(const Position& start, const Position& end, BranchNodeEntry& sums) const
    {
        processSubstreamGroups(SumsFn(), start, end, sums);
    }

    void sums(BranchNodeEntry& sums) const
    {
        Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), sums).get_or_throw();
    }

    BranchNodeEntry sums() const
    {
        BranchNodeEntry sums;
        Dispatcher(state()).dispatchNotEmpty(allocator(), SumsFn(), sums).get_or_throw();
        return sums;
    }

    void sum(int32_t stream, int32_t block_num, int32_t start, int32_t end, int64_t& accum) const
    {
        Dispatcher(state()).dispatch(stream, allocator(), SumsFn(), block_num, start, end, accum).get_or_throw();
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


    struct UpdateUpFn {
        template <int32_t Idx, typename StreamType>
        VoidResult stream(StreamType&& tree, int32_t idx, const BranchNodeEntry& accum) noexcept
        {
            return tree.update_entries(idx, 1, [&](psize_t col, psize_t) noexcept {
                return std::get<Idx>(accum)[col];
            });
        }
    };


    VoidResult updateUp(int32_t idx, const BranchNodeEntry& keys)
    {
        UpdateUpFn fn;
        return Dispatcher(state()).dispatchNotEmpty(allocator(), fn, idx, keys);
    }



    bool checkCapacities(const Position& sizes) const
    {
        return capacity() >= sizes.get();
    }


    struct GenerateDataEventsFn {
        template <int32_t Idx, typename Tree>
        auto stream(Tree&& tree, IBlockDataEventHandler* handler)
        {
            return tree.generateDataEvents(handler);
        }
    };

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        MEMORIA_TRY_VOID(node_->template generateDataEvents<RootMetadataList>(handler));
        MEMORIA_TRY_VOID(Dispatcher(state()).dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler));

        handler->startGroup("TREE_VALUES", size());

        for (int32_t idx = 0; idx < size(); idx++)
        {
            ValueHelper<Value>::setup(handler, "CHILD_ID", value(idx));
        }

        handler->endGroup();

        return VoidResult::of();
    }

    VoidResult init_root_metadata() noexcept {
        return node_->template init_root_metadata<RootMetadataList>();
    }

    int32_t find_child_idx(const Value& child_id) const noexcept
    {
        int32_t idx = -1;

        const Value* values = node_->values();
        int32_t size = node_->size();

        for (int32_t c = 0; c < size; c++) {
            if (child_id == values[c]) {
                return c;
            }
        }

        return idx;
    }



    /******************************************************************/

    Dispatcher dispatcher() const noexcept {
        return Dispatcher(state());
    }

    template <typename Fn, typename... Args>
    void dispatchAll(Fn&& fn, Args&&... args) const noexcept
    {
        Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamPath>
    auto substream()
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }

    template <typename SubstreamPath>
    auto substream() const
    {
        const int32_t SubstreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }


    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
    }

    template <typename Fn, typename... Args>
    auto processNotEmpty(Fn&& fn, Args&&... args) noexcept
    {
        return Dispatcher(state()).dispatchNotEmpty(allocator(), std::forward<Fn>(fn), args...);
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
        return Dispatcher(state()).dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) noexcept
    {
        return Dispatcher(state()).dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
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
        return SubstreamsDispatcher<SubstreamsPath>(state())
                .dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const noexcept
    {
        const int32_t StreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) noexcept
    {
        const int32_t StreamIdx = list_tree::LeafCount<BranchSubstreamsStructList, SubstreamPath>;
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto processStreamByIdx(Fn&& fn, Args&&... args) const noexcept
    {
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t StreamIdx, typename Fn, typename... Args>

    auto processStreamByIdx(Fn&& fn, Args&&... args) noexcept
    {
        return Dispatcher(state())
                .template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Fn, typename... Args>
    auto processSubstreamGroups(Fn&& fn, Args&&... args) noexcept
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
    auto processSubstreamGroups(Fn&& fn, Args&&... args) const noexcept
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
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args) noexcept
    {
        using GroupsList = bt::BuildTopLevelLeafSubsets<BranchSubstreamsStructList>;

        return bt::GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) noexcept
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
    auto processStreamsStart(Fn&& fn, Args&&... args) const noexcept
    {
        using Subset = bt::StreamsStartSubset<BranchSubstreamsStructList>;

        using SD = typename Dispatcher::template SubsetDispatcher<Subset>;

        return SD(state()).dispatchAll(
                allocator(),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};

}
