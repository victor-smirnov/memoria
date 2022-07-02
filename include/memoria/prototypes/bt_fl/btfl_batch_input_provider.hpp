
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/core/tools/assert.hpp>

#include <memory>

namespace memoria {
namespace btfl {

namespace detail {

    enum class StreamSelectorType {DATA, STRUCTURE};

    template <StreamSelectorType TT, int32_t DataStreams> struct PrepareStreamSelector;
    template <StreamSelectorType TT, int32_t DataStreams> struct CommitStreamSelector;

    template <int32_t DataStreams>
    struct CommitStreamSelector<StreamSelectorType::DATA, DataStreams>
    {
        template <
                size_t StreamIdx,
                size_t SubstreamIdx,
                typename StreamObj, typename Position, typename UpdateStatus, typename CtrInputBuffer
        >
        static void io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,                
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateStatus& update_status
        )
        {
            static_assert(StreamIdx < DataStreams, "");

            stream.commit_insert_io_substream(
                    at[StreamIdx],
                    get_ctr_batch_input_substream<StreamIdx, SubstreamIdx>(input_buffer),
                    starts[StreamIdx],
                    sizes[StreamIdx],
                    update_status
            );
        }
    };

    template <int32_t DataStreams>
    struct PrepareStreamSelector<StreamSelectorType::DATA, DataStreams>
    {
        template <
                size_t StreamIdx,
                size_t SubstreamIdx,
                typename StreamObj, typename Position, typename UpdateStatus, typename CtrInputBuffer
        >
        static PkdUpdateStatus io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateStatus& update_status
        )
        {
            static_assert(StreamIdx < DataStreams, "");

            PkdUpdateStatus status = stream.prepare_insert_io_substream(
                            at[StreamIdx],
                            get_ctr_batch_input_substream<StreamIdx, SubstreamIdx>(input_buffer),
                            starts[StreamIdx],
                            sizes[StreamIdx],
                            update_status
            );
            return status;
        }
    };

    template <int32_t DataStreams>
    struct CommitStreamSelector<StreamSelectorType::STRUCTURE, DataStreams>
    {
        template <
                size_t StreamIdx,
                size_t SubstreamIdx,
                typename StreamObj, typename Position, typename UpdateStatus, typename CtrInputBuffer
        >
        static void io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateStatus& update_status
        )
        {
            static_assert(StreamIdx == DataStreams, "");
            stream.commit_insert_io_substream(
                    at[StreamIdx],
                    input_buffer.symbols(),
                    starts[StreamIdx],
                    sizes[StreamIdx],
                    update_status
            );
        }
    };

    template <int32_t DataStreams>
    struct PrepareStreamSelector<StreamSelectorType::STRUCTURE, DataStreams>
    {
        template <
                size_t StreamIdx,
                size_t SubstreamIdx,
                typename StreamObj, typename Position, typename UpdateStatus, typename CtrInputBuffer
        >
        static PkdUpdateStatus io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateStatus& update_status
        )
        {
            static_assert(StreamIdx == DataStreams, "");

            PkdUpdateStatus status = stream.prepare_insert_io_substream(
                        at[StreamIdx],
                        input_buffer.symbols(),
                        starts[StreamIdx],
                        sizes[StreamIdx],
                        update_status
            );
            return status;
        }
    };
}




template <typename CtrT>
class AbstractCtrBatchInputProviderBase: public bt::CtrBatchInputProviderBase<CtrT> {

protected:
    static const int32_t Streams                = CtrT::Types::Streams;
    static const int32_t DataStreams            = CtrT::Types::DataStreams;
    static const int32_t StructureStreamIdx     = CtrT::Types::StructureStreamIdx;


public:
    using MyType = AbstractCtrBatchInputProviderBase<CtrT>;

    using TreeNodePtr           = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT              = typename CtrT::Types::CtrSizeT;
    using Position              = typename CtrT::Types::Position;
    using DataPositions         = core::StaticVector<uint64_t, DataStreams>;
    using CtrDataPositionsT     = core::StaticVector<int64_t, DataStreams>;

    using CtrInputBuffer        = typename CtrT::Types::CtrInputBuffer;


    using NodePair = std::pair<TreeNodePtr, TreeNodePtr>;

protected:

    DataPositions start_;
    DataPositions size_;

    bool finished_{false};

    CtrT& ctr_;

    CtrSizeT orphan_splits_{};

    //FIXME: remove it
    NodePair split_watcher_;

    CtrSizeT total_symbols_{};

    CtrDataPositionsT totals_{};

    CtrBatchInputFn<CtrInputBuffer> producer_{};
    CtrInputBuffer& input_buffer_;

public:

    AbstractCtrBatchInputProviderBase(
            CtrT& ctr,
            CtrBatchInputFn<CtrInputBuffer> producer,
            CtrInputBuffer& input_buffer
    ):
        ctr_(ctr), producer_(producer), input_buffer_(input_buffer)
    {}


    CtrT& ctr() {return ctr_;}
    const CtrT& ctr() const {return ctr_;}

    NodePair& split_watcher() {
        return split_watcher_;
    }

    const NodePair& split_watcher() const {
        return split_watcher_;
    }

    CtrSizeT orphan_splits() const {
        return orphan_splits_;
    }

    const CtrDataPositionsT& totals() const {
        return totals_;
    }

    virtual bool hasData()
    {
        bool buffer_has_data = start_.sum() < size_.sum();

        if (buffer_has_data) {
            return true;
        }
        else {
            return populate_buffer();
        }
    }

    virtual Position fill(const TreeNodePtr& leaf, const Position& start) = 0;

    DataPositions buffer_size() const {
        return size_ - start_;
    }


    DataPositions rank(int32_t idx) const
    {
        DataPositions rnk;

        int32_t start_pos = start_.sum();

        input_buffer_.symbols().rank_to(start_pos + idx, rnk.span());

        return rnk - start_;
    }

    DataPositions rank() const {
        return buffer_size();
    }

    virtual bool populate_buffer()
    {
        if (start_.sum() < size_.sum())
        {
            return true;
        }
        else if (!finished_)
        {
            do_populate_iobuffer();

            if (finished_)
            {
                return start_.sum() < size_.sum();
            }
            else {
                return true;
            }
        }
        else {
            return false;
        }
    }

    void do_populate_iobuffer()
    {
        const auto& seq = input_buffer_.symbols();

        start_.clear();
        size_.clear();

        input_buffer_.clear();
        finished_ = producer_(input_buffer_);

        reindex_ctr_batch_input(input_buffer_);

        seq.rank_to(input_buffer_.symbols().size(), size_.span());
    }

    DataPositions to_data_positions(const Position& pos)
    {
        DataPositions dp;

        for (int32_t c = 0; c < DataPositions::Indexes; c++) {
            dp[c] = pos[c];
        }

        return dp;
    }
};











template <
    typename CtrT,
    int32_t Streams = CtrT::Types::Streams,
    LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength
>
class CtrBatchInputProvider;



template <
    typename CtrT,
    int32_t Streams
>
class CtrBatchInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrBatchInputProviderBase<CtrT> {

    using Base = AbstractCtrBatchInputProviderBase<CtrT>;

    static constexpr float FREE_SPACE_THRESHOLD = 0.1;

public:
    using MyType = CtrBatchInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using typename Base::DataPositions;
    using typename Base::Position;
    using typename Base::CtrInputBuffer;

    using Base::StructureStreamIdx;
    using Base::DataStreams;


protected:
    using Base::rank;
    using Base::ctr_;
    using Base::start_;
    using Base::size_;
    using Base::totals_;
    using Base::total_symbols_;
    using Base::to_data_positions;

    using Base::input_buffer_;

public:

    CtrBatchInputProvider(
            CtrT& ctr,
            CtrBatchInputFn<CtrInputBuffer> producer,
            CtrInputBuffer& input_buffer
    ):
        Base(ctr, producer, input_buffer)
    {}

    CtrT& ctr() {
        return ctr_;
    }

    Position to_ctr_positions(const Position& ctr_start, const DataPositions& start, const DataPositions& end)
    {
        Position ctr_end;

        for (int32_t c = 0; c < DataPositions::Indexes; c++)
        {
            ctr_end[c] = end[c];
        }

        ctr_end[StructureStreamIdx] = ctr_start[StructureStreamIdx] + end.sum() - start.sum();

        return ctr_end;
    }

    virtual Position fill(const TreeNodePtr& leaf, const Position& start)
    {
        DataPositions data_start = to_data_positions(start);
        DataPositions pos        = data_start;

        while(true)
        {
            auto has_data = this->hasData();

            if (!has_data) {
                break;
            }

            auto buffer_sizes = this->buffer_size();

            auto inserted = insertBuffer(leaf, pos, buffer_sizes);

            if (inserted.sum() > 0)
            {
                pos += inserted;

                if (!hasFreeSpace(leaf))
                {
                    break;
                }
            }
            else {
                break;
            }
        }

        return to_ctr_positions(start, data_start, pos);
    }


    virtual DataPositions insertBuffer(const TreeNodePtr& leaf, DataPositions at, const DataPositions& size)
    {
        auto status0 = tryInsertBuffer(leaf, at, size);
        if (status0)
        {
            start_ += size;
            totals_ += size;
            total_symbols_ += size.sum();
            return size;
        }
        else {
            auto imax = size.sum();
            decltype(imax) imin  = 0;
            decltype(imax) start = 0;

            DataPositions tmp = at;

            while (imax > imin && hasFreeSpace(leaf))
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    int32_t try_block_size = mid - start;

                    auto sizes = rank(try_block_size);

                    auto status2 = tryInsertBuffer(leaf, at, sizes);
                    if (status2)
                    {
                        imin = mid + 1;

                        start = mid;
                        at += sizes;
                        start_ += sizes;

                        totals_ += sizes;
                        total_symbols_ += sizes.sum();
                    }
                    else {
                        imax = mid - 1;
                    }
                }
                else {
                    auto sizes = rank(1);

                    auto status1 = tryInsertBuffer(leaf, at, sizes);
                    if (status1)
                    {
                        start += 1;
                        at += sizes;
                        start_ += sizes;
                        totals_ += sizes;
                        total_symbols_ += sizes.sum();
                    }

                    break;
                }
            }

