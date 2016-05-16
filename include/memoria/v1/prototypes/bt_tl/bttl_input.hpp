
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

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace bttl {
namespace iobuf {

namespace {

	template <typename T>
	class InputBufferHandle {
		T* ref_;
	public:
		using MyType = InputBufferHandle<T>;
		using PtrType = T;

		InputBufferHandle(T* ref): ref_(ref) {}
		InputBufferHandle(): ref_(nullptr) {}
		InputBufferHandle(const MyType& other) = delete;
		InputBufferHandle(MyType&& other): ref_(other.ref_) {
			other.ref_ = nullptr;
		}

		~InputBufferHandle() {
			if (ref_) ::free(ref_);
		}

		T* get() {
			return ref_;
		}

		const T* get() const {
			return ref_;
		}

		MyType& operator=(const MyType& other) = delete;

		void operator=(MyType&& other) {
			if (ref_) ::free(ref_);
			ref_ = other.ref_;
			other.ref_ = nullptr;
		}

		T* operator->() {return ref_;}
		const T* operator->() const {return ref_;}

		bool is_null() const {
			return ref_ == nullptr;
		}
	};



	template <typename DataBuffer, typename CtrSizeT>
	struct BTSSStreamInputBufferBase {
		using Buffer    = DataBuffer;
	protected:

		using BufferT 		= typename DataBuffer::PtrType;
		using BufferSizes 	= typename BufferT::BufferSizesT;

		using AppendState	= typename BufferT::AppendState;

		DataBuffer 	buffer_;
		BufferT*   	buffer_ptr_;
		AppendState append_state_;

	public:
		BTSSStreamInputBufferBase()
	{}

		void init(DataBuffer&& buffer)
		{
			buffer_ 	= std::move(buffer);
			buffer_ptr_ = buffer_.get();
		}

		void init(Int capacity)
		{
			init(create_input_buffer(capacity));
		}


		DataBuffer& buffer() {return buffer_;}
		const DataBuffer& buffer() const {return buffer_;}


		void reset()
		{
			if (!buffer_.is_null())
			{
				buffer_->reset();
				append_state_ =  buffer_->append_state();
			}
		}







		BufferSizes data_capacity() const {
			return buffer_->data_capacity();
		}

		auto* create_input_buffer(const BufferSizes& buffer_sizes)
		{
			Int block_size = BufferT::block_size(buffer_sizes);
			BufferT* buffer = T2T<BufferT*>(malloc(block_size));
			if (buffer)
			{
				buffer->setTopLevelAllocator();
				buffer->init(block_size, buffer_sizes);
				return buffer;
			}
			else {
				throw OOMException(MA_RAW_SRC);
			}
		}

		auto* create_input_buffer(Int buffer_size)
		{
			Int block_size = BufferT::block_size(buffer_size) + 500;
			BufferT* buffer = T2T<BufferT*>(malloc(block_size));
			if (buffer)
			{
				buffer->setTopLevelAllocator();
				buffer->init(block_size, buffer_size);
				return buffer;
			}
			else {
				throw OOMException(MA_RAW_SRC);
			}
		}


	protected:

		void enlarge()
		{
			BufferSizes current_capacity 	= data_capacity();
			BufferSizes new_capacity 		= current_capacity;
			VectorAdd(new_capacity, new_capacity);

			auto new_buffer = create_input_buffer(new_capacity);

			buffer_.get()->copyTo(new_buffer);

			init(DataBuffer(new_buffer));
		}
	};





	template <typename DataBuffer, typename CtrSizeT, bool LastStream>
	struct BTSSStreamInputBuffer: BTSSStreamInputBufferBase<DataBuffer, CtrSizeT> {
		using Base = BTSSStreamInputBufferBase<DataBuffer, CtrSizeT>;

		using BufferT       = typename DataBuffer::PtrType;

		using SizesBuffer   = std::vector<CtrSizeT>;

		using AppendState	= typename BufferT::AppendState;

	private:

		SizesBuffer sizes_buffer_;
		size_t eb_head_ = 0;

		AppendState append_state_;

	public:
		BTSSStreamInputBuffer() {}



		void reset()
		{
			Base::reset();

			for (size_t c = 0; c < sizes_buffer_.size(); c++)
			{
				sizes_buffer_[c] = 0;
			}

			eb_head_ = 0;
		}

		void finish()
		{
			if (eb_head_ > 0)
			{
				append_sizes_substream();
				this->buffer()->reindex();
			}
			else {
				this->buffer()->reset();
			}
		}


		void add_size(Int idx, CtrSizeT value)
		{
			sizes_buffer_[idx] += value;
		}


		template <typename IOBuffer>
		void append_stream_entries(Int entries, IOBuffer& buffer)
		{
			for (Int c = 0; c < entries; c++)
			{
				this->append_io_entry(buffer);

				if (eb_head_ < sizes_buffer_.size())
				{
					sizes_buffer_[eb_head_] = 0;
				}
				else {
					sizes_buffer_.emplace_back(0);
				}

				eb_head_++;
			}
		}

	private:


		template <typename IOBuffer>
		void append_io_entry(IOBuffer& io_buffer, Int enlargements = 0)
		{
			size_t pos = io_buffer.pos();

			auto tmp = append_state_;

			if (this->buffer_ptr_->append_bttl_entry_from_iobuffer(append_state_, io_buffer))
			{
				return;
			}
			else {
				append_state_ = tmp;
				io_buffer.pos(pos);

				if (enlargements < 5)
				{
					this->enlarge();
					append_io_entry(io_buffer, enlargements + 1);
				}
				else {
					throw Exception(MA_RAW_SRC, "Supplied entry is too large for InputBuffer");
				}
			}
		}

		void append_sizes_substream()
		{
			Int tries = 0;
			while (tries < 5)
			{
				if (this->buffer()->has_bttl_capacity_for(sizes_buffer_, 0, eb_head_))
				{
					this->buffer()->append_last_substream(sizes_buffer_, 0, eb_head_);
					return;
				}
				else if (tries < 5)
				{
					this->enlarge();
				}
				else {
					throw Exception(MA_RAW_SRC, "Supplied stream sizes buffer is too large for InputBuffer");
				}
			}
		}
	};


	template <typename DataBuffer, typename CtrSizeT>
	struct BTSSStreamInputBuffer<DataBuffer, CtrSizeT, true>: BTSSStreamInputBufferBase<DataBuffer, CtrSizeT> {
		void finish()
		{
			this->buffer()->reindex();
		}


		template <typename IOBuffer>
		void append_stream_entries(Int entries, IOBuffer& buffer)
		{
			for (Int c = 0; c < entries; c++)
			{
				this->append_io_entry(buffer);
			}
		}


		template <typename IOBuffer>
		void append_io_entry(IOBuffer& io_buffer, Int enlargements = 0)
		{
			size_t pos = io_buffer.pos();

			auto tmp = this->append_state_;

			if (this->buffer_ptr_->append_entry_from_iobuffer(this->append_state_, io_buffer))
			{
				return;
			}
			else{
				this->append_state_ = tmp;
				io_buffer.pos(pos);

				if (enlargements < 5)
				{
					this->enlarge();
					append_io_entry(io_buffer, enlargements + 1);
				}
				else {
					throw Exception(MA_RAW_SRC, "Supplied entry is too large for InputBuffer");
				}
			}
		}
	};






	template <typename Types, Int Streams, Int Idx = 0>
	struct InputBufferBuilder {
		using InputBuffer = StreamInputBuffer<
				Idx,
				typename Types::template StreamInputBufferStructList<Idx>
		>;

		using UInputBufferPtr 	= InputBufferHandle<InputBuffer>;
		using SizedBuffer 		= BTSSStreamInputBuffer<UInputBufferPtr, typename Types::CtrSizeT, Idx == Streams - 1>;

		using Type = MergeLists<
				SizedBuffer,
				typename InputBufferBuilder<Types, Streams, Idx + 1>::Type
		>;
	};

	template <typename Types, Int Streams>
	struct InputBufferBuilder<Types, Streams, Streams> {
		using Type = TL<>;
	};


	template <typename SeqT>
	class SymbolsHandle {
		FreeUniquePtr<PackedAllocator>  ref_;
	public:
		SymbolsHandle(Int capacity): ref_(allocate(capacity))
	{}

		SeqT* get() {return ref_->template get<SeqT>(0);}
		const SeqT* get() const {return ref_->template get<SeqT>(0);}

		SeqT* operator->() {return this->get();}
		const SeqT* operator->() const {return this->get();}

		void enlarge(Int required)
		{
			Int current_size = get()->size();
			Int new_size = current_size * 2;

			Int new_capacity = new_size - current_size;
			if (new_capacity < required)
			{
				new_size += required - new_capacity;
			}

			auto new_ptr = allocate(new_size);

			get()->splitTo(new_ptr->template get<SeqT>(0), 0);

			ref_.reset(new_ptr.release());
		}

	private:
		static auto allocate(Int capacity)
		{
			Int block_size = SeqT::packed_block_size(capacity);
			auto ptr = AllocTool<PackedAllocator>::create(block_size, 1);

			SeqT* seq = ptr->template allocate<SeqT>(0, SeqT::packed_block_size(capacity));

			seq->enlargeData(capacity);

			return ptr;
		}
	};

}




template <typename CtrT>
class AbstractCtrInputProviderBase {

protected:
    static const Int Streams = CtrT::Types::Streams;

public:
    using MyType = AbstractCtrInputProviderBase<CtrT>;

    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Iterator  = typename CtrT::Iterator;

    using Buffer = AsTuple<
            typename InputBufferBuilder<
                typename CtrT::Types,
                Streams
            >::Type
    >;

    using Position  = typename CtrT::Types::Position;
    using CtrSizesT = typename CtrT::Types::CtrSizesT;

    using ForAllBuffer = ForAllTuple<std::tuple_size<Buffer>::value>;

    using NodePair = std::pair<NodeBaseG, NodeBaseG>;

    using AnchorValueT  = core::StaticVector<CtrSizeT, Streams - 1>;
    using AnchorPosT    = core::StaticVector<Int, Streams - 1>;
    using AnchorNodeT   = core::StaticVector<NodeBaseG, Streams - 1>;

    using Sequence = PkdFSSeq<typename PkdFSSeqTF<NumberOfBits(Streams)>::Type>;

protected:

    Buffer buffer_;
    CtrSizesT start_;
    CtrSizesT size_;
    bool finished_ = false;


    SymbolsHandle<Sequence> symbols_;

    CtrT& ctr_;

    AnchorPosT   anchors_;
    AnchorValueT anchor_values_;
    AnchorNodeT  leafs_;


    Int last_stream_;

    CtrSizeT orphan_splits_ = 0;

    NodePair split_watcher_;

    Int start_stream_;

    CtrSizeT total_symbols_ = 0;

    CtrSizesT totals_;
    CtrSizesT locals_; // FIXME: remove?



private:
    struct CreateBufferFn {
        template <Int Idx, typename Buffer>
        void process(Buffer&& buffer, Int initial_capacity)
        {
            buffer.init(initial_capacity);
        }
    };


    struct FinishBufferFn {
        template <Int Idx, typename Buffer>
        void process(Buffer&& buffer)
        {
            buffer.finish();
        }
    };

    struct DumpBufferFn {
        template <Int Idx, typename Buffer, typename EventConsumer>
        void process(Buffer&& buffer, EventConsumer&& consumer, std::ostream& out)
        {
            out<<"Begin Stream Dump: "<<Idx<<std::endl;
            buffer.buffer()->generateDataEvents(&consumer);
            out<<"End Stream Dump: "<<Idx<<std::endl<<std::endl<<std::endl<<std::endl;
        }
    };

    struct ResetBufferFn {
        template <Int Idx, typename Buffer>
        void process(Buffer&& buffer)
        {
            buffer.reset();
        }
    };

public:

    AbstractCtrInputProviderBase(CtrT& ctr, Int start_level, Int initial_capacity):
        symbols_(initial_capacity),
        ctr_(ctr),
        last_stream_(start_level - 1),
        start_stream_(start_level)
    {
        ForAllBuffer::process(buffer_, CreateBufferFn(), initial_capacity);
    }
private:
    static Int block_size(Int capacity)
    {
        return Sequence::packed_block_size(capacity);
    }

protected:
    void finish_buffer()
    {
        ForAllBuffer::process(buffer_, FinishBufferFn());
    }

    void reset_buffer()
    {
        ForAllBuffer::process(buffer_, ResetBufferFn());
    }

public:

    template <typename Iterator>
    void prepare(Iterator iter, const Position& start)
    {
        auto stream = iter.stream();

        for (Int s = stream; s > 0; s--)
        {
            iter.toIndex();

            auto ss = iter.stream();
            this->leafs_[ss]        = iter.leaf();
            this->anchors_[ss]      = iter.idx();
        }
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

    const CtrSizesT& totals() const {
        return totals_;
    }

    const CtrSizesT& locals() const {
        return locals_;
    }

    Int last_symbol() const {
        return last_stream_;
    }


    virtual bool hasData()
    {
        bool buffer_has_data = start_.sum() < size_.sum();

        return buffer_has_data || populate_buffer();
    }

    virtual Position fill(NodeBaseG& leaf, const Position& start)
    {
        Position pos = start;

        while(true)
        {
            auto buffer_sizes = this->buffer_size();

            if (buffer_sizes.sum() == 0)
            {
                if (!this->populate_buffer())
                {
                    return pos;
                }
                else {
                    buffer_sizes = this->buffer_size();
                }
            }

            auto capacity = this->findCapacity(leaf, buffer_sizes);

            if (capacity.sum() > 0)
            {
                insertBuffer(leaf, pos, capacity);

                auto rest = this->buffer_size();

                if (rest.sum() > 0)
                {
                    return pos + capacity;
                }
                else {
                    pos += capacity;
                }
            }
            else {
                return pos;
            }
        }
    }

    void nextLeaf(const NodeBaseG& leaf) {}

    virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes) = 0;


    struct InsertBufferFn {

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename StreamObj>
        bool stream(StreamObj* stream, PackedAllocator* alloc, const Position& at, const Position& starts, const Position& sizes, const Buffer& buffer)
        {
            static_assert(StreamIdx < Position::Indexes, "");
            static_assert(StreamIdx < std::tuple_size<Buffer>::value, "");

            stream->insert_buffer(
                    at[StreamIdx],
                    std::get<StreamIdx>(buffer).buffer()->template substream_by_idx<Idx>(),
                    starts[StreamIdx],
                    sizes[StreamIdx]
            );

            return true;
        }

        template <typename NodeTypes, typename... Args>
        auto treeNode(LeafNode<NodeTypes>* leaf, Args&&... args)
        {
            leaf->layout(255);
            return leaf->processSubstreamGroups(*this, leaf->allocator(), std::forward<Args>(args)...);
        }
    };




    virtual void insertBuffer(NodeBaseG& leaf, const Position& at, const Position& sizes)
    {
        CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, InsertBufferFn(), at, start_, sizes, buffer_);

        if (leaf->parent_id().isSet())
        {
            ctr().update_path(leaf);
        }

        updateLeafAnchors(leaf, at, sizes);

        start_ += sizes;
    }

    const Buffer& buffer() const {
        return buffer_;
    }

    Position buffer_size() const
    {
        return size_ - start_;
    }


    Position rank(CtrSizeT idx) const
    {
        Position rnk;

        Int start_pos = start_.sum();

        const Sequence* symbols = this->symbols();

        for (Int s = 0; s < Streams; s++)
        {
            rnk[s] = symbols->rank(idx + start_pos, s);
        }

        return rnk - start_;
    }

    Position rank() const {
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
        ForAllBuffer::process(buffer_, DumpBufferFn(), dumper, out);

        out<<"Begin Symbols"<<std::endl;
        symbols()->generateDataEvents(&dumper);
        out<<"End Symbols"<<std::endl<<std::endl<<std::endl<<std::endl;
    }

protected:
    auto* symbols() {
        return symbols_.get();
    }

    const auto* symbols() const {
        return symbols_.get();
    }

protected:


    struct AddSizeFn {
        template <Int SStreamIdx, typename Buffer, typename T1, typename T2>
        void process(Buffer&& buffer, T1 idx, T2 value)
        {
            std::get<SStreamIdx>(buffer).add_size(idx, value);
        }
    };


    void finish_stream_run(Int symbol, Int last_symbol, const Position& sizes, Position& buffer_sums)
    {
        for (Int sym = last_symbol; sym > symbol; sym--)
        {
            if (sizes[sym - 1] > 0)
            {
                if (buffer_sums[sym] > 0)
                {
                    ForEachStream<Streams - 2>::process(sym - 1, AddSizeFn(), buffer_, sizes[sym - 1] - 1, buffer_sums[sym]);
                }
            }
            else if (leafs_[sym - 1].isSet())
            {
                if (buffer_sums[sym] > 0)
                {
                    updateLeaf(sym - 1, anchors_[sym - 1], buffer_sums[sym]);
                }
            }

            buffer_sums[sym] = 0;
        }
    }

    void updateLeafAnchors(const NodeBaseG& leaf, const Position& at, const Position& sizes)
    {
        for (Int c = 0; c < Streams - 1; c++)
        {
            if (sizes[c] > 0)
            {
                anchors_[c] = at[c] + sizes[c] - 1;
                leafs_[c]  = leaf;
            }
        }
    }


protected:

    virtual void updateLeaf(Int sym, CtrSizeT pos, CtrSizeT sum)
    {
        this->ctr_.add_to_stream_counter(leafs_[sym], sym, pos, sum);
    }
};











template <
    typename CtrT,
    Int Streams = CtrT::Types::Streams,
    LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength
>
class AbstractCtrInputProvider;



template <
    typename CtrT,
    Int Streams
>
class AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::FIXED>: public AbstractCtrInputProviderBase<CtrT> {

    using Base = AbstractCtrInputProviderBase<CtrT>;

public:
    using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::FIXED>;

    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Buffer    = typename Base::Buffer;
    using Position  = typename Base::Position;

public:

    AbstractCtrInputProvider(CtrT& ctr, Int start_level, Int total_capacity):
        Base(ctr, start_level, total_capacity)
    {}

    virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes)
    {
        auto size  = sizes.sum();

        if (checkSize(leaf, size))
        {
            return sizes;
        }
        else {
            auto imax           = size;
            decltype(imax) imin = 0;
            auto accepts        = 0;

            Int last = imin;

            while (imax > imin)
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    if (this->checkSize(leaf, mid))
                    {
                        if (accepts++ >= 50)
                        {
                            return this->rank(mid);
                        }
                        else {
                            imin = mid + 1;
                        }

                        last = mid;
                    }
                    else {
                        imax = mid - 1;
                    }
                }
                else {
                    if (this->checkSize(leaf, imax))
                    {
                        return this->rank(imax);
                    }
                    else {
                        return this->rank(last);
                    }
                }
            }

            return this->rank(last);
        }
    }

    bool checkSize(const NodeBaseG& leaf, CtrSizeT target_size)
    {
        auto rank = this->rank(target_size);
        return this->ctr().checkCapacities(leaf, rank);
    }

    bool checkSize(const NodeBaseG& leaf, Position target_size)
    {
        return this->ctr().checkCapacities(leaf, target_size);
    }
};








