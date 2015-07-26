
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_PROTOTYPES_BT_INPUT_HPP_
#define MEMORIA_PROTOTYPES_BT_INPUT_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/exceptions/memoria.hpp>

#include <cstdlib>
#include <tuple>

namespace memoria 	{
namespace bt 		{



//template <typename T>
//T* Malloc(size_t size, const char* source = MA_SRC)
//{
//	T* ptr = T2T<T>(std::malloc(size));
//
//	if (ptr != nullptr) {
//		return ptr;
//	}
//	else {
//		throw OOMException(source);
//	}
//}
//
//
//template <typename T>
//T* Realloc(T* ptr, size_t new_size, const char* source = MA_SRC)
//{
//	T* new_ptr = T2T<T>(std::realloc(ptr, new_size));
//
//	if (new_ptr != nullptr) {
//		return new_ptr;
//	}
//	else {
//		throw OOMException(source);
//	}
//}



template <typename Types>
struct AbstractInputProvider {
	using Position = typename Types::Position;
	using NodeBaseG = typename Types::NodeBaseG;

	virtual bool hasData() = 0;
	virtual Position fill(NodeBaseG& leaf, const Position& from)	= 0;
};


template <typename Tuple, typename SizeT>
class SizedTuple {
	Tuple tuple_;
	SizeT size_;

public:
	SizedTuple(const Tuple& tuple, SizeT size): tuple_(tuple), size_(size) {}

	SizeT size() const {
		return size_;
	}

	const Tuple& tuple() const {
		return tuple_;
	}
};



template <typename Tuple, Int Idx>
class InputTupleView {
	const Tuple& tuples_;
	Int size_;
public:
	InputTupleView(const Tuple& tuples, Int size): tuples_(tuples), size_(size)
	{}

	Int size() const {
		return size_;
	}

	auto get(Int idx) const {
		return std::get<Idx>(tuples_[idx].tuple());
	}
};

namespace detail {

template <typename Types, Int Streams, Int Idx = 0>
struct InputBufferBuilder {
	using InputTuple 	= typename Types::template StreamInputTuple<Idx>;
	using SizeT 		= typename Types::CtrSizeT;

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



template <typename InputBuffer, Int Idx = 0, Int Size = std::tuple_size<InputBuffer>::value - 1> struct RankHelper;
template <typename InputBuffer, Int Idx, Int Size>
struct RankHelper {
	template <typename Fn, typename... Args>
	static void process(const InputBuffer& tuple, Fn&& fn, Args&&... args)
	{
		using NextHelper = RankHelper<InputBuffer, Idx + 1, Size>;
		fn.template process<Idx, NextHelper>(tuple, std::forward<Args>(args)...);
	}
};


template <typename InputBuffer, Int Idx>
struct RankHelper<InputBuffer, Idx, Idx> {
	template <typename Fn, typename... Args>
	static void process(const InputBuffer& tuple, Fn&& fn, Args&&... args)
	{
		fn.template processLast<Idx>(tuple, std::forward<Args>(args)...);
	}
};



template <
	template <Int> class TupleHelper,
	typename CtrSizeT,
	typename Position
>
struct RankFn {
	CtrSizeT pos_ = 0;
	CtrSizeT target_;

	Position start_;
	Position size_;
	Position prefix_;
	Position indexes_;

	RankFn(const Position& start, const Position& size, const Position& prefix, CtrSizeT target):
		target_(target),
		start_(start),
		size_(size),
		prefix_(prefix),
		indexes_(prefix)
	{}

	template <Int Idx, typename NextHelper, typename Buffer>
	void process(Buffer&& buffer, CtrSizeT length)
	{
		for (auto i = 0; i < length; i++)
		{
			if (pos_ < target_)
			{
				auto next_length = TupleHelper<Idx>::get(std::get<Idx>(buffer), i + prefix_[Idx] + start_[Idx]);

				indexes_[Idx]++;
				pos_++;

				NextHelper::process(buffer, *this, next_length);
			}
			else {
				break;
			}
		}
	}


