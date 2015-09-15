
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_PROTOTYPES_BT_TL_INPUT_HPP_
#define MEMORIA_PROTOTYPES_BT_TL_INPUT_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>

namespace memoria 	{
namespace bttl 		{

namespace detail {

template <typename Types, Int Streams, Int Idx = 0>
struct InputBufferBuilder {
	using SizeT 		= typename Types::CtrSizeT;
	using InputTuple 	= typename Types::template StreamInputTuple<Idx>;


	using Type = MergeLists<
			std::vector<InputTuple>,
			typename InputBufferBuilder<Types, Streams, Idx + 1>::Type
	>;
};

template <typename Types, Int Streams>
struct InputBufferBuilder<Types, Streams, Streams> {
	using Type = TL<>;
};





template <typename List>  struct AsTuple;

template <typename... Types>
struct AsTuple<TL<Types...>> {
	using Type = std::tuple<Types...>;
};


template <Int Size, Int Idx = 0>
struct ForAllTuple {
	template <typename InputBuffer, typename Fn, typename... Args>
	static void process(InputBuffer&& tuple, Fn&& fn, Args&&... args)
	{
		fn.template process<Idx>(std::get<Idx>(tuple), std::forward<Args>(args)...);
		ForAllTuple<Size, Idx + 1>::process(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
	}
};

template <Int Idx>
struct ForAllTuple<Idx, Idx> {
	template <typename InputBuffer, typename Fn, typename... Args>
	static void process(InputBuffer&& tuple, Fn&& fn, Args&&... args)
	{}
};




template <
	typename Dispatcher,
	Int Size,
	template <Int> class PathListT,
	Int StreamIdx = 0
>
struct UpdateLeafH {

	template <typename NTypes, typename SizeT>
	void treeNode(LeafNode<NTypes>* node, Int idx, SizeT value)
	{
		using Path = PathListT<StreamIdx>;

		auto substream = node->template substream<Path>();

		substream->addValue(0, idx, value);
	}

	template <typename PageG, typename... Args>
	void process(Int stream, PageG&& page, Args&&... args)
	{
		if (stream == StreamIdx)
		{
			Dispatcher::dispatch(std::forward<PageG>(page), *this, std::forward<Args>(args)...);
		}
		else {
			UpdateLeafH<Dispatcher, Size, PathListT, StreamIdx + 1>().process(stream, std::forward<PageG>(page), std::forward<Args>(args)...);
		}
	}
};



template <
	typename Dispatcher,
	Int Size,
	template <Int> class PathListT
>
struct UpdateLeafH<Dispatcher, Size, PathListT, Size> {
	template <typename PageG, typename... Args>
	void process(Int stream, PageG&& page, Args&&... args)
	{
		throw vapi::Exception(MA_SRC, SBuf()<<"Failed to dispatch stream: "<<stream);
	}
};

}












template <
	typename CtrT
>
class AbstractCtrInputProviderBase: public memoria::bt::AbstractInputProvider<typename CtrT::Types> {

	using Base = AbstractInputProvider<typename CtrT::Types>;
protected:
	static const Int Streams = CtrT::Types::Streams;

public:
	using MyType = AbstractCtrInputProviderBase<CtrT>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;

	using Iterator  = typename CtrT::Iterator;

	using Buffer = typename detail::AsTuple<
			typename detail::InputBufferBuilder<
				typename CtrT::Types,
				Streams
			>::Type
	>::Type;

	using Position	= typename Base::Position;

	using ForAllBuffer = detail::ForAllTuple<std::tuple_size<Buffer>::value>;

	static constexpr Int get_symbols_number(Int v) {
		return v == 1 ?  0 : (v == 2 ? 1 : (v == 3 ? 2 : (v == 4 ? 2 : (v == 5 ? 3 : (v == 6 ? 3 : (v == 7 ? 3 : 4))))));
	}

	using Symbols = PkdFSSeq<typename PkdFSSeqTF<get_symbols_number(Streams)>::Type>;

	class RunDescr {
		Int symbol_;
		Int length_;
	public:
		RunDescr(Int symbol, Int length = 1): symbol_(symbol), length_(length) {}

		Int symbol() const {return symbol_;}
		Int length() const {return length_;}
	};

protected:
	Buffer buffer_;
	Position start_;
	Position size_;

	Position capacity_;

	FreeUniquePtr<PackedAllocator> allocator_;

	CtrT& 	ctr_;

	Position ancors_;
	NodeBaseG leafs_[Streams];

	Int last_symbol_ = -1;

	CtrSizeT fills_ = 0;

private:



	struct ResizeBufferFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, const Position& sizes)
		{
			buffer.resize(sizes[Idx]);
		}
	};



public:

	AbstractCtrInputProviderBase(CtrT& ctr, const Position& capacity): Base(),
		capacity_(capacity),
		allocator_(AllocTool<PackedAllocator>::create(block_size(capacity.sum()), 1)),
		ctr_(ctr)
	{
		ForAllBuffer::process(buffer_, ResizeBufferFn(), capacity);

		allocator_->template allocate<Symbols>(0, Symbols::packed_block_size(capacity.sum()));

		symbols()->enlargeData(capacity.sum());
	}
private:
	static Int block_size(Int capacity)
	{
		return Symbols::packed_block_size(capacity);
	}
public:

	Iterator compute_ancors(const NodeBaseG& leaf, const Position& start)
	{
		Iterator iter(this->ctr_);
		iter.leaf() = leaf;
		iter.refresh();

		for (Int s = 0; s < Streams; s++)
		{
			if (start[s] > 0)
			{
				this->ancors_[s] = start[s] - 1;
				this->leafs_[s]  = leaf;
			}
			else {
				auto tmp = iter;
				tmp.stream() = s;
				tmp.idx() = 0;

				if (tmp.skipBw(1) == 1)
				{
					this->ancors_[s] = tmp.idx();
					this->leafs_[s]  = tmp.leaf();
				}
			}
		}

		return iter;
	}


	CtrT& ctr() {return ctr_;}
	const CtrT& ctr() const {return ctr_;}

	virtual bool hasData()
	{
		bool buffer_has_data = start_.sum() < size_.sum();

		return buffer_has_data || populate_buffer();
	}

	virtual Position fill(NodeBaseG& leaf, const Position& start)
	{
		fills_++;

		if (this->fills_ == 1)
		{
			compute_ancors(leaf, start);
		}

		auto end = fill0(leaf, start);

		if (fills_ == 1 && this->hasData())
		{
			auto leaf_sizes = ctr().getLeafStreamSizes(leaf);

			if (end.sum() < leaf_sizes.sum())
			{
				ctr().splitLeafP(leaf, end);

				return fill0(leaf, end);
			}
		}

		return end;
	}

	Position fill0(NodeBaseG& leaf, const Position& start)
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
					return pos + buffer_sizes;
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

	virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes) = 0;


	struct InsertBufferFn {

		template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename StreamObj>
		bool stream(StreamObj* stream, PackedAllocator* alloc, const Position& at, const Position& starts, const Position& sizes, const Buffer& buffer)
		{
			static_assert(StreamIdx < Position::Indexes, "");
			static_assert(StreamIdx < std::tuple_size<Buffer>::value, "");

//			if (stream == nullptr) {
//				stream = alloc->template allocateEmpty<StreamObj>(AllocatorIdx);
//			}

			stream->_insert(at[StreamIdx], sizes[StreamIdx], [&](Int idx){
				return std::get<Idx>(std::get<StreamIdx>(buffer)[idx + starts[StreamIdx]]);
			});

			return true;
		}


		template <typename NodeTypes, typename... Args>
		auto treeNode(LeafNode<NodeTypes>* leaf, Args&&... args)
		{
			leaf->layout(255);
			return leaf->processSubstreamGroups(*this, leaf->allocator(), std::forward<Args>(args)...);
		}
	};


	void updateLeafAncors(const NodeBaseG& leaf, const Position& at, const Position& sizes)
	{
		for (Int c = 0; c < Streams - 1; c++)
		{
			if (sizes[c] > 0)
			{
				ancors_[c] = at[c] + sizes[c] - 1;
				leafs_[c]  = leaf;
			}
		}
	}


