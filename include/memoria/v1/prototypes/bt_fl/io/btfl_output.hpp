
// Copyright 2016 Victor Smirnov
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
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/v1/prototypes/bt/bt_iterator.hpp>

#include <memory>
#include <tuple>
#include <limits>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {

using bt::StreamTag;

enum class Ending {
    CONTINUE, END_OF_PAGE, END_OF_IOBUFFER, LIMIT_REACHED
};

template <typename PkdStruct>
class ReadIterator {
    using ReadState = typename PkdStruct::ReadState;

    const PkdStruct* struct_ = nullptr;
    ReadState state_;
    Int idx_  = 0;
    Int size_ = 0;

public:
    ReadIterator(const PkdStruct* pstruct, Int idx = 0): struct_(pstruct), state_(pstruct->positions(idx)), idx_(idx)
    {
        size_ = pstruct->size();
    }

    ReadIterator() {}

    Int size() const {
        return size_;
    }

    Int idx() const {
        return idx_;
    }

    template <typename IOBuffer>
    auto next(IOBuffer& io_buffer)
    {
        if (idx_ < size_)
        {
            if (struct_->readTo(state_, io_buffer))
            {
                return Ending::CONTINUE;
            }

            return Ending::END_OF_IOBUFFER;
        }
        else {
            return Ending::END_OF_PAGE;
        }
    }
};



namespace {
    template <typename LeafNode, Int Idx, Int Streams, template <typename> class MapFn>
    struct MapAllStreams4Data {
        using Type = MergeLists<
                AsTuple<typename LeafNode::template StreamDispatcher<Idx>::template ForAllStructs<MapFn>>,
                typename MapAllStreams4Data<LeafNode, Idx + 1, Streams, MapFn>::Type
        >;
    };

    template <typename LeafNode, Int Idx, template <typename> class MapFn>
    struct MapAllStreams4Data<LeafNode, Idx, Idx, MapFn> {
        using Type = TL<>;
    };
}













class PopulateStatus {
    Int entries_;
    Ending ending_;
public:
    PopulateStatus(Int entries, Ending ending): entries_(entries), ending_(ending)
    {}

    PopulateStatus(): entries_(0), ending_(Ending::CONTINUE)
    {}

    auto entries() const {return entries_;}
    Ending ending() const {return ending_;}
};


/**
 * Scans within only the run of the same symbols
 */

template <typename T>
class ScanRunGTStrategy {
    BigInt limit_;
    BigInt total_;
    Int stream_;
public:
    void init(Int stream, BigInt limit) {
        stream_ = stream;
        total_ = 0;
        limit_ = limit;
    }

    BigInt accept(Int stream, BigInt length) const
    {
        if (stream_ == stream)
        {
            return total_ + length <= limit_ ? length : limit_ - total_;
        }
        else if (stream > stream_)
        {
            return length;
        }
        else {
            return 0;
        }
    }


    void commit(Int stream, BigInt length)
    {
        if (stream == stream_)
        {
            total_ += length;
        }
    }


    BigInt totals() const {
        return total_;
    }
};

/**
 * Scans through all runs
 */

template <typename T>
class ScanThroughStrategy {
        BigInt limit_;
        BigInt total_;
public:
    void init(Int stream, BigInt limit) {
        total_ = 0;
        limit_ = limit;
    }

    BigInt accept(Int stream, BigInt length) const
    {
        return total_ + length <= limit_ ? length : limit_ - total_;
    }

    void commit(Int stream, BigInt length) {
        total_ += length;
    }

    BigInt totals() const {
        return total_;
    }
};


template <typename IteratorT, typename IOBufferT, template <typename> class ScanStrategy = ScanThroughStrategy>
class BTFLWalker {

    using CtrT      = typename IteratorT::Container;

    using Types     = typename CtrT::Types;
    using MyType    = BTFLWalker<IteratorT, IOBufferT, ScanStrategy>;

    using CtrSizeT      = typename Types::CtrSizeT;
    using CtrSizesT     = typename Types::CtrSizesT;
    using DataSizesT    = typename Types::DataSizesT;
    using NodeBaseG     = typename Types::NodeBaseG;

    using LeafDispatcher = typename Types::Pages::LeafDispatcher;
    using Iterator  = IteratorT;

    static constexpr Int DataStreams                = Types::DataStreams;
    static constexpr Int StructureStreamIdx         = Types::StructureStreamIdx;

    using DataStreamsSizes  = core::StaticVector<Int, DataStreams>;

    template <typename PackedStructDescr>
    using StreamDataMapFn = HasType<ReadIterator<typename PackedStructDescr::Type>>;

    using ReadStreamDataStates  = AsTuple<typename MapAllStreams4Data<typename Types::LeafNode, 0, DataStreams, StreamDataMapFn>::Type>;

