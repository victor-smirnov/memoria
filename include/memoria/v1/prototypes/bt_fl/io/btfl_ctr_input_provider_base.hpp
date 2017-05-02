
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_data_input.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_structure_input.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_rank_dictionary.hpp>


#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {

namespace {

    template <typename Types, int32_t Streams, int32_t Idx = 0>
    struct DataStreamInputBufferBuilder {
        using InputBuffer = StreamInputBuffer<
                Idx,
                typename Types::template StreamInputBufferStructList<Idx>
        >;

        using UInputBufferPtr   = InputBufferHandler<InputBuffer>;
        using SizedBuffer       = DataStreamInputBuffer<UInputBufferPtr, typename Types::CtrSizeT>;

        using Type = MergeLists<
                SizedBuffer,
                typename DataStreamInputBufferBuilder<Types, Streams, Idx + 1>::Type
        >;
    };

    template <typename Types, int32_t Streams>
    struct DataStreamInputBufferBuilder<Types, Streams, Streams> {
        using Type = TL<>;
    };

    template <typename... Args> struct JoinBuffersH;

    template <typename... T1, typename T2>
    struct JoinBuffersH<std::tuple<T1...>, T2>: HasType<std::tuple<const T1*..., const T2*>> {};
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
    using DataPositions         = core::StaticVector<int32_t, DataStreams>;
    using CtrDataPositionsT     = core::StaticVector<int64_t, DataStreams>;


    using Iterator = typename CtrT::Iterator;

    using DataStreamBuffers = AsTuple<
            typename DataStreamInputBufferBuilder<
                typename CtrT::Types,
                DataStreams
            >::Type
    >;



    using ForAllDataStreams = ForAllTuple<std::tuple_size<DataStreamBuffers>::value>;

    using NodePair = std::pair<NodeBaseG, NodeBaseG>;
    using RankDictionaryT = RankDictionary<DataStreams>;

    using StructureStreamBuffer = StructureStreamInputBuffer<
            InputBufferHandler<
                StreamInputBuffer<
                    StructureStreamIdx,
                    typename CtrT::Types::template StreamInputBufferStructList<StructureStreamIdx>
                >
            >,
            CtrSizeT
    >;

protected:

    DataStreamBuffers       data_buffers_;
    StructureStreamBuffer   structure_buffer_;


    DataPositions start_;
    DataPositions size_;
    bool finished_ = false;


    RankDictionaryT symbols_;

    CtrT& ctr_;

    CtrSizeT orphan_splits_ = 0;

    NodePair split_watcher_;


    CtrSizeT total_symbols_ = 0;

    CtrDataPositionsT totals_;

private:
    struct CreateBufferFn {
        template <int32_t Idx, typename Buffer>
        void process(Buffer&& buffer, int32_t initial_capacity)
        {
            buffer.init(initial_capacity);
        }
    };


    struct FinishBufferFn {
        template <int32_t Idx, typename Buffer>
        void process(Buffer&& buffer)
        {
            buffer.finish();
        }
    };

    struct DumpBufferFn {
        template <int32_t Idx, typename Buffer, typename EventConsumer>
        void process(Buffer&& buffer, EventConsumer&& consumer, std::ostream& out)
        {
            out<<"Begin Stream Dump: "<<Idx<<std::endl;
            buffer.buffer()->generateDataEvents(&consumer);
            out<<"End Stream Dump: "<<Idx<<std::endl<<std::endl<<std::endl<<std::endl;
        }
    };

    struct ResetBufferFn {
        template <int32_t Idx, typename Buffer>
        void process(Buffer&& buffer)
        {
            buffer.reset();
        }
    };

public:

    AbstractCtrInputProviderBase(CtrT& ctr, int32_t initial_capacity):
        symbols_(initial_capacity),
        ctr_(ctr)

    {
        ForAllDataStreams::process(data_buffers_, CreateBufferFn(), initial_capacity);

        structure_buffer_.init(initial_capacity);
    }


protected:
    void finish_buffer()
    {
        ForAllDataStreams::process(data_buffers_, FinishBufferFn());

        structure_buffer_.finish();
    }

    void reset_buffer()
    {
        ForAllDataStreams::process(data_buffers_, ResetBufferFn());

        structure_buffer_.reset();
    }

public:

    void init()
    {
    	start_.clear();
    	size_.clear();

    	finished_ 		 = false;
    	total_symbols_ = 0;
    }


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

    void nextLeaf(const NodeBaseG& leaf) {}



    const DataStreamBuffers& data_buffer() const {
        return data_buffers_;
    }

    DataPositions buffer_size() const
    {
        return size_ - start_;
    }