template <
    typename CtrT,
    Int Streams
>
class AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrInputProviderBase<CtrT> {

    using Base = AbstractCtrInputProviderBase<CtrT>;

    static constexpr float FREE_SPACE_THRESHOLD = 0.01;


public:
    using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;
    using Iterator  = typename CtrT::Iterator;

    using Buffer    = typename Base::Buffer;
    using Position  = typename Base::Position;

    using PageUpdateMgr         = typename CtrT::Types::PageUpdateMgr;
    using LeafPrefixRanks       = typename CtrT::Types::LeafPrefixRanks;

    using LeafExtents           = v1::core::StaticVector<Position, Streams - 1>;

    Position current_extent_;
    LeafExtents leaf_extents_;

    using typename Base::AnchorValueT;
    using typename Base::AnchorPosT;

public:

    AbstractCtrInputProvider(CtrT& ctr, Int start_level, Int total_capacity):
        Base(ctr, start_level, total_capacity)
    {}

    CtrT& ctr() {
        return this->ctr_;
    }


    template <typename Iterator>
    void prepare(Iterator iter, const Position& start)
    {
        current_extent_ = iter.leaf_extent();

        auto stream = iter.stream();

        for (Int s = stream; s > 0; s--)
        {
            iter.toIndex();

            auto ss = iter.stream();
            this->leafs_[ss]        = iter.leaf();
            this->anchors_[ss]      = iter.idx();
            this->leaf_extents_[ss] = iter.leaf_extent();
        }
    }