	virtual void insertBuffer(NodeBaseG& leaf, const Position& at, const Position& sizes)
	{
		CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, InsertBufferFn(), at, start_, sizes, buffer_);

		updateLeafAncors(leaf, at, sizes);

		start_ += sizes;
	}

	const Buffer& buffer() const {
		return buffer_;
	}

	Position buffer_size() const
	{
		return size_ - start_;
	}

	struct BufferCapacityFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, Position& pos)
		{
			pos[Idx] = buffer.size();
		}
	};

	Position capacity() const
	{
		return capacity_;
	}

	Position buffer_capacity() const
	{
		Position sizes;
		ForAllBuffer::process(buffer_, BufferCapacityFn(), sizes);
		return sizes;
	}


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
		Position sizes;
		Position buffer_sums;

		start_.clear();
		size_.clear();

		Int symbol_pos = 0;

		auto capacity = this->buffer_capacity();

		auto symbols = this->symbols();

		auto& symbols_size = symbols->size();

		symbols_size = 0;

		auto syms = symbols->symbols();

		while (true)
		{
			RunDescr run = populate(size_);

			Int symbol = run.symbol();
			Int length = run.length();

			if (symbol >= 0)
			{
				symbols_size += length;
				for (Int c = symbol_pos; c < symbol_pos + length; c++)
				{
					symbols->tools().set(syms, c, symbol);
				}

				symbol_pos += length;

				size_[symbol] += length;

				if (symbol > last_symbol_ + 1)
				{
					throw Exception(MA_SRC, SBuf()<<"Invalid sequence state: last_symbol="<<last_symbol_<<", symbol="<<symbol);
				}
				else if (symbol < last_symbol_)
				{
					this->finish_stream_run(symbol, last_symbol_, sizes, buffer_sums);
				}

				buffer_sums[symbol] += length;
				sizes[symbol] += length;

				if (size_[symbol] == capacity[symbol])
				{
					this->finish_stream_run(0, Streams - 1, sizes, buffer_sums);
					break;
				}

				last_symbol_ = symbol;
			}
			else {
				this->finish_stream_run(0, Streams - 1, sizes, buffer_sums);
				break;
			}
		}

		symbols->reindex();

		return symbol_pos > 0;
	}

	virtual RunDescr populate(const Position& pos) = 0;

	struct DumpFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, const Position& prefix, const Position& start, const Position& size)
		{
			std::cout<<"Level "<<Idx<<" prefix: "<<prefix[Idx]<<", start: "<<start[Idx]<<" size: "<<size[Idx]<<" capacity: "<<buffer.size()<<std::endl;
			for (Int c = start[Idx]; c < size[Idx]; c++)
			{
				std::cout<<c<<": "<<buffer[c]<<std::endl;
			}
			std::cout<<std::endl;
		}
	};


	void dumpBuffer() const
	{
		ForAllBuffer::process(buffer_, DumpFn(), Position(), start_, size_);

		symbols()->dump();
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
			using BufferEntry 	  = typename std::tuple_element<SStreamIdx, typename std::remove_reference<Buffer>::type>::type::value_type;
			constexpr Int SizeIdx = std::tuple_size<BufferEntry>::value - 1;

			std::get<SizeIdx>(std::get<SStreamIdx>(buffer)[idx]) += value;
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
					ForEachStream<Streams - 1>::process(sym - 1, AddSizeFn(), buffer_, sizes[sym - 1] - 1, buffer_sums[sym]);
				}
			}
			else if (leafs_[sym - 1].isSet())
			{
				if (buffer_sums[sym] > 0)
				{
					updateLeaf(sym - 1, ancors_[sym - 1], buffer_sums[sym]);
				}
			}

			buffer_sums[sym] = 0;
		}
	}

protected:

	virtual void updateLeaf(Int sym, CtrSizeT pos, CtrSizeT sum)
	{
		detail::UpdateLeafH<
			typename CtrT::Types::Pages::LeafDispatcher,
			CtrT::Types::Streams - 1,
			CtrT::Types::template LeafSizesSubstreamPath
		>().process(sym, leafs_[sym], pos, sum);
	}
};