	template <Int Idx, typename Buffer>
	void processLast(Buffer&& buffer, CtrSizeT length)
	{
		CtrSizeT size = size_[Idx];

		if (indexes_[Idx] + length > size)
		{
			length = size;
		}

		if (pos_ + length < target_)
		{
			indexes_[Idx] += length;
			pos_ += length;
		}
		else
		{
			auto limit = target_ - pos_;
			indexes_[Idx] += limit;
			pos_ += limit;
		}
	}
};


template <
	Int Idx,
	Int Size
>
struct ZeroRankHelper
{
	template <typename Provider, typename Position1, typename Position2, typename... Args>
	static auto process(const Provider* provider, Position1&& sizes, Position2&& prefix, Args&&... args)
	{
		auto size = sizes[Idx];
		if (size > 0)
		{
			return provider->template _rank<Idx>(prefix, size, std::forward<Args>(args)...);
		}
		else {
			return ZeroRankHelper<Idx + 1, Size>::process(provider, std::forward<Position1>(sizes), std::forward<Position2>(prefix), std::forward<Args>(args)...);
		}
	}
};


template <Int Idx>
struct ZeroRankHelper<Idx, Idx> {
	template <typename Provider, typename Position1, typename Position2, typename... Args>
	static auto process(const Provider*, Position1&&, Position2&&, Args&&...)
	{
		return typename std::remove_reference<Position1>::type();
	}
};




template <
	template <Int> class TupleHelper,
	typename CtrSizeT,
	typename Position
>
struct ExtendFn {

	Position start_;
	Position size_;
	Position prefix_;
	Position indexes_;

	ExtendFn(const Position& start, const Position& size, const Position& prefix):
		start_(start), size_(size), prefix_(prefix), indexes_(prefix)
	{}

	template <Int Idx, typename NextHelper, typename Buffer>
	void process(Buffer&& buffer, CtrSizeT end)
	{
		CtrSizeT buffer_size = size_[Idx] - start_[Idx];

		if (end > buffer_size)
		{
			end = buffer_size;
		}

		indexes_[Idx] += end;

		CtrSizeT size = 0;
		for (auto i = 0; i < end; i++)
		{
			size += TupleHelper<Idx>::get(std::get<Idx>(buffer), i + prefix_[Idx] + start_[Idx]);
		}

		NextHelper::process(buffer, *this, size);
	}


	template <Int Idx, typename Buffer>
	void processLast(Buffer&& buffer, CtrSizeT end)
	{
		CtrSizeT buffer_size = size_[Idx] - start_[Idx];

		if (end > buffer_size)
		{
			end = buffer_size;
		}

		indexes_[Idx] += end;
	}
};



template <
	Int Idx,
	Int Size
>
struct ZeroExtendHelper
{
	template <typename Provider, typename Position1, typename Position2, typename Position3>
	static auto process(const Provider* provider, Position1&& sizes, Position2&& prefix, Position3&& pos) -> typename std::remove_reference<Position1>::type
	{
		auto size = sizes[Idx];
		if (size > 0)
		{
			return provider->template _extend<Idx>(prefix, pos[Idx]);
		}
		else {
			return ZeroExtendHelper<Idx + 1, Size>::process(provider, std::forward<Position1>(sizes), std::forward<Position2>(prefix), std::forward<Position3>(pos));
		}
	}
};


template <Int Idx>
struct ZeroExtendHelper<Idx, Idx> {
	template <typename Provider, typename Position1, typename Position2, typename Position3>
	static auto process(const Provider*, Position1&&, Position2&&, Position3&&) -> typename std::remove_reference<Position1>::type
	{
		return typename std::remove_reference<Position1>::type();
	}
};



template <
	template <Int> class SizeAccessor,
	Int Idx,
	Int Size
>
struct InputTupleSizeHelper {
	template <typename Buffer, typename... Args>
	static auto get(Int stream, Buffer&& buffer, Args&&... args) ->
		decltype(SizeAccessor<Idx>::get(std::get<Idx>(buffer), std::forward<Args>(args)...))
	{
		if (stream == Idx)
		{
			return SizeAccessor<Idx>::get(std::get<Idx>(buffer), std::forward<Args>(args)...);
		}
		else {
			return InputTupleSizeHelper<SizeAccessor, Idx + 1, Size>::get(stream, std::forward<Buffer>(buffer), std::forward<Args>(args)...);
		}
	}