    virtual Position fill(NodeBaseG& leaf, const Position& start)
    {
        Position pos = start;

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

                auto next_leaf = applyAnchorValues(mgr, leaf, pos, inserted);

                if (next_leaf == leaf)
                {
                    updateLeafAnchors(leaf, pos, inserted);
                    pos += inserted;
                }
                else {
                    auto split_at = ctr().getNodeSizes(leaf);

                    auto start_pos      = pos.sum();
                    auto split_at_pos   = split_at.sum();
                    auto end_pos        = (pos + inserted).sum();

                    if (start_pos >= split_at_pos)
                    {
                        pos -= split_at;

                        updateLeafAnchors(next_leaf, pos, inserted);

                        pos += inserted;

                        current_extent_ += ctr().node_extents(leaf);

                        mgr.remove(leaf);

                        leaf = next_leaf;

                        break;
                    }
                    else if (end_pos < split_at_pos)
                    {
                        MEMORIA_V1_ASSERT_TRUE(!leaf->parent_id().is_null());

                        updateLeafAnchors(leaf, pos, inserted);

                        mgr.checkpoint(leaf);

                        pos += inserted;
                    }
                    else {
                        auto leaf_inserted = split_at - pos;

                        updateLeafAnchors(leaf, pos, leaf_inserted);

                        auto next_leaf_inserted = inserted - leaf_inserted;

                        updateLeafAnchors(next_leaf, Position(0), next_leaf_inserted);

                        pos -= split_at;
                        pos += inserted;

                        current_extent_ += ctr().node_extents(leaf);

                        mgr.remove(leaf);

                        leaf = next_leaf;

                        break;
                    }
                }

                if (!hasFreeSpace(leaf))
                {
                    break;
                }
            }
            else {
                break;
            }
        }

        return pos;
    }

    void nextLeaf(const NodeBaseG& leaf)
    {
        current_extent_ += ctr().node_extents(leaf);
    }

    virtual Position insertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, Position at, Position size)
    {
        if (tryInsertBuffer(mgr, leaf, at, size))
        {
            this->start_ += size;
            return size;
        }
        else {
            auto imax = size.sum();
            decltype(imax) imin  = 0;
            decltype(imax) start = 0;

            Position tmp = at;

            while (imax > imin && hasFreeSpace(leaf))
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    Int try_block_size = mid - start;

                    auto sizes = this->rank(try_block_size);
                    if (tryInsertBuffer(mgr, leaf, at, sizes))
                    {
                        imin = mid + 1;

                        start = mid;
                        at += sizes;
                        this->start_ += sizes;
                    }
                    else {
                        imax = mid - 1;
                    }
                }
                else {
                    auto sizes = this->rank(1);
                    if (tryInsertBuffer(mgr, leaf, at, sizes))
                    {
                        start += 1;
                        at += sizes;
                        this->start_ += sizes;
                    }

                    break;
                }
            }

            return at - tmp;
        }
    }

