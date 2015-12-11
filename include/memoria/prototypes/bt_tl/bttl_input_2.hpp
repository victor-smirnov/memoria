
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_PROTOTYPES_BT_TL_INPUT_2_HPP_
#define MEMORIA_PROTOTYPES_BT_TL_INPUT_2_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/prototypes/bt_tl/bttl_input.hpp>

#include <memory>

namespace memoria 	{
namespace bttl 		{

namespace detail {



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
	using Buffer 	= DataBuffer;
private:

	DataBuffer buffer_;

public:
	BTSSStreamInputBufferBase()
	{}

	void init(DataBuffer&& buffer)
	{
		buffer_ = std::move(buffer);
	}


	DataBuffer& buffer() {return buffer_;}
	const DataBuffer& buffer() const {return buffer_;}


	void reset() {
		if (!buffer_.is_null()) {
			buffer_->reset();
		}
	}
};



template <typename StreamEntry, typename DataBuffer, typename CtrSizeT, bool LastStream>
struct BTSSStreamInputBuffer: BTSSStreamInputBufferBase<DataBuffer, CtrSizeT> {
	using Base = BTSSStreamInputBufferBase<DataBuffer, CtrSizeT>;

	using BufferT = typename DataBuffer::PtrType;
	using EntryT = StreamEntry;

	using Sizes = std::vector<CtrSizeT>;
	using EntryBuffer = std::vector<EntryT>;

private:
	Sizes sizes_;

	EntryBuffer entry_buffer_;
	size_t eb_head_ = 0;

public:
	BTSSStreamInputBuffer() {}

	void init(DataBuffer&& buffer, Int capacity)
	{
		Base::init(std::move(buffer), capacity);

		sizes_.resize(capacity);

		for (auto& v: sizes_) {v = 0;}
	}


	void init(DataBuffer&& buffer)
	{
		Base::init(std::move(buffer));
	}

	Sizes& sizes() {return sizes_;}
	const Sizes& sizes() const {return sizes_;}

	void reset()
	{
		Base::reset();
		eb_head_ = 0;
	}

	void finish()
	{
		auto buffer_sizes = BufferT::compute_buffer_sizes_for(eb_head_, entry_buffer_);

		if (this->buffer().is_null() || !this->buffer()->has_capacity_for(buffer_sizes))
		{
			Int block_size = BufferT::block_size(buffer_sizes);
			BufferT* buffer = T2T<BufferT*>(malloc(block_size));
			if (buffer)
			{
				buffer->setTopLevelAllocator();
				buffer->init(block_size, buffer_sizes);
				this->init(DataBuffer(buffer));
			}
			else {
				throw OOMException(MA_SRC);
			}
		}

		this->buffer()->append(entry_buffer_, 0, eb_head_);
		this->buffer()->reindex();
	}

	void add_size(Int idx, CtrSizeT value)
	{
		constexpr Int SizeIdx = std::tuple_size<EntryT>::value - 1;
		std::get<SizeIdx>(entry_buffer_[idx]) += value;
	}


	void append_entry(const EntryT& entry)
	{
		if (entry_buffer_.size() == eb_head_)
		{
			entry_buffer_.resize(entry_buffer_.size() + 1, entry);
		}
		else {
			entry_buffer_[eb_head_] = entry;
		}

		eb_head_++;
	}

	Int size() const {
		return eb_head_;
	}
};


template <typename StreamEntry, typename DataBuffer, typename CtrSizeT>
struct BTSSStreamInputBuffer<StreamEntry, DataBuffer, CtrSizeT, true>: BTSSStreamInputBufferBase<DataBuffer, CtrSizeT> {
	void finish() {
		this->buffer()->reindex();
	}
};






template <typename Types, Int Streams, Int Idx = 0>
struct InputBufferBuilder2 {
	using InputBuffer = StreamInputBuffer<
		typename Types::template StreamInputBufferStructList<Idx>
	>;

	using StreamEntry = typename Types::template StreamInputTuple<Idx>;

	using UInputBufferPtr = InputBufferHandle<InputBuffer>;

	using SizedBuffer = BTSSStreamInputBuffer<StreamEntry, UInputBufferPtr, typename Types::CtrSizeT, Idx == Streams - 1>;