	template <typename Buffer, typename... Args>
	static void add(Int stream, Buffer&& buffer, Args&&... args)
	{
		if (stream == Idx)
		{
			SizeAccessor<Idx>::add(std::get<Idx>(buffer), std::forward<Args>(args)...);
		}
		else {
			InputTupleSizeHelper<SizeAccessor, Idx + 1, Size>::add(stream, std::forward<Buffer>(buffer), std::forward<Args>(args)...);
		}
	}
};



template <
	template <Int> class SizeAccessor,
	Int Size
>
struct InputTupleSizeHelper<SizeAccessor, Size, Size> {
	template <typename Buffer, typename... Args>
	static Int get(Int stream, Buffer&& buffer, Args&&... args){
		return 0;
	}

	template <typename Buffer, typename... Args>
	static void add(Int stream, Buffer&& buffer, Args&&... args){}
};


template <
	typename Dispatcher,
	template<Int> class Fn,
	typename PathList,
	Int Idx = 0
>
struct UpdateLeafH;

template <
	typename Dispatcher,
	template<Int> class Fn,
	typename Path,
	typename... Tail,
	Int Idx
>
struct UpdateLeafH<Dispatcher, Fn, TL<Path, Tail...>, Idx> {

	template <typename NTypes, typename... Args>
	void treeNode(LeafNode<NTypes>* node, Args&&... args)
	{
		node->template processStream<Path>(Fn<Idx>(), std::forward<Args>(args)...);
	}

	template <typename PageG, typename... Args>
	void process(Int stream, PageG&& page, Args&&... args)
	{
		if (stream == Idx)
		{
			Dispatcher::dispatch(std::forward<PageG>(page), *this, std::forward<Args>(args)...);
		}
		else {
			UpdateLeafH<Dispatcher, Fn, TL<Tail...>, Idx + 1>::process(stream, std::forward<PageG>(page), std::forward<Args>(args)...);
		}
	}
};



template <
	typename Dispatcher,
	template<Int> class Fn,
	Int Idx
>
struct UpdateLeafH<Dispatcher, Fn, TL<>, Idx> {
	template <typename PageG, typename... Args>
	static void process(Int stream, PageG&& page, Args&&... args)
	{
		throw vapi::Exception(MA_SRC, SBuf()<<"Failed to dispatch stream: "<<stream);
	}
};

}


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
class AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::FIXED>: public AbstractInputProvider<typename CtrT::Types> {

	using Base = AbstractInputProvider<typename CtrT::Types>;

public:
	using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLengthType::FIXED>;

	using NodeBaseG = typename CtrT::Types::NodeBaseG;
	using CtrSizeT 	= typename CtrT::Types::CtrSizeT;

	using Buffer = typename detail::AsTuple<
			typename detail::InputBufferBuilder<
				typename CtrT::Types,
				Streams
			>::Type
	>::Type;

	using Position	= typename Base::Position;

	template <Int, Int> friend struct detail::ZeroExtendHelper;
	template <Int, Int> friend struct detail::ZeroRankHelper;
	template <template <Int> class T, typename, typename> friend struct detail::RankFn;
	template <template <Int> class T, typename, typename> friend struct detail::ExtendFn;

	template <Int StreamIdx>
	using InputTupleSizeAccessor = typename CtrT::Types::template InputTupleSizeAccessor<StreamIdx>;

	using InputTupleSizeHelper = detail::InputTupleSizeHelper<InputTupleSizeAccessor, 0, std::tuple_size<Buffer>::value>;


	using ForAllBuffer = detail::ForAllTuple<std::tuple_size<Buffer>::value>;

protected:
	Buffer buffer_;
	Position start_;
	Position size_;


	Position prefix_;

	CtrT& 	ctr_;

	Position pos_;

	Position ancors_;
	NodeBaseG leafs_[Streams];
	Position buffer_sums_;
	Position rank_;

private:

	struct ResizeBufferFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, const Position& sizes)
		{
			buffer.resize(sizes[Idx]);
		}
	};

public:

	AbstractCtrInputProvider(CtrT& ctr, const Position& capacity): Base(),
		ctr_(ctr)
	{
		ForAllBuffer::process(buffer_, ResizeBufferFn(), capacity);
	}

	CtrT& ctr() {return ctr_;}
	const CtrT& ctr() const {return ctr_;}

	virtual bool hasData()
	{
		auto sizes = this->buffer_size();

		bool buffer_has_data = pos_.sum() < sizes.sum();

		return buffer_has_data || populate_buffer();
	}

	virtual Position fill(NodeBaseG& leaf, const Position& from)
	{
		Position pos = from;

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

	virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes)
	{
		auto size  = sizes.sum();

		auto imax 			= size;
		decltype(imax) imin = 0;
		auto accepts 		= 0;

		while (accepts < 50 && imax > imin)
		{
			if (imax - 1 != imin)
			{
				auto mid = imin + ((imax - imin) / 2);

				if (this->checkSize(leaf, mid))
				{
					accepts++;
					imin = mid + 1;
				}
				else {
					imax = mid - 1;
				}
			}
			else {
				if (this->checkSize(leaf, imax))
				{
					accepts++;
				}

				break;
			}
		}

		if (imax < size)
		{
			return rank(imax);
		}
		else {
			return sizes;
		}
	}

	bool checkSize(const NodeBaseG& leaf, CtrSizeT target_size)
	{
		auto rank = this->rank(target_size);
		return ctr_.checkCapacities(leaf, rank);
	}


	struct InsertBufferFn {

		template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename StreamObj>
		void stream(StreamObj* stream, const Position& at, const Position& starts, const Position& sizes, const Buffer& buffer)
		{
			static_assert(StreamIdx < Position::Indexes, "");
			static_assert(StreamIdx < tuple_size<Buffer>::value, "");
			static_assert(Idx < tuple_size<typename tuple_element<StreamIdx, Buffer>::type::value_type>::value, "");

			stream->_insert(at[StreamIdx], sizes[StreamIdx], [&](Int idx){
				return std::get<Idx>(std::get<StreamIdx>(buffer)[idx + starts[StreamIdx]]);
			});
		}


		template <typename NodeTypes, typename... Args>
		void treeNode(LeafNode<NodeTypes>* leaf, Args&&... args)
		{
			leaf->processSubstreamGroups(*this, std::forward<Args>(args)...);
		}
	};


	virtual void insertBuffer(NodeBaseG& leaf, const Position& at, const Position& sizes)
	{
		CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, InsertBufferFn(), at, start_, sizes, buffer_);

		for (Int c = 0; c < Streams - 1; c++)
		{
			if (sizes[c] > 0)
			{
				ancors_[c] = at[c] + sizes[c] - 1;
				leafs_[c]  = leaf;
			}
		}

		auto ext = this->extend(sizes);
		this->prefix_ = ext - sizes;

		start_ += sizes;
	}




	virtual CtrSizeT populate(Int stream) {
		return 0;
	}

	const Buffer& buffer() const {
		return buffer_;
	}