protected:
    virtual Position findCapacity(const NodeBaseG& leaf, const Position& size) {
        return Position();
    }

    bool tryInsertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, const Position& at, const Position& size)
    {
        try {
            CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, typename Base::InsertBufferFn(), at, this->start_, size, this->buffer_);

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

    virtual void updateLeaf(Int sym, CtrSizeT pos, CtrSizeT sum)
    {
        this->anchor_values_[sym] += sum;
    }

private:

    NodeBaseG applyAnchorValues(PageUpdateMgr& mgr, NodeBaseG current_leaf, const Position& pos, const Position& inserted)
    {
        auto next_leaf = current_leaf;

        for (Int s = 0; s < Streams - 1; s++)
        {
            const auto value = this->anchor_values_[s];

            if (value > 0)
            {
                for (Int i = s + 1; i < Streams - 1; i++)
                {
                    if (this->leafs_[s] != this->leafs_[i])
                    {
                        leaf_extents_[i][s + 1] += value;
                    }
                }

                if (this->leafs_[s] != current_leaf)
                {
                    current_extent_[s + 1] += value;
                }
            }
        }

        Int splits = 0;

        for (Int s = 0; s < Streams - 1; s++)
        {
            auto& leaf = this->leafs_[s];

            if (leaf.isSet())
            {
                if (current_leaf == leaf)
                {
                    auto leaf2 = processCurrentAnchor(s, mgr, pos, inserted);

                    if (leaf2 != leaf)
                    {
                        next_leaf = leaf2;
                    }
                }
                else {
                    processAnchor(mgr, s, splits);
                }
            }
        }

        if (splits > 0) {
            mgr.checkpoint(current_leaf);
        }

        return next_leaf;
    }

    void processAnchor(PageUpdateMgr& main_mgr, Int stream, Int& splits)
    {
        auto& ctr = this->ctr();

        auto& leaf          = this->leafs_[stream];
        auto& anchor_value  = this->anchor_values_[stream];
        auto& anchor        = this->anchors_[stream];

        if (anchor_value > 0)
        {
            MEMORIA_V1_ASSERT_TRUE(leaf);

            PageUpdateMgr mgr(ctr);

            mgr.add(leaf);

            try {
                ctr.add_to_stream_counter(leaf, stream, anchor, anchor_value);
                anchor_value = 0;
            }
            catch (PackedOOMException& ex)
            {
                splits++;
                mgr.rollback();

                auto sizes      = ctr.getNodeSizes(leaf);

                auto anchors        = this->anchors_;
                auto anchor_values  = this->anchor_values_;

                for (Int c = 0; c < Streams - 1; c++) {
                    if (this->leafs_[c].isEmpty() || this->leafs_[c] != leaf)
                    {
                        anchors[c] = -1;
                    }
                }

                auto extent1 = leaf_extents_[stream];

                for (Int c = 0; c < Streams; c++) {
                    if (extent1[c] > sizes[c]) {
                        extent1[c] = sizes[c];
                    }
                }

                auto split_at   = ctr.leafrank_(leaf, sizes, extent1, sizes.sum() / 2, anchors, anchor_values);

                NodeBaseG next_leaf;

                if (leaf->is_root() || leaf->parent_id().isSet())
                {
                    next_leaf = ctr.split_leaf_p(leaf, split_at);
                }
                else {
                    this->orphan_splits_++;

                    auto page_size  = ctr.getRootMetadata().page_size();
                    next_leaf       = ctr.createNode(0, false, true, page_size);

                    ctr.split_leaf_node(leaf, next_leaf, split_at);
                }

                if (this->split_watcher_.first == leaf) {
                    this->split_watcher_.second = next_leaf;
                }

                next_leaf->next_leaf_id()   = leaf->next_leaf_id();
                leaf->next_leaf_id()        = next_leaf->id();


                // FIXME rewrite this stupid code.
                auto current_leaf_id = leaf->id();

                for (Int ss = 0; ss < Streams - 1; ss++)
                {
                    auto& lleaf             = this->leafs_[ss];
                    auto& lanchor           = this->anchors_[ss];

                    if (lleaf.isSet() && lleaf->id() == current_leaf_id)
                    {
                        if (lanchor >= split_at[ss])
                        {
                            lanchor -= split_at[ss];

                            leaf_extents_[ss] += ctr.node_extents(leaf);

                            mgr.remove(leaf);
                            mgr.add(next_leaf);

                            lleaf = next_leaf;
                        }
                    }
                }

                ctr.add_to_stream_counter(leaf, stream, anchor, anchor_value);
                anchor_value = 0;

                main_mgr.checkpoint(leaf);
                main_mgr.checkpoint(next_leaf);
            }
        }
    }


    NodeBaseG processCurrentAnchor(Int stream, PageUpdateMgr& mgr, const Position& pos, const Position& inserted)
    {
        auto& ctr = this->ctr();

        auto& leaf          = this->leafs_[stream];
        auto& anchor_value  = this->anchor_values_[stream];
        auto& anchor        = this->anchors_[stream];

        auto next_leaf = leaf;

        MEMORIA_V1_ASSERT_TRUE(leaf);

        if (anchor_value > 0)
        {
            try {
                ctr.add_to_stream_counter(leaf, stream, anchor, anchor_value);

                mgr.checkpoint(leaf);

                anchor_value = 0;
            }
            catch (PackedOOMException& ex)
            {
                mgr.restoreNodeState();

                auto split_at = pos;

                if (leaf->is_root() || leaf->parent_id().is_set())
                {
                    next_leaf = ctr.split_leaf_p(leaf, split_at);
                }
                else {
                    this->orphan_splits_++;

                    auto page_size  = ctr.getRootMetadata().page_size();
                    next_leaf       = ctr.createNode(0, false, true, page_size);

                    ctr.split_leaf_node(leaf, next_leaf, split_at);

                    next_leaf->next_leaf_id()   = leaf->next_leaf_id();
                    leaf->next_leaf_id()        = next_leaf->id();
                }

                if (this->split_watcher_.first == leaf)
                {
                    this->split_watcher_.second = next_leaf;
                }

                auto node_to_update  = leaf;
                auto current_leaf_id = leaf->id();

                for (Int ss = 0; ss < Streams - 1; ss++)
                {
                    auto& lleaf             = this->leafs_[ss];
                    auto& lanchor           = this->anchors_[ss];

                    if (lleaf.isSet() && lleaf->id() == current_leaf_id) {
                        if (lanchor >= split_at[ss])
                        {
                            lanchor -= split_at[ss];

                            leaf_extents_[ss] += ctr.node_extents(leaf);

                            lleaf = next_leaf;

                            if (ss == stream) {
                                node_to_update = next_leaf;
                            }
                        }
                    }
                }

                ctr.add_to_stream_counter(node_to_update, stream, anchor, anchor_value);
                anchor_value = 0;

                mgr.checkpoint(leaf);
                mgr.checkpoint(next_leaf);
            }
        }

        return next_leaf;
    }



    void updateLeafAnchors(const NodeBaseG& leaf, const Position& at, const Position& sizes)
    {
        MEMORIA_V1_ASSERT_TRUE(current_extent_.gteAll(0));

        for (Int c = 0; c < Streams - 1; c++)
        {
            if (sizes[c] > 0)
            {
                this->anchors_[c]       = at[c] + sizes[c] - 1;

                this->leafs_[c]         = leaf;
                this->anchor_values_[c] = 0;

                this->leaf_extents_[c] = current_extent_;
            }
        }
    }
};




template <
    typename CtrT,
    typename IOBuffer
>
class IOBufferCtrInputProvider: public v1::bttl::iobuf::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {
public:
    using Base      = v1::bttl::iobuf::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

    static constexpr Int Streams = CtrT::Types::Streams;

    using typename Base::Position;
    using typename Base::CtrSizesT;

protected:

    using typename Base::ForAllBuffer;
    using typename Base::Buffer;


    using Base::reset_buffer;
    using Base::finish_buffer;

    using Base::start_;
    using Base::size_;
    using Base::buffer_;
    using Base::symbols_;
    using Base::finished_;
    using Base::last_stream_;
    using Base::start_stream_;
    using Base::total_symbols_;
    using Base::locals_;
    using Base::totals_;


    Int stream_run_remainder_ = 0;

    BufferProducer<IOBuffer>* iobuffer_producer_;
    IOBuffer* io_buffer_;

public:
    IOBufferCtrInputProvider(CtrT& ctr, BufferProducer<IOBuffer>* iobuffer_producer, Int start_level, Int initial_capacity = 4000):
        Base(ctr, start_level, initial_capacity),
		iobuffer_producer_(iobuffer_producer),
		io_buffer_(&iobuffer_producer->buffer())
    {}


    struct AppendStreamEntriesFn {
    	template <Int Idx, typename InputBuffer>
    	void process(InputBuffer& input_buffer, Int stream, Int length, IOBuffer& io_buffer)
    	{
    		if (Idx == stream)
    		{
    			input_buffer.append_stream_entries(length, io_buffer);
    		}
    	}
    };


    void append_stream_entries(Int stream, Int length, IOBuffer& io_buffer)
    {
    	ForAllBuffer::process(buffer_, AppendStreamEntriesFn(), stream, length, io_buffer);
    }

    virtual void do_populate_iobuffer()
    {
    	reset_buffer();

        Position sizes;
        Position buffer_sums;

        start_.clear();
        size_.clear();

        auto symbols 		= symbols_.get();
        auto tools 	 		= symbols->tools();
        auto syms 	 		= symbols->symbols();
        auto* symbols_size 	= &symbols->size();

        *symbols_size = 0;

        Int capacity 		= symbols->capacity();
        Int symbol_pos 		= 0;

        io_buffer_->rewind();
        Int entries = iobuffer_producer_->populate(*io_buffer_);
        io_buffer_->rewind();

        if (entries != 0)
        {
        	if (entries < 0)
        	{
        		finished_ = true;
        		entries = -entries;
        	}

        	for (Int entry_num = 0; entry_num < entries;)
        	{
        		Int stream;
        		Int run_length;
        		bool premature_eob = false;

        		if (stream_run_remainder_ != 0)
        		{
        			stream = last_stream_;

        			if (stream_run_remainder_ <= entries)
        			{
        				run_length 				= stream_run_remainder_;
        				stream_run_remainder_ 	= 0;
        			}
        			else {
        				run_length 				= entries;
        				stream_run_remainder_ 	-= entries;
        				premature_eob 			= true;
        			}
        		}
        		else {
        			auto run = io_buffer_->template getSymbolsRun<Streams>();

        			stream 		= run.symbol();
        			run_length 	= run.length();

        			entry_num++;

        			Int iobuffer_remainder = entries - entry_num;

        			if (run_length >= iobuffer_remainder)
        			{
        				stream_run_remainder_ 	= run_length - iobuffer_remainder;
        				run_length 				-= stream_run_remainder_;
        				premature_eob 			= true;
        			}
        		}



        		if (total_symbols_ == 0 && (stream != start_stream_))
        		{
        			throw Exception(MA_SRC, SBuf()<<"Invalid start stream: "<<stream<<" expected: "<<start_stream_);
        		}
        		else if (stream > last_stream_ + 1)
        		{
        			throw Exception(MA_RAW_SRC, SBuf() << "Invalid sequence state: last_stream=" << last_stream_ << ", stream=" << stream);
        		}


        		if (capacity < run_length)
        		{
        			symbols_.enlarge(run_length);

        			symbols 		= this->symbols();
        			syms 	 		= symbols->symbols();
        			symbols_size 	= &symbols->size();
        			capacity 		= symbols->capacity();
        		}

        		append_stream_entries(stream, run_length, *io_buffer_);

				*symbols_size += run_length;
				for (Int c = symbol_pos; c < symbol_pos + run_length; c++)
				{
					tools.set(syms, c, stream);
				}

				symbol_pos      += run_length;
				total_symbols_  += run_length;
				size_[stream]   += run_length;
				capacity        -= run_length;

				if (!premature_eob)
				{
					if (stream < last_stream_)
					{
						this->finish_stream_run(stream, last_stream_, sizes, buffer_sums);
					}
					else if (stream > last_stream_)
					{
						locals_[stream] = 0;
					}

					buffer_sums[stream] += run_length;
				}
				else {
					// We have reached the end of IOBuffer

					buffer_sums[stream] += run_length;
					this->finish_stream_run(0,  Streams - 1, sizes, buffer_sums);
				}


				sizes[stream]       += run_length;
				totals_[stream]     += run_length;
				locals_[stream]     += run_length;

				last_stream_ = stream;

				entry_num += run_length;
        	}

        	if (finished_)
        	{
        		// We have reached the end of data stream
        		this->finish_stream_run(0,  Streams - 1, sizes, buffer_sums);
        	}

        	symbols->reindex();

        	finish_buffer();
        }
        else {
        	finished_ = true;
        }
    }

};