    using StructureIterator     = typename Types::template LeafPackedStruct<IntList<StructureStreamIdx, 1>>::Iterator;




    class WriteStreamFn;
    friend class WriteStreamFn;

    class WriteStreamFn {

        MyType* walker_;

    public:

        PopulateStatus status_;

        WriteStreamFn(MyType* walker): walker_(walker) {}

        template <Int Idx, typename StreamsData>
        void process(StreamsData& stream_data, Int stream, BigInt length, IOBufferT& io_buffer)
        {
            if (Idx == stream)
            {
                Int entries = 0;

                for (BigInt c = 0; c < length; c++, entries++)
                {
                    auto backup       = stream_data;
                    auto write_status = write_entry(stream_data, io_buffer);

                    if (write_status != Ending::CONTINUE)
                    {
                        stream_data = backup;

                        status_ = PopulateStatus(entries, write_status);
                        return;
                    }
                }

                status_ = PopulateStatus(entries, Ending::CONTINUE);
            }
        }

        struct WriteEntryFn
        {
            Ending ending_ = Ending::CONTINUE;

            template <Int Idx, typename StreamsData>
            void process(StreamsData& stream_data, IOBufferT& io_buffer)
            {
                if (ending_ == Ending::CONTINUE)
                {
                    ending_ = stream_data.next(io_buffer);
                }
            }
        };

        template <typename StreamData>
        Ending write_entry(StreamData& stream_data, IOBufferT& io_buffer)
        {
            WriteEntryFn fn;
            ForAllTuple<std::tuple_size<std::remove_reference_t<StreamData>>::value>::process(stream_data, fn, io_buffer);
            return fn.ending_;
        }
    };

    ReadStreamDataStates  stream_data_;

    Iterator* iter_;

    Int idx_;

    NodeBaseG leaf_;

    CtrSizeT run_pos_     = 0;
    CtrSizeT run_length_  = 0;

    StructureIterator symbols_;

    Int stream_;

    ScanStrategy<MyType> scan_strategy_;

    bool limit_ = false;
    bool partial_run_ = false;

public:

    BTFLWalker(): iter_(), leaf_(), stream_()
    {}

    void init(Iterator& iter, Int expected_stream, CtrSizeT limit = std::numeric_limits<CtrSizeT>::max())
    {
    	limit_  = false;
    	iter_   = &iter;
    	leaf_   = iter.leaf();

    	idx_     = iter.idx();
    	symbols_ = leaf_structure()->iterator(idx_);

    	stream_     = symbols_.symbol();
    	run_pos_    = symbols_.local_idx();

    	scan_strategy_.init(expected_stream >= 0 ? expected_stream : stream_, limit);

    	run_length_ = run_pos_ + scan_strategy_.accept(stream_, symbols_.length() - run_pos_);
    	limit_      = run_length_ <= run_pos_;

    	partial_run_ = run_length_ < symbols_.length();

    	auto data_positions = rank(iter.idx());

    	configure_data(data_positions);
    }

    Int idx() const {
        return idx_;
    }

    auto totals() const {
        return scan_strategy_.totals();
    }

    auto& leaf() {
        return leaf_;
    }

    const auto& leaf() const {
        return leaf_;
    }




    void prepare_new_page(Int start_idx = 0)
    {
        idx_     = start_idx;
        symbols_ = leaf_structure()->iterator(idx_);

        stream_     = symbols_.symbol();
        run_pos_    = symbols_.local_idx();

        run_length_ = run_pos_ + scan_strategy_.accept(stream_, symbols_.length() - run_pos_);
        limit_      = run_length_ <= run_pos_;

        partial_run_ = run_length_ < symbols_.length();
    }

    PopulateStatus write_stream(Int stream, BigInt length, IOBufferT& io_buffer)
    {
        WriteStreamFn fn(this);
        ForAllTuple<DataStreams>::process(stream_data_, fn, stream, length, io_buffer);
        return fn.status_;
    }

    template <typename IOBuffer>
    PopulateStatus populate(IOBuffer& buffer)
    {
        Int entries = 0;

        while(symbols_.has_data() && !limit_)
        {
            size_t descr_pos = buffer.pos();

            auto length = run_length_ - run_pos_;

            if (buffer.template putSymbolsRun<DataStreams>(stream_, length))
            {
                entries++;
            }
            else {
                return PopulateStatus(entries, Ending::END_OF_IOBUFFER);
            }

            auto write_result = write_stream(stream_, length, buffer);

            auto entries_written = write_result.entries();

            if (entries_written < length)
            {
                if (entries_written > 0) {
                    buffer.template updateSymbolsRun<DataStreams>(descr_pos, stream_, entries_written);
                }
                else {
                    entries--;
                    return PopulateStatus(entries, Ending::END_OF_IOBUFFER);
                }
            }

            entries  += entries_written;
            run_pos_ += entries_written;
            idx_     += entries_written;

            scan_strategy_.commit(stream_, entries_written);

            if (run_pos_ == run_length_)
            {
                if (!partial_run_)
                {
                    MEMORIA_V1_ASSERT_FALSE(write_result.ending() == Ending::END_OF_IOBUFFER);

                    symbols_.next_run();

                    if (symbols_.has_data())
                    {
                        run_pos_        = 0;
                        stream_         = symbols_.symbol();
                        run_length_ = scan_strategy_.accept(stream_, symbols_.length());
                        limit_          = run_length_ <= run_pos_;

                        partial_run_ = run_length_ < symbols_.length();
                    }
                }
                else {
                    limit_ = true;
                }
            }
            else if (write_result.ending() == Ending::END_OF_IOBUFFER)
            {
                return PopulateStatus(entries, Ending::END_OF_IOBUFFER);
            }
        }

        return PopulateStatus(entries, !limit_ ? Ending::END_OF_PAGE : Ending::LIMIT_REACHED);
    }


    bool next_page()
    {
        NodeBaseG next = iter_->ctr().getNextNodeP(leaf_);

        if (next)
        {
            leaf_ = next;

            prepare_new_page();

            configure_data(DataStreamsSizes());

            return true;
        }
        else {
            return false;
        }
    }


    void clear() noexcept
            {
        leaf_ = nullptr;
        iter_ = nullptr;
            }

private:

    DataStreamsSizes rank(Int idx) const
    {
        DataStreamsSizes sizes;

        auto s = this->leaf_structure();

        for (Int c = 0; c < DataStreamsSizes::Indexes; c++)
        {
            sizes[c] = s->rank(idx, c);
        }

        return sizes;
    }

    const auto* leaf_structure() const
    {
        return getPackedStruct<IntList<StructureStreamIdx, 1>>();
    }


    template <typename SubstreamPath>
    struct GetPackedStructFn {
        template <typename T>
        auto treeNode(const LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }

        template <typename T>
        auto treeNode(LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }
    };

    template <typename SubstreamPath>
    const auto* getPackedStruct() const
    {
        return LeafDispatcher::dispatch(leaf_, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto* getPackedStruct()
    {
        return LeafDispatcher::dispatch(leaf_, GetPackedStructFn<SubstreamPath>());
    }


    void dump_symbols()
    {
        leaf_structure()->dump();
    }

    struct ConfigureDataFn {

        template <Int StreamIdx, typename StreamData, typename Node>
        void process(StreamData&& data, const DataStreamsSizes& idx, const Node* leaf)
        {
            constexpr Int Substreams = std::tuple_size<typename std::remove_reference<StreamData>::type>::value;

            ForAllTuple<Substreams>::process(data, *this, idx[StreamIdx], leaf, bt::StreamTag<StreamIdx>());
        }

        template <Int SubstreamIdx, typename StreamData, typename Node, Int StreamIdx>
        void process(StreamData&& data, Int idx, const Node* leaf, const bt::StreamTag<StreamIdx>&)
        {
            constexpr Int FullSubstreamIdx = Node::template StreamStartIdx<StreamIdx>::Value + SubstreamIdx;
            using T = typename Node::Dispatcher::template StreamTypeT<FullSubstreamIdx>::Type;

            const T* pstruct = leaf->allocator()->template get<T>(FullSubstreamIdx + Node::SubstreamsStart);

            data = ReadIterator<T>(pstruct, idx);
        }

        template <typename NTypes>
        void treeNode(const LeafNode<NTypes>* leaf, ReadStreamDataStates& stream_data, const DataStreamsSizes& idx)
        {
            ForAllTuple<DataStreams>::process(stream_data, *this, idx, leaf);
        }
    };


    void configure_data(const DataStreamsSizes& idx)
    {
        return LeafDispatcher::dispatch(leaf_, ConfigureDataFn(), stream_data_, idx);
    }
};





template <typename IOBufferT, typename Iterator>
class ChainedIOBufferProducer: public BufferProducer<IOBufferT> {

    using WalkerType = BTFLWalker<Iterator, IOBufferT>;

    Iterator* iter_;
    WalkerType walker_;
    IOBufferT io_buffer_;

public:
    ChainedIOBufferProducer(Iterator* iter, Int buffer_size = 65536):
        iter_(iter),
        walker_(*iter),
        io_buffer_(buffer_size)
    {
    }

    virtual IOBufferT& buffer() {
        return io_buffer_;
    }

    virtual Int populate(IOBufferT& buffer)
    {
        return iter_->bulkio_populate(walker_, &io_buffer_);
    }
};





}}}}