	const Position& prefix() const {
		return prefix_;
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

	Position buffer_capacity() const
	{
		Position sizes;
		ForAllBuffer::process(buffer_, BufferCapacityFn(), sizes);
		return sizes;
	}


	Position rank(CtrSizeT idx) const
	{
		auto prefix_size = prefix_.sum();

		if (idx > prefix_size)
		{
			return this->rank(prefix_, idx);
		}
		else {
			return this->rank_p(idx);
		}
	}

	Position extend(const Position& pos) const
	{
		auto prefix_size = prefix_.sum();

		if (pos.sum() > prefix_size)
		{
			return this->extend(prefix_, pos);
		}
		else {
			return this->extend_p(pos);
		}
	}

	Position rank() const {
		return buffer_size();
	}


	virtual bool populate_buffer()
	{
		Int last_symbol = -1;

		Position cnt;

		Position sizes;

		start_.clear();
		size_.clear();

		auto capacity = this->buffer_capacity();

		while (true)
		{
			Int symbol = populate(buffer_);

			if (symbol >= 0)
			{
				cnt[symbol]++;

				if (symbol > last_symbol + 1)
				{
					throw Exception(MA_SRC, SBuf()<<"Invalid sequence state: last_symbol="<<last_symbol<<", symbol="<<symbol);
				}
				else if (symbol < last_symbol)
				{
					for (Int sym = last_symbol; sym > symbol; sym--)
					{
						if (sizes[sym - 1] > 0)
						{
							InputTupleSizeHelper::add(sym - 1, buffer_, sizes[sym - 1], buffer_sums_[sym]);
						}
						else if (leafs_[sym - 1].isSet())
						{
							updateLeaf(sym - 1, ancors_[sym - 1], buffer_sums_[sym]);
						}

						buffer_sums_[sym] = 0;
					}
				}

				last_symbol = symbol;

				buffer_sums_[symbol]++;
				rank_[symbol]++;
				sizes[symbol]++;

				if (cnt[symbol] == capacity[symbol]) {
					break;
				}
			}
			else {
				break;
			}
		}

		size_ = cnt;

		return cnt.sum() > 0;
	}

	virtual Int populate(Buffer& buffer) = 0;

private:

	void updateLeaf(Int sym, CtrSizeT pos, CtrSizeT sum)
	{
		detail::UpdateLeafH<
			typename CtrT::Types::Pages::LeafDispatcher,
			CtrT::Types::template LeafStreamSizeAccessor,
			typename CtrT::Types::StreamsSizes
		>().process(sym, leafs_[sym], pos, sum);
	}


	Position rank(const Position& prefix, CtrSizeT target) const
	{
		return detail::ZeroRankHelper<0, std::tuple_size<Buffer>::value>::process(this, this->buffer_size(), prefix, target);
	}

	// rank inside prefix
	Position rank_p(CtrSizeT target) const
	{
		return detail::ZeroRankHelper<0, std::tuple_size<Buffer>::value>::process(this, prefix_, prefix_, target);
	}



	template <Int Idx>
	Position _rank(const Position& prefix, CtrSizeT length, CtrSizeT target) const
	{
		detail::RankFn<InputTupleSizeAccessor, CtrSizeT, Position> fn(start_, size_, prefix , target);
		detail::RankHelper<Buffer>::process(buffer_, fn, length);

		return fn.indexes_;
	}

	template <Int Idx>
	Position _extend(const Position& prefix, CtrSizeT size) const
	{
		detail::ExtendFn<InputTupleSizeAccessor, CtrSizeT, Position> fn(start_, size_, prefix);
		detail::RankHelper<Buffer>::process(buffer_, fn, size);
		return fn.indexes_;
	}

	Position extend(const Position& prefix, const Position& pos) const
	{
		return detail::ZeroExtendHelper<0, std::tuple_size<Buffer>::value>::process(this, this->buffer_size(), prefix, pos);
	}

	Position extend_p(const Position& pos) const
	{
		return detail::ZeroExtendHelper<0, std::tuple_size<Buffer>::value>::process(this, prefix_, prefix_, pos);
	}
};


}
}

#endif