template <
	typename CtrT,
	Int Streams,
	LeafDataLengthType LeafDataLength
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
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;

	using Buffer 	= typename Base::Buffer;
	using Position	= typename Base::Position;

public:

	AbstractCtrInputProvider(CtrT& ctr, const Position& capacity):
		Base(ctr, capacity)
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
class AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>: public AbstractCtrInputProviderBase<CtrT> {

	using Base = AbstractCtrInputProviderBase<CtrT>;

	static constexpr float FREE_SPACE_THRESHOLD = 0.05;



public:
	using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::VARIABLE>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;
	using Iterator 	= typename CtrT::Iterator;

	using Buffer 	= typename Base::Buffer;
	using Position	= typename Base::Position;

	using PageUpdateMgr			 = typename CtrT::Types::PageUpdateMgr;

	Position extent_;

	Position ancors_tmp_;
	NodeBaseG leafs_tmp_[Streams];

	PageUpdateMgr mgr_;

public:

	AbstractCtrInputProvider(CtrT& ctr, const Position& capacity):
		Base(ctr, capacity),
		mgr_(ctr)
	{}

	CtrT& ctr() {
		return this->ctr_;
	}

	virtual Position fill(NodeBaseG& leaf, const Position& start)
	{
		this->fills_++;

		if (this->fills_ == 1)
		{
			Iterator iter(this->ctr_);
			iter.leaf() = leaf;
			iter.refresh();

			extent_ = iter.leaf_extent();

			for (Int s = 0; s < Streams; s++)
			{
				if (start[s] > 0)
				{
					this->ancors_[s] = start[s] - 1;
					this->leafs_[s]  = leaf;
				}
				else {
					auto tmp = iter;

					tmp.stream() = s;
					tmp.idx() = 0;

					if (tmp.skipBw1() == 1)
					{
						this->ancors_[s] = tmp.idx();
						this->leafs_[s]  = tmp.leaf();
					}
				}
			}
		}


		auto end = fill0(leaf, start);

		if (this->fills_ == 1 && this->hasData())
		{
			auto leaf_sizes = ctr().getLeafStreamSizes(leaf);

			if (end.sum() < leaf_sizes.sum())
			{
				ctr().splitLeafP(leaf, end);

				return fill0(leaf, end);
			}
		}

		return end;
	}


	Position fill0(NodeBaseG& leaf, const Position& start)
	{
		Position pos = start;

		mgr_.add(leaf);

		typename PageUpdateMgr::Remover remover(mgr_, leaf);

		while(this->hasData())
		{
			auto buffer_sizes = this->buffer_size();

			auto inserted = insertBuffer(leaf, pos, buffer_sizes);

			if (inserted.sum() > 0)
			{
				this->updateLeafAncors(leaf, pos, inserted);

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

		return pos;
	}


	virtual Position insertBuffer(NodeBaseG& leaf, Position at, Position size)
	{
		if (tryInsertBuffer(leaf, at, size))
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
					if (tryInsertBuffer(leaf, at, sizes))
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
					if (tryInsertBuffer(leaf, at, sizes))
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

	bool tryInsertBuffer(NodeBaseG& leaf, const Position& at, const Position& size)
	{
		try {
			CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, typename Base::InsertBufferFn(), at, this->start_, size, this->buffer_);

			mgr_.checkpoint(leaf);

			return true;
		}
		catch (PackedOOMException& ex)
		{
			mgr_.restoreNodeState();
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

protected:

	virtual void updateLeaf(Int sym, CtrSizeT pos, CtrSizeT sum)
	{
		//FIXME: handle page overflows
		// split page if necessary

		detail::UpdateLeafH<
			typename CtrT::Types::Pages::LeafDispatcher,
			CtrT::Types::Streams - 1,
			CtrT::Types::template LeafSizesSubstreamPath
		>().process(sym, this->leafs_[sym], pos, sum);

		mgr_.checkpoint(this->leafs_[sym]);
	}
};





}
}






#endif