	using Type = MergeLists<
			SizedBuffer,
			typename InputBufferBuilder2<Types, Streams, Idx + 1>::Type
	>;
};

template <typename Types, Int Streams>
struct InputBufferBuilder2<Types, Streams, Streams> {
	using Type = TL<>;
};





}


//class RunDescr {
//	Int symbol_;
//	Int length_;
//public:
//	RunDescr(Int symbol, Int length = 1): symbol_(symbol), length_(length) {}
//
//	Int symbol() const {return symbol_;}
//	Int length() const {return length_;}
//
//	void set_length(Int len) {
//		length_ = len;
//	}
//};



template <
	typename CtrT
>
class AbstractCtrInputProviderBase2 {

protected:
	static const Int Streams = CtrT::Types::Streams;

public:
	using MyType = AbstractCtrInputProviderBase<CtrT>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;

	using Iterator  = typename CtrT::Iterator;

	using Buffer = AsTuple<
			typename detail::InputBufferBuilder2<
				typename CtrT::Types,
				Streams
			>::Type
	>;

	using Position	= typename CtrT::Types::Position;

	using ForAllBuffer = detail::ForAllTuple<std::tuple_size<Buffer>::value>;

	static constexpr Int get_symbols_number(Int v) {
		return v == 1 ?  0 : (v == 2 ? 1 : (v == 3 ? 2 : (v == 4 ? 2 : (v == 5 ? 3 : (v == 6 ? 3 : (v == 7 ? 3 : 4))))));
	}

	using Symbols = PkdFSSeq<typename PkdFSSeqTF<get_symbols_number(Streams)>::Type>;

	using NodePair = std::pair<NodeBaseG, NodeBaseG>;

	using AnchorValueT 	= core::StaticVector<CtrSizeT, Streams - 1>;
	using AnchorPosT 	= core::StaticVector<Int, Streams - 1>;
	using AnchorNodeT 	= core::StaticVector<NodeBaseG, Streams - 1>;


protected:
	Buffer buffer_;
	Position start_;
	Position size_;

	Int total_capacity_;

	FreeUniquePtr<PackedAllocator> allocator_;

	CtrT& 	ctr_;

	AnchorPosT 	 anchors_;
	AnchorValueT anchor_values_;
	AnchorNodeT	 leafs_;

	Int last_symbol_;

	CtrSizeT orphan_splits_ = 0;

	NodePair split_watcher_;

	Int start_level_;

	CtrSizeT total_symbols_ = 0;

	Position totals_;

private:
	struct CreateBufferFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, Int total_capacity)
		{
			using SizesBufferHandleType 	= typename std::remove_reference<Buffer>::type;
			using BufferHandleType 			= typename SizesBufferHandleType::Buffer;
			using BufferType 	  			= typename BufferHandleType::PtrType;

			if (Idx == Streams - 1)
			{
				Int object_size = BufferType::block_size(total_capacity);

				void* ptr = malloc(object_size);

				if (ptr != nullptr)
				{
					BufferType* pbuffer = T2T<BufferType*>(ptr);

					pbuffer->setTopLevelAllocator();

					pbuffer->init(object_size, total_capacity);

					buffer.init(BufferHandleType(pbuffer));
				}
				else {
					throw vapi::OOMException(MA_SRC);
				}
			}
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

	AbstractCtrInputProviderBase2(CtrT& ctr, Int start_level, Int total_capacity):
		total_capacity_(total_capacity),
		allocator_(AllocTool<PackedAllocator>::create(block_size(total_capacity), 1)),
		ctr_(ctr),
		last_symbol_(start_level - 1),
		start_level_(start_level)
	{
		ForAllBuffer::process(buffer_, CreateBufferFn(), total_capacity);

		allocator_->template allocate<Symbols>(0, Symbols::packed_block_size(total_capacity));

		symbols()->enlargeData(total_capacity);
	}
private:
	static Int block_size(Int capacity)
	{
		return Symbols::packed_block_size(capacity);
	}

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
			this->leafs_[ss]  		= iter.leaf();
			this->anchors_[ss] 		= iter.idx();
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