class RunDescr {
    Int symbol_;
    Int length_;
public:
    RunDescr(): symbol_(), length_() {}

    RunDescr(Int symbol, Int length = 1): symbol_(symbol), length_(length) {}

    Int symbol() const {return symbol_;}
    Int length() const {return length_;}

    void set_length(Int len) {
        length_ = len;
    }
};


template <Int Streams, typename IOBufferT>
class FlatTreeIOBufferAdapter: public BufferProducer<IOBufferT> {

public:

    static constexpr BigInt MaxRunLength 	= IOBufferT::template getMaxSymbolsRunLength<Streams>();

    using CtrSizesT = core::StaticVector<BigInt, Streams>;

    using IOBuffer = IOBufferT;

private:

    RunDescr state_;
    Int processed_ = 0;
    BigInt run_length_ = 0;
    BigInt run_processed_ = 0;
    bool symbol_encoded_ = false;

    CtrSizesT consumed_;

public:
    FlatTreeIOBufferAdapter(){}

    const CtrSizesT& consumed() const {
    	return consumed_;
    }

    virtual RunDescr query() = 0;
    virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length) = 0;

    virtual Int populate(IOBuffer& io_buffer)
    {
    	Int entries = 0;

    	while (true)
    	{
    		if (processed_ == state_.length())
    		{
    			state_ 			= this->query();
    			symbol_encoded_ = false;
    			processed_ 		= 0;
    		}

    		if (state_.symbol() >= 0)
    		{
    			auto length = state_.length();

    			while (processed_ < length || length == 0)
    			{
    				Int remainder = length - processed_;

    				if (run_processed_ == run_length_)
    				{
    					run_length_ = remainder > MaxRunLength ? MaxRunLength : remainder;
    					run_processed_  = 0;
    					symbol_encoded_ = false;
    				}

    				Int to_encode = run_length_ - run_processed_;

    				if (!symbol_encoded_)
    				{
    					auto pos = io_buffer.pos();
    					if (io_buffer.template putSymbolsRun<Streams>(state_.symbol(), to_encode))
    					{
    						symbol_encoded_ = true;
    						entries++;
    					}
    					else {
    						io_buffer.pos(pos);
    						symbol_encoded_ = false;
    						return entries;
    					}
    				}

    				if (to_encode > 0)
    				{
    					Int actual = populate_stream(state_.symbol(), io_buffer, to_encode);

    					processed_ 		+= actual;
    					run_processed_ 	+= actual;
    					entries 		+= actual;

    					consumed_[state_.symbol()] += actual;

    					if (actual < to_encode)
    					{
    						return entries;
    					}
    					else {
    						symbol_encoded_ = false;
    					}
    				}
    			}
    		}
    		else {
    			return -entries;
    		}
    	}

    	return entries;
    }
};







