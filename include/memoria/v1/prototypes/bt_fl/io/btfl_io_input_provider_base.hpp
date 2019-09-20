
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/assert.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {

namespace _ {

    enum class StreamSelectorType {DATA, STRUCTURE};

    template <StreamSelectorType TT, int32_t DataStreams> struct StreamSelector;

    template <int32_t DataStreams>
    struct StreamSelector<StreamSelectorType::DATA, DataStreams>
    {
        template <int32_t StreamIdx, typename StreamObj, typename Position>
        static void io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::v1::io::IOVector& io_vector,
                OpStatus& status,
                int32_t& current_substream
        )
        {
            static_assert(StreamIdx < DataStreams, "");
            if (isOk(status))
            {
                status <<= stream.insert_io_substream(
                    at[StreamIdx],
                    io_vector.substream(current_substream),
                    starts[StreamIdx],
                    sizes[StreamIdx]
                );
            }

            current_substream++;
        }
    };

    template <int32_t DataStreams>
    struct StreamSelector<StreamSelectorType::STRUCTURE, DataStreams>
    {
        template <int32_t StreamIdx, typename StreamObj, typename Position>
        static void io_stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::v1::io::IOVector& io_vector,
                OpStatus& status,
                int32_t& current_substream
        )
        {
            static_assert(StreamIdx == DataStreams, "");
            if (isOk(status))
            {
                status <<= stream.insert_io_substream(
                    at[StreamIdx],
                    io_vector.symbol_sequence(),
                    starts[StreamIdx],
                    sizes[StreamIdx]
                );
            }

            current_substream++;
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

    using NodeBaseG             = typename CtrT::Types::NodeBaseG;
    using CtrSizeT              = typename CtrT::Types::CtrSizeT;
    using Position              = typename CtrT::Types::Position;
    using DataPositions         = core::StaticVector<uint64_t, DataStreams>;
    using CtrDataPositionsT     = core::StaticVector<int64_t, DataStreams>;


    using Iterator = typename CtrT::Iterator;
    using NodePair = std::pair<NodeBaseG, NodeBaseG>;

protected:

    DataPositions start_;
    DataPositions size_;

    bool finished_{false};

    CtrT& ctr_;

    CtrSizeT orphan_splits_{};

    NodePair split_watcher_;

    CtrSizeT total_symbols_{};

    CtrDataPositionsT totals_{};

    memoria::v1::io::IOVectorProducer* producer_{};
    memoria::v1::io::IOVector* io_vector_{};

    CtrSizeT start_pos_;
    CtrSizeT length_;

public:

    AbstractCtrInputProviderBase(
            CtrT& ctr,
            memoria::v1::io::IOVectorProducer* producer,
            memoria::v1::io::IOVector* io_vector,
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

    virtual bool hasData()
    {
        bool buffer_has_data = start_.sum() < size_.sum();
        return buffer_has_data || populate_buffer();
    }

    virtual Position fill(NodeBaseG& leaf, const Position& start) = 0;

    void iter_next_leaf(const NodeBaseG& leaf) {}

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
        const auto& seq = io_vector_->symbol_sequence();

        do
        {
            start_.clear();
            size_.clear();

            io_vector_->reset();
            finished_ = producer_->populate(*io_vector_);
            io_vector_->reindex();

            seq.rank_to(io_vector_->symbol_sequence().size(), &size_[0]);

            if (MMA1_UNLIKELY(start_pos_ > 0))
            {
                int32_t seq_size = seq.size();
                if (start_pos_ < seq_size)
                {
                    seq.rank_to(start_pos_, &start_[0]);
                    start_pos_ -= start_.sum();
                }
                else {
                    start_pos_ -= seq_size;
                }
            }
        }
        while (start_pos_ > 0);

        CtrSizeT remainder = length_ - total_symbols_;
        if (MMA1_UNLIKELY(length_ < std::numeric_limits<CtrSizeT>::max() && (size_ - start_).sum() > remainder))
        {
            seq.rank_to(start_.sum() + remainder, &size_[0]);
            finished_ = true;
        }
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

    using NodeBaseG = typename CtrT::Types::NodeBaseG;
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
            memoria::v1::io::IOVectorProducer* producer,
            memoria::v1::io::IOVector* io_vector,
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

    virtual Position fill(NodeBaseG& leaf, const Position& start)
    {
        DataPositions data_start = to_data_positions(start);
        DataPositions pos        = data_start;

        BlockUpdateMgr mgr(ctr());

        mgr.add(leaf);

        while(this->hasData())
        {
            auto buffer_sizes = this->buffer_size();

            auto inserted = insertBuffer(mgr, leaf, pos, buffer_sizes);

            if (inserted.sum() > 0)
            {
                //TODO update leaf's parents here
                if (leaf->parent_id().isSet())
                {
                    ctr().ctr_update_path(leaf);
                }

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


    virtual DataPositions insertBuffer(BlockUpdateMgr& mgr, NodeBaseG& leaf, DataPositions at, const DataPositions& size)
    {
        if (tryInsertBuffer(mgr, leaf, at, size))
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
                    if (tryInsertBuffer(mgr, leaf, at, sizes))
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
                    if (tryInsertBuffer(mgr, leaf, at, sizes))
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

    struct InsertBuffersFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        int32_t current_substream_{};
        OpStatus status_{OpStatus::OK};

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamObj>
        void stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                const memoria::v1::io::IOVector& io_vector)
        {
            _::StreamSelector<
                    StreamIdx < DataStreams ? _::StreamSelectorType::DATA : _::StreamSelectorType::STRUCTURE,
                    DataStreams
            >::template io_stream<StreamIdx>(
                    stream,
                    alloc,
                    at,
                    starts,
                    sizes,
                    io_vector,
                    status_,
                    current_substream_
            );
        }

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx,
                typename ExtData, typename PakdStruct
        >
        void stream(
                PackedSizedStructSO<ExtData, PakdStruct>& stream,
                PackedAllocator* alloc,
                const Position& at,
                const Position& starts,
                const Position& sizes,
                memoria::v1::io::IOVector& io_vector)
        {
            static_assert(StreamIdx < Streams, "");
            if (isOk(status_))
            {
                status_ <<= stream.insertSpace(at[StreamIdx], sizes[StreamIdx]);
            }
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        auto treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args)
        {
            leaf.layout(255);
            leaf.processSubstreamGroups(*this, leaf.allocator(), std::forward<Args>(args)...);

            return;
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


    MMA1_NODISCARD bool tryInsertBuffer(BlockUpdateMgr& mgr, NodeBaseG& leaf, const DataPositions& at, const DataPositions& size)
    {
        InsertBuffersFn insert_fn;

        ctr().leaf_dispatcher().dispatch(
                    leaf,
                    insert_fn,
                    to_position(at),
                    to_position(start_),
                    to_position(size),
                    *io_vector_
        );

        if (isFail(insert_fn.status_)) {
            mgr.restoreNodeState();
            return false;
        }

        mgr.checkpoint(leaf);

        return true;
    }

    static float getFreeSpacePart(const NodeBaseG& node)
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }

    static bool hasFreeSpace(const NodeBaseG& node)
    {
        return getFreeSpacePart(node) > FREE_SPACE_THRESHOLD;
    }
};



}}}}