    DataPositions rank(CtrSizeT idx) const
    {
        DataPositions rnk;

        int32_t start_pos = start_.sum();

        const auto* symbols = this->symbols();

        for (int32_t s = 0; s < DataStreams; s++)
        {
            rnk[s] = symbols->rank(start_pos + idx, s);
        }

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

    virtual void do_populate_iobuffer() = 0;


    void dump_buffer(std::ostream& out = std::cout) const
    {
        TextPageDumper dumper(out);
        ForAllDataStreams::process(data_buffers_, DumpBufferFn(), dumper, out);

        out<<"Begin Symbols"<<std::endl;
        symbols()->generateDataEvents(&dumper);
        out<<"End Symbols"<<std::endl<<std::endl<<std::endl<<std::endl;
    }

    DataPositions to_data_positions(const Position& pos)
    {
        DataPositions dp;

        for (int32_t c = 0; c < DataPositions::Indexes; c++) {
            dp[c] = pos[c];
        }

        return dp;
    }



protected:
    auto* symbols() {
        return symbols_.get();
    }

    const auto* symbols() const {
        return symbols_.get();
    }
};











template <
    typename CtrT,
    int32_t Streams = CtrT::Types::Streams,
    LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength
>
class AbstractCtrInputProvider;



template <
    typename CtrT,
    int32_t Streams
>
class AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrInputProviderBase<CtrT> {

    using Base = AbstractCtrInputProviderBase<CtrT>;

    static constexpr float FREE_SPACE_THRESHOLD = 0.1;


public:
    using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;
    using Iterator  = typename CtrT::Iterator;

    using PageUpdateMgr         = typename CtrT::Types::PageUpdateMgr;

    using typename Base::DataPositions;
    using typename Base::Position;
    using typename Base::DataStreamBuffers;
    using typename Base::StructureStreamBuffer;

protected:
    using Base::rank;
    using Base::ctr_;
    using Base::start_;
    using Base::size_;
    using Base::data_buffers_;
    using Base::structure_buffer_;
    using Base::to_data_positions;


    using Base::DataStreams;
    using Base::StructureStreamIdx;

public:

    AbstractCtrInputProvider(CtrT& ctr, int32_t total_capacity):
        Base(ctr, total_capacity)
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

        PageUpdateMgr mgr(ctr());

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
                    ctr().update_path(leaf);
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


    virtual DataPositions insertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, DataPositions at, const DataPositions& size)
    {
        if (tryInsertBuffer(mgr, leaf, at, size))
        {
            start_ += size;
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
                    }

                    break;
                }
            }

            return at - tmp;
        }
    }

protected:

    struct InsertBuffersFn {

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamObj, typename StreamBuffer>
        void stream(StreamObj* stream, PackedAllocator* alloc, const Position& at, const Position& starts, const Position& sizes, StreamBuffer&& buffer)
        {
            static_assert(StreamIdx < std::tuple_size<StreamBuffer>::value, "");

            stream->insert_buffer(
                    at[StreamIdx],
                    std::get<StreamIdx>(buffer)->buffer()->template substream_by_idx<Idx>(),
                    starts[StreamIdx],
                    sizes[StreamIdx]
            );
        }

        template <typename NodeTypes, typename... Args>
        auto treeNode(LeafNode<NodeTypes>* leaf, Args&&... args)
        {
            leaf->layout(255);
            return leaf->processSubstreamGroups(*this, leaf->allocator(), std::forward<Args>(args)...);
        }
    };


    struct AssignDataBuffersFn {
        template <int32_t Idx, typename DataBuffers, typename JointBuffers>
        void process(DataBuffers&& data_buffers, JointBuffers&& joint_buffers)
        {
            std::get<Idx>(joint_buffers) = &data_buffers;
        }
    };



    auto make_joined_buffers_tuple()
    {
        using JointBufferTupleT = typename JoinBuffersH<DataStreamBuffers, StructureStreamBuffer>::Type;

        JointBufferTupleT joint_buffer;

        ForAllTuple<std::tuple_size<DataStreamBuffers>::value>::process(data_buffers_, AssignDataBuffersFn(), joint_buffer);

        std::get<StructureStreamIdx>(joint_buffer) = &structure_buffer_;

        return joint_buffer;
    }

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


    bool tryInsertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, const DataPositions& at, const DataPositions& size)
    {
        try {
            CtrT::Types::Pages::LeafDispatcher::dispatch(
                    leaf,
                    InsertBuffersFn(),
                    to_position(at),
                    to_position(start_),
                    to_position(size),
                    make_joined_buffers_tuple()
            );

            mgr.checkpoint(leaf);

            return true;
        }
        catch (PackedOOMException& ex)
        {
            mgr.restoreNodeState();
            return false;
        }
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