	const Position& totals() const {
		return totals_;
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

		auto end = at + sizes;

		if (leaf->parent_id().isSet())
		{
			auto sums = ctr().sums(leaf, at, end);
			ctr().update_parent(leaf, sums);
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


//	Position buffer_capacity() const
//	{
//		Position sizes;
//		ForAllBuffer::process(buffer_, BufferCapacityFn(), sizes);
//		return sizes;
//	}


	Position rank(CtrSizeT idx) const
	{
		Position rnk;

		Int start_pos = start_.sum();

		auto symbols = this->symbols();

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
		reset_buffer();

		Position sizes;
		Position buffer_sums;

		start_.clear();
		size_.clear();

		Int capacity = total_capacity_;

		Int symbol_pos = 0;

		auto symbols = this->symbols();

		auto& symbols_size = symbols->size();

		symbols_size = 0;

		auto syms = symbols->symbols();

		while (true)
		{
			RunDescr run = populate(capacity);

			Int symbol = run.symbol();
			Int length = run.length();

			if (symbol >= 0)
			{
				if (length > 0)
				{
					if (total_symbols_ == 0 && (symbol != start_level_))
					{
						throw vapi::Exception(MA_SRC, SBuf()<<"Invalid start symbol: "<<symbol<<" expected: "<<start_level_);
					}

					symbols_size += length;
					for (Int c = symbol_pos; c < symbol_pos + length; c++)
					{
						symbols->tools().set(syms, c, symbol);
					}

					symbol_pos 		+= length;
					total_symbols_ 	+= length;
					size_[symbol] 	+= length;
					capacity 		-= length;

					if (symbol > last_symbol_ + 1)
					{
						throw Exception(MA_SRC, SBuf()<<"Invalid sequence state: last_symbol="<<last_symbol_<<", symbol="<<symbol);
					}
					else if (symbol < last_symbol_)
					{
						this->finish_stream_run(symbol, last_symbol_, sizes, buffer_sums);
					}

					buffer_sums[symbol] += length;
					sizes[symbol] 		+= length;
					totals_[symbol] 	+= length;

					if (capacity <= 0)
					{
						this->finish_stream_run(0, Streams - 1, sizes, buffer_sums);
						last_symbol_ = symbol;
						break;
					}
					else {
						last_symbol_ = symbol;
					}
				}
				else {
					// buffer is full
					cout<<"Buffer is full: "<<size_<<endl;
					this->finish_stream_run(0, Streams - 1, sizes, buffer_sums);
					break;
				}
			}
			else {
				this->finish_stream_run(0, Streams - 1, sizes, buffer_sums);
				last_symbol_ = -1;
				break;
			}
		}

		symbols->reindex();

		finish_buffer();

		return symbol_pos > 0;
	}

	virtual RunDescr populate(Int capacity) = 0;


	void dump_buffer(std::ostream& out = std::cout) const
	{
		TextPageDumper dumper(out);
		ForAllBuffer::process(buffer_, DumpBufferFn(), dumper, out);

		out<<"Begin Symbols"<<std::endl;
		symbols()->generateDataEvents(&dumper);
		out<<"End Symbols"<<std::endl<<std::endl<<std::endl<<std::endl;
	}

protected:
	Symbols* symbols() {
		return allocator_->get<Symbols>(0);
	}

	const Symbols* symbols() const {
		return allocator_->get<Symbols>(0);
	}

private:


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
	Int Streams,
	LeafDataLengthType LeafDataLength
>
class AbstractCtrInputProvider2;



template <
	typename CtrT,
	Int Streams
>
class AbstractCtrInputProvider2<CtrT, Streams, LeafDataLengthType::FIXED>: public AbstractCtrInputProviderBase2<CtrT> {

	using Base = AbstractCtrInputProviderBase<CtrT>;

public:
	using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::FIXED>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;

	using Buffer 	= typename Base::Buffer;
	using Position	= typename Base::Position;

public:

	AbstractCtrInputProvider2(CtrT& ctr, Int start_level, const Position& capacity):
		Base(ctr, start_level, capacity)
	{}

	virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes)
	{
		auto size  = sizes.sum();

		if (checkSize(leaf, size))
		{
			return sizes;
		}
		else {
			auto imax 			= size;
			decltype(imax) imin = 0;
			auto accepts 		= 0;

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
class AbstractCtrInputProvider2<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrInputProviderBase2<CtrT> {

	using Base = AbstractCtrInputProviderBase2<CtrT>;

	static constexpr float FREE_SPACE_THRESHOLD = 0.01;


public:
	using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;
	using Iterator 	= typename CtrT::Iterator;

	using Buffer 	= typename Base::Buffer;
	using Position	= typename Base::Position;

	using PageUpdateMgr			= typename CtrT::Types::PageUpdateMgr;
	using LeafPrefixRanks		= typename CtrT::Types::LeafPrefixRanks;

	using LeafExtents 			= memoria::core::StaticVector<Position, Streams - 1>;

	Position current_extent_;
	LeafExtents leaf_extents_;

public:

	AbstractCtrInputProvider2(CtrT& ctr, Int start_level, Int total_capacity):
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
			this->leafs_[ss]  		= iter.leaf();
			this->anchors_[ss] 		= iter.idx();
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

				auto end = pos + inserted;

				if (leaf->parent_id().isSet())
				{
					auto sums = ctr().sums(leaf, pos, end);
					ctr().update_parent(leaf, sums);
				}

				auto next_leaf = applyAnchorValues(mgr, leaf);

				if (next_leaf == leaf)
				{
					updateLeafAnchors(leaf, pos, inserted);
					pos += inserted;
				}
				else {
					auto split_at = ctr().getNodeSizes(leaf);

					auto start_pos 		= pos.sum();
					auto split_at_pos 	= split_at.sum();
					auto end_pos 		= (pos + inserted).sum();

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
					else if (end_pos <= start_pos)
					{
						MEMORIA_ASSERT_TRUE(leaf->parent_id());

						pos -= split_at;

						updateLeafAnchors(leaf, pos, inserted);

						mgr.checkpoint(leaf);

						pos += inserted;
					}
					else {
						auto leaf_inserted = split_at - pos;

						updateLeafAnchors(leaf, pos, leaf_inserted);

						auto next_leaf_inserted = pos + inserted - split_at;

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

	NodeBaseG applyAnchorValues(PageUpdateMgr& mgr, NodeBaseG& current_leaf)
	{
		auto next_leaf = current_leaf;

		for (Int s = 0; s < Streams - 1; s++)
		{
			auto value = this->anchor_values_[s];

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

		for (Int s = 0; s < Streams - 1; s++)
		{
			auto& leaf = this->leafs_[s];

			if (leaf.isSet())
			{
				if (current_leaf == leaf)
				{
					next_leaf = processCurrentAnchor(s, mgr);
				}
				else {
					processAnchor(s);
				}
			}
		}

		return next_leaf;
	}

	void processAnchor(Int stream)
	{
		auto& ctr = this->ctr();

		auto& leaf 			= this->leafs_[stream];
		auto& anchor_value 	= this->anchor_values_[stream];
		auto& anchor		= this->anchors_[stream];

		if (anchor_value > 0)
		{
			MEMORIA_ASSERT_TRUE(leaf);

			PageUpdateMgr mgr(ctr);

			mgr.add(leaf);

			try {
				ctr.add_to_stream_counter(leaf, stream, anchor, anchor_value);
				anchor_value = 0;
			}
			catch (PackedOOMException& ex)
			{
				mgr.rollback();

				auto sizes		= ctr.getNodeSizes(leaf);
				auto split_at 	= ctr.leafrank_(leaf, sizes, leaf_extents_[stream], sizes.sum() / 2);

				NodeBaseG next_leaf;

				if (leaf->is_root() || leaf->parent_id().isSet())
				{
					next_leaf = ctr.split_leaf_p(leaf, split_at);
				}
				else {
					this->orphan_splits_++;

					auto page_size 	= ctr.getRootMetadata().page_size();
					next_leaf 		= ctr.createNode1(0, false, true, page_size);

					ctr.split_leaf_node(leaf, next_leaf, split_at);
				}

				if (this->split_watcher_.first == leaf) {
					this->split_watcher_.second = next_leaf;
				}

				next_leaf->next_leaf_id() 	= leaf->next_leaf_id();
				leaf->next_leaf_id()		= next_leaf->id();

				for (Int ss = 0; ss < Streams - 1; ss++)
				{
					auto& lleaf 			= this->leafs_[ss];
					auto& lanchor			= this->anchors_[ss];

					if (lleaf == leaf) {
						if (lanchor >= split_at[ss])
						{
							lanchor -= split_at[ss];

							leaf_extents_[ss] += ctr.node_extents(leaf);

							lleaf = next_leaf;
						}
					}
				}

				ctr.add_to_stream_counter(leaf, stream, anchor, anchor_value);
				anchor_value = 0;
			}
		}
	}


	NodeBaseG processCurrentAnchor(Int stream, PageUpdateMgr& mgr)
	{
		auto& ctr = this->ctr();

		auto& leaf 			= this->leafs_[stream];
		auto& anchor_value 	= this->anchor_values_[stream];
		auto& anchor		= this->anchors_[stream];

		auto next_leaf = leaf;

		MEMORIA_ASSERT_TRUE(leaf);

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

				auto sizes		= ctr.getNodeSizes(leaf);
				auto split_at 	= ctr.leafrank_(leaf, sizes, leaf_extents_[stream], sizes.sum() / 2);

				if (leaf->is_root() || leaf->parent_id())
				{
					next_leaf = ctr.split_leaf_p(leaf, split_at);
				}
				else {
					this->orphan_splits_++;

					auto page_size 	= ctr.getRootMetadata().page_size();
					next_leaf 		= ctr.createNode1(0, false, true, page_size);

					ctr.split_leaf_node(leaf, next_leaf, split_at);

					next_leaf->next_leaf_id() 	= leaf->next_leaf_id();
					leaf->next_leaf_id()		= next_leaf->id();
				}

				if (this->split_watcher_.first == leaf)
				{
					this->split_watcher_.second = next_leaf;
				}

				NodeBaseG node_to_update = leaf;

				for (Int ss = 0; ss < Streams - 1; ss++)
				{
					auto& lleaf 			= this->leafs_[ss];
					auto& lanchor			= this->anchors_[ss];

					if (lleaf == leaf) {
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
			}
		}

		return next_leaf;
	}



	void updateLeafAnchors(const NodeBaseG& leaf, const Position& at, const Position& sizes)
	{
		MEMORIA_ASSERT_TRUE(current_extent_.gteAll(0));

		for (Int c = 0; c < Streams - 1; c++)
		{
			if (sizes[c] > 0)
			{
				this->anchors_[c] 		= at[c] + sizes[c] - 1;

				this->leafs_[c]  		= leaf;
				this->anchor_values_[c] = 0;

				this->leaf_extents_[c] = current_extent_;
			}
		}
	}
};









//template <Int StreamIdx> struct StreamTag {};

namespace details {

template <
	template <Int> class Tag,
	Int Size,
	Int Idx = 0
>
struct PopulateHelper2 {
	template <typename Chunk, typename Provider, typename Buffer, typename... Args>
	static void process(Chunk&& chunk, Provider&& provider, Buffer&& buffer, Int capacity, Args&&... args)
	{
		if (chunk.symbol() == Idx)
		{
			auto len1  = chunk.length();

			if (len1 > capacity)
			{
				len1 = capacity;
			}

			auto len2 = provider.populate(Tag<Idx>(), std::get<Idx>(buffer), len1, std::forward<Args>(args)...);

			chunk.set_length(len2);
		}
		else {
			PopulateHelper2<Tag, Size, Idx + 1>::process(
					chunk,
					std::forward<Provider>(provider),
					std::forward<Buffer>(buffer),
					capacity,
					std::forward<Args>(args)...
			);
		}
	}
};


template <template <Int> class Tag, Int Size>
struct PopulateHelper2<Tag, Size, Size> {

	template <typename Chunk, typename Provider, typename Buffer, typename... Args>
	static void process(Chunk&& chunk, Provider&& provider, Buffer&& buffer, Int capacity, Args&&... args)
	{
		auto len1  = chunk.length();

		if (len1 > capacity)
		{
			len1 = capacity;
		}

		auto len2 = provider.populateLastStream(std::get<Size>(buffer), len1, std::forward<Args>(args)...);

		chunk.set_length(len2);
	}
};


}


template <
	typename CtrT,
	typename DataProvider
>
class StreamingCtrInputProvider2: public memoria::bttl::AbstractCtrInputProvider2<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {
public:
	using Base 		= memoria::bttl::AbstractCtrInputProvider2<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

	using CtrSizesT = typename CtrT::Types::Position;

	static constexpr Int Streams = CtrT::Types::Streams;
private:

	DataProvider& data_provider_;
public:
	StreamingCtrInputProvider2(CtrT& ctr, DataProvider& data_provider, Int start_level, Int total_capacity = 4000):
		Base(ctr, start_level, total_capacity),
		data_provider_(data_provider)
	{}

	virtual RunDescr populate(Int capacity)
	{
		RunDescr chunk = data_provider_.query();

		if (chunk.symbol() >= 0)
		{
			details::PopulateHelper2<StreamTag, Streams - 1>::process(chunk, data_provider_, this->buffer_, capacity);
		}

		return chunk;
	}

	const DataProvider& data_provider() const {
		return data_provider_;
	}
};





}
}






#endif