
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/core/tools/assert.hpp>

#include <memory>

namespace memoria {
namespace btfl {
namespace io {

namespace _ {

    enum class StreamSelectorType {DATA, STRUCTURE};

    template <StreamSelectorType TT, int32_t DataStreams> struct StreamSelector;

    template <int32_t DataStreams>
    struct StreamSelector<StreamSelectorType::DATA, DataStreams>
    {
        template <int32_t StreamIdx, typename StreamObj, typename Position>
        static VoidResult io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::io::IOVector& io_vector,
                int32_t& current_substream
        ) noexcept
        {
            static_assert(StreamIdx < DataStreams, "");

            MEMORIA_TRY_VOID(stream.insert_io_substream(
                    at[StreamIdx],
                    io_vector.substream(current_substream),
                    starts[StreamIdx],
                    sizes[StreamIdx]
            ));

            current_substream++;

            return VoidResult::of();
        }
    };

    template <int32_t DataStreams>
    struct StreamSelector<StreamSelectorType::STRUCTURE, DataStreams>
    {
        template <int32_t StreamIdx, typename StreamObj, typename Position>
        static VoidResult io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::io::IOVector& io_vector,
                int32_t& current_substream
        ) noexcept
        {
            static_assert(StreamIdx == DataStreams, "");
            MEMORIA_TRY_VOID(stream.insert_io_substream(
                    at[StreamIdx],
                    io_vector.symbol_sequence(),
                    starts[StreamIdx],
                    sizes[StreamIdx]
            ));

            current_substream++;

            return VoidResult::of();
        }
    };

}




template <typename CtrT>
class AbstractCtrInputProviderBase {

protected:
    static const int32_t Streams                = CtrT::Types::Streams;
    static const int32_t DataStreams            = CtrT::Types::DataStreams;
    static const int32_t StructureStreamIdx     = CtrT::Types::StructureStreamIdx;


public:
    using MyType = AbstractCtrInputProviderBase<CtrT>;

    using TreeNodePtr             = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT              = typename CtrT::Types::CtrSizeT;
    using Position              = typename CtrT::Types::Position;
    using DataPositions         = core::StaticVector<uint64_t, DataStreams>;
    using CtrDataPositionsT     = core::StaticVector<int64_t, DataStreams>;


    using Iterator = typename CtrT::Iterator;
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

    memoria::io::IOVectorProducer* producer_{};
    memoria::io::IOVector* io_vector_{};

    CtrSizeT start_pos_;
    CtrSizeT length_;

public:

    AbstractCtrInputProviderBase(
            CtrT& ctr,
            memoria::io::IOVectorProducer* producer,
            memoria::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length
    ):
        ctr_(ctr), producer_(producer), io_vector_(io_vector),
        start_pos_(start_pos), length_(length)
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

    virtual BoolResult hasData() noexcept
    {
        bool buffer_has_data = start_.sum() < size_.sum();

        if (buffer_has_data) {
            return BoolResult::of(true);
        }
        else {
            return populate_buffer();
        }
    }

    virtual Result<Position> fill(TreeNodePtr& leaf, const Position& start) noexcept = 0;

    VoidResult iter_next_leaf(const TreeNodePtr& leaf) noexcept {
        return VoidResult::of();
    }

    DataPositions buffer_size() const
    {
        return size_ - start_;
    }


    DataPositions rank(int32_t idx) const
    {
        DataPositions rnk;

        int32_t start_pos = start_.sum();

        io_vector_->symbol_sequence().rank_to(start_pos + idx, &rnk[0]);

        return rnk - start_;
    }

    DataPositions rank() const {
        return buffer_size();
    }

    virtual BoolResult populate_buffer() noexcept
    {
        if (start_.sum() < size_.sum())
        {
            return BoolResult::of(true);
        }
        else if (!finished_)
        {
            MEMORIA_TRY_VOID(do_populate_iobuffer());

            if (finished_)
            {
                return BoolResult::of(start_.sum() < size_.sum());
            }
            else {
                return BoolResult::of(true);
            }
        }
        else {
            return BoolResult::of(false);
        }
    }

    VoidResult do_populate_iobuffer() noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
        const auto& seq = io_vector_->symbol_sequence();

        do
        {
            start_.clear();
            size_.clear();

            io_vector_->clear();
            finished_ = producer_->populate(*io_vector_);
            io_vector_->reindex();

            seq.rank_to(io_vector_->symbol_sequence().size(), &size_[0]);

            if (MMA_UNLIKELY(start_pos_ > 0))
            {
                int32_t ctr_seq_size = seq.size();
                if (start_pos_ < ctr_seq_size)
                {
                    seq.rank_to(start_pos_, &start_[0]);
                    start_pos_ -= start_.sum();
                }
                else {
                    start_pos_ -= ctr_seq_size;
                }
            }
        }
        while (start_pos_ > 0);

        CtrSizeT remainder = length_ - total_symbols_;
        if (MMA_UNLIKELY(length_ < std::numeric_limits<CtrSizeT>::max() && (size_ - start_).sum() > remainder))
        {
            seq.rank_to(start_.sum() + remainder, &size_[0]);
            finished_ = true;
        }

        return VoidResult::of();
        });
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
class IOVectorCtrInputProvider;



template <
    typename CtrT,
    int32_t Streams
>
class IOVectorCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrInputProviderBase<CtrT> {

    using Base = AbstractCtrInputProviderBase<CtrT>;

    static constexpr float FREE_SPACE_THRESHOLD = 0.1;

public:
    using MyType = IOVectorCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;
    using Iterator  = typename CtrT::Iterator;

    using BlockUpdateMgr = typename CtrT::Types::BlockUpdateMgr;

    using typename Base::DataPositions;
    using typename Base::Position;

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

    using Base::io_vector_;

public:

    IOVectorCtrInputProvider(
            CtrT& ctr,
            memoria::io::IOVectorProducer* producer,
            memoria::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length
    ):
        Base(ctr, producer, io_vector, start_pos, length)
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

    virtual Result<Position> fill(TreeNodePtr& leaf, const Position& start) noexcept
    {
        using ResultT = Result<Position>;

        DataPositions data_start = to_data_positions(start);
        DataPositions pos        = data_start;

        BlockUpdateMgr mgr(ctr());

        MEMORIA_TRY_VOID(mgr.add(leaf));

        while(true)
        {
            MEMORIA_TRY(has_data, this->hasData());

            if (!has_data) {
                break;
            }

            auto buffer_sizes = this->buffer_size();

            MEMORIA_TRY(inserted, insertBuffer(mgr, leaf, pos, buffer_sizes));

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

        return ResultT::of(to_ctr_positions(start, data_start, pos));
    }


    virtual Result<DataPositions> insertBuffer(BlockUpdateMgr& mgr, TreeNodePtr& leaf, DataPositions at, const DataPositions& size) noexcept
    {
        using ResultT = Result<DataPositions>;

        MEMORIA_TRY(status0, tryInsertBuffer(mgr, leaf, at, size));
        if (status0)
        {
            start_ += size;
            totals_ += size;
            total_symbols_ += size.sum();
            return ResultT::of(size);
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

                    MEMORIA_TRY(status2, tryInsertBuffer(mgr, leaf, at, sizes));
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

                    MEMORIA_TRY(status1, tryInsertBuffer(mgr, leaf, at, sizes));
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

            return ResultT::of(at - tmp);
        }
    }

protected:

    struct InsertBuffersFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        int32_t current_substream_{};


        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamObj>
        VoidResult stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::io::IOVector& io_vector) noexcept
        {
            return _::StreamSelector<
                    StreamIdx < DataStreams ? _::StreamSelectorType::DATA : _::StreamSelectorType::STRUCTURE,
                    DataStreams
            >::template io_stream<StreamIdx>(
                    stream,
                    alloc,
                    at,
                    starts,
                    sizes,
                    io_vector,
                    current_substream_
            );
        }

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx,
                typename ExtData, typename PakdStruct
        >
        VoidResult stream(
                PackedSizedStructSO<ExtData, PakdStruct>& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                memoria::io::IOVector& io_vector) noexcept
        {
            static_assert(StreamIdx < Streams, "");
            return stream.insertSpace(at[StreamIdx], sizes[StreamIdx]);
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        VoidResult treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args) noexcept
        {
            MEMORIA_TRY_VOID(leaf.layout(255));
            return leaf.processSubstreamGroups(*this, leaf.allocator(), std::forward<Args>(args)...);
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


    BoolResult tryInsertBuffer(BlockUpdateMgr& mgr, TreeNodePtr& leaf, const DataPositions& at, const DataPositions& size) noexcept
    {
        InsertBuffersFn insert_fn;

        VoidResult status = ctr().leaf_dispatcher().dispatch(
                    leaf,
                    insert_fn,
                    to_position(at),
                    to_position(start_),
                    to_position(size),
                    *io_vector_
        );

        if (status.is_error()) {
            if (status.is_packed_error())
            {
                mgr.restoreNodeState();
                return BoolResult::of(false);
            }
        }

        mgr.checkpoint(leaf);

        return BoolResult::of(true);
    }

    static float getFreeSpacePart(const TreeNodePtr& node) noexcept
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }

    static bool hasFreeSpace(const TreeNodePtr& node) noexcept
    {
        return getFreeSpacePart(node) > FREE_SPACE_THRESHOLD;
    }
};



}}}