namespace {

    template <Int Size, Int Idx = 0>
    struct StreamSizeAdapter {
        template <typename T, typename... Args>
        static auto process(Int stream, T&& object, Args&&... args)
        {
            if (stream == Idx)
            {
                return object.stream_size(StreamTag<Idx>(), std::forward<Args>(args)...);
            }
            else {
                return StreamSizeAdapter<Size, Idx+1>::process(stream, std::forward<T>(object), std::forward<Args>(args)...);
            }
        }
    };


    template <Int Size>
    struct StreamSizeAdapter<Size, Size> {
        template <typename T, typename... Args>
        static auto process(Int stream, T&& object, Args&&... args)
        {
            if (stream == Size)
            {
                return object.stream_size(StreamTag<Size>(), std::forward<Args>(args)...);
            }
            else {
                throw Exception(MA_RAW_SRC, SBuf() << "Invalid stream number: " << stream);
            }
        }
    };
}






template <typename MyType, Int Streams>
class FlatTreeStructureGeneratorBase {

public:
	using CtrSizeT = BigInt;

    using CtrSizesT = core::StaticVector<CtrSizeT, Streams>;

private:

    CtrSizesT counts_;
    CtrSizesT current_limits_;
    CtrSizesT totals_;

    Int level_;

    template <Int, Int> friend struct StreamSizeAdapter;

public:
    FlatTreeStructureGeneratorBase(Int level = 0):
        level_(level)
    {}

    auto query()
    {
        if (counts_[level_] < current_limits_[level_])
        {
            if (level_ < Streams - 1)
            {
                level_++;

                counts_[level_] = 0;
                current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);

                return bttl::iobuf::RunDescr(level_ - 1, 1);
            }
            else {
                return bttl::iobuf::RunDescr(level_, current_limits_[level_] - counts_[level_]);
            }
        }
        else if (level_ > 0)
        {
            level_--;
            return this->query();
        }
        else {
            return bttl::iobuf::RunDescr(-1);
        }
    }

    const auto& consumed() const {
        return totals_;
    }

    auto& consumed() {
    	return totals_;
    }

    const auto& counts() const {
    	return counts_;
    }

    auto& counts() {
    	return counts_;
    }

    void commit(Int level, CtrSizeT len)
    {
    	counts_[level] += len;
    }

    const auto& current_limits() const {
    	return current_limits_;
    }

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<MyType*>(this);}

    void init() {
        current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);
    }

private:

    auto stream_size(StreamTag<0>, const CtrSizesT& pos)
    {
        return self().prepare(StreamTag<0>());
    }

    template <Int StreamIdx>
    auto stream_size(StreamTag<StreamIdx>, const CtrSizesT& pos)
    {
        return self().prepare(StreamTag<StreamIdx>(), pos);
    }
};


}}}}