            return at - tmp;
        }
    }

protected:

    struct PrepareInsertBuffersFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <
                size_t StreamIdx, size_t AllocatorIdx, size_t Idx, size_t StreamsStartIdx,
                typename StreamObj, typename UpdateState
        >
        void stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                SizeTList<StreamsStartIdx>,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateState& update_state
        )
        {
            if (is_success(status)) {
                status = detail::PrepareStreamSelector<
                    StreamIdx < DataStreams ? detail::StreamSelectorType::DATA : detail::StreamSelectorType::STRUCTURE,
                    DataStreams
                >::template io_stream<StreamIdx, Idx>(
                    std::forward<StreamObj>(stream),
                    alloc,
                    at,
                    starts,
                    sizes,
                    input_buffer,
                    std::get<AllocatorIdx - StreamsStartIdx>(update_state)
                );
            }
        }


        template <
                size_t StreamIdx, size_t AllocatorIdx, size_t Idx, size_t StreamsStartIdx,
                typename ExtData, typename PakdStruct, typename UpdateState
        >
        void stream(
                PackedSizedStructSO<ExtData, PakdStruct>& stream,
                PackedAllocator* alloc,
                SizeTList<StreamsStartIdx>,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateState& update_state)
        {
            static_assert(StreamIdx < Streams, "");            
            // Always succsseds
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        void treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args)
        {
            constexpr size_t StreamsStartIdx = NodeT::StreamsStart;
            return leaf.processSubstreamGroups(
                        *this,
                        leaf.allocator(),
                        SizeTList<StreamsStartIdx>{},
                        std::forward<Args>(args)...
            );
        }
    };


    struct CommitInsertBuffersFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        template <
                size_t StreamIdx, size_t AllocatorIdx, size_t Idx, size_t StreamsStartIdx,
                typename StreamObj, typename UpdateState
        >
        void stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                SizeTList<StreamsStartIdx>,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateState& update_state
        )
        {
            detail::CommitStreamSelector<
                    StreamIdx < DataStreams ? detail::StreamSelectorType::DATA : detail::StreamSelectorType::STRUCTURE,
                    DataStreams
            >::template io_stream<StreamIdx, Idx>(
                    std::forward<StreamObj>(stream),
                    alloc,
                    at,
                    starts,
                    sizes,
                    input_buffer,
                    std::get<AllocatorIdx - StreamsStartIdx>(update_state)
            );
        }


        template <
                size_t StreamIdx, size_t AllocatorIdx, size_t Idx, size_t StreamsStartIdx,
                typename ExtData, typename PakdStruct, typename UpdateState
        >
        void stream(
                PackedSizedStructSO<ExtData, PakdStruct>& stream,
                PackedAllocator* alloc,
                SizeTList<StreamsStartIdx>,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const CtrInputBuffer& input_buffer,
                UpdateState& update_state)
        {
            static_assert(StreamIdx < Streams, "");
            return stream.insert_space(at[StreamIdx], sizes[StreamIdx]);
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        void treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args)
        {
            constexpr size_t StreamsStartIdx = NodeT::StreamsStart;
            return leaf.processSubstreamGroups(
                  *this,
                  leaf.allocator(),
                  SizeTList<StreamsStartIdx>{},
                  std::forward<Args>(args)...
            );
        }
    };




    Position to_position(const DataPositions& data_pos)
    {
        Position pos;

        for (int32_t c = 0; c < DataPositions::Indexes; c++)
        {
            pos[c] = data_pos[c];
        }

        pos[StructureStreamIdx] = data_pos.sum();

        return pos;
    }


    bool tryInsertBuffer(const TreeNodePtr& leaf, const DataPositions& at, const DataPositions& size)
    {
        PrepareInsertBuffersFn insert_fn1;

        auto update_state = ctr().template ctr_make_leaf_update_state<IntList<>>(leaf.as_immutable());
        ctr().leaf_dispatcher().dispatch(
                    leaf,
                    insert_fn1,
                    to_position(at),
                    to_position(start_),
                    to_position(size),
                    input_buffer_,
                    update_state
        );

        if (is_success(insert_fn1.status))
        {
            CommitInsertBuffersFn insert_fn2;

            ctr().leaf_dispatcher().dispatch(
                        leaf,
                        insert_fn2,
                        to_position(at),
                        to_position(start_),
                        to_position(size),
                        input_buffer_,
                        update_state
            );
            return true;
        }

        return false;
    }

    static float getFreeSpacePart(const TreeNodePtr& node)
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }

    static bool hasFreeSpace(const TreeNodePtr& node) {
        return getFreeSpacePart(node) > FREE_SPACE_THRESHOLD;
    }
};



}}
