
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


template <typename T>
T* Malloc(size_t size, const char* source = MA_SRC)
{
	T* ptr = T2T<T>(std::malloc(size));

	if (ptr != nullptr) {
		return ptr;
	}
	else {
		throw OOMException(source);
	}
}


template <typename T>
T* Realloc(T* ptr, size_t new_size, const char* source = MA_SRC)
{
	T* new_ptr = T2T<T>(std::realloc(ptr, new_size));

	if (new_ptr != nullptr) {
		return new_ptr;
	}
	else {
		throw OOMException(source);
	}
}



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


template <typename InputBuffer, Int Size = std::tuple_size<InputBuffer>::value, Int Idx = 0> struct ForAllTuple;
template <typename InputBuffer, Int Size, Int Idx>
struct ForAllTuple {
	template <typename Fn, typename... Args>
	static void process(InputBuffer& tuple, Fn&& fn, Args&&... args)
	{
		fn.template process<Idx>(std::get<Idx>(tuple), std::forward<Args>(args)...);
		ForAllTuple<InputBuffer>::process(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
	}

	template <typename Fn, typename... Args>
	static void process(const InputBuffer& tuple, Fn&& fn, Args&&... args)
	{
		fn.template process<Idx>(std::get<Idx>(tuple), std::forward<Args>(args)...);
		ForAllTuple<InputBuffer>::process(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
	}
};

template <typename InputBuffer, Int Idx>
struct ForAllTuple<InputBuffer, Idx, Idx> {
	template <typename Fn, typename... Args>
	static void process(InputBuffer& tuple, Fn&& fn, Args&&... args)
	{}

	template <typename Fn, typename... Args>
	static void process(const InputBuffer& tuple, Fn&& fn, Args&&... args)
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

	Position prefix_;
	Position indexes_;

	RankFn(const Position& prefix, CtrSizeT target):
		target_(target),
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
				auto next_length = TupleHelper<Idx>::get(std::get<Idx>(buffer), i + prefix_[Idx]);

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
		CtrSizeT size = std::get<Idx>(buffer).size();

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

	Position prefix_;
	Position indexes_;

	ExtendFn(const Position& prefix): prefix_(prefix), indexes_(prefix)
	{}

	template <Int Idx, typename NextHelper, typename Buffer>
	void process(Buffer&& buffer, CtrSizeT end)
	{
		CtrSizeT buffer_size = std::get<Idx>(buffer).size();

		if (end > buffer_size)
		{
			end = buffer_size;
		}

		indexes_[Idx] += end;

		CtrSizeT size = 0;
		for (auto i = 0; i < end; i++)
		{
			size += TupleHelper<Idx>::get(std::get<Idx>(buffer), i + prefix_[Idx]);
		}

		NextHelper::process(buffer, *this, size);
	}


	template <Int Idx, typename Buffer>
	void processLast(Buffer&& buffer, CtrSizeT end)
	{
		CtrSizeT buffer_size = std::get<Idx>(buffer).size();

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
class AbstractCtrInputProvider: public AbstractInputProvider<typename CtrT::Types> {

	using Base = AbstractInputProvider<typename CtrT::Types>;

public:
	using MyType = AbstractCtrInputProvider<CtrT, Streams, LeafDataLength>;

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

protected:
	Buffer buffer_;
	Position prefix_;
	CtrT& 	ctr_;

	CtrSizeT capacity_;

	Position pos_;



	Position ancors_;
	NodeBaseG leafs_[Streams];
	Position buffer_sums_;
	Position rank_;

private:



public:
	AbstractCtrInputProvider(CtrT& ctr, CtrSizeT capacity): Base(),
		ctr_(ctr), capacity_(capacity)
	{

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
			}

			auto capacity = this->findCapacity(leaf, pos);

			if (capacity.sum() > 0)
			{
				insertBuffer(leaf, pos, capacity);
			}
			else {
				return pos;
			}
		}
	}

	virtual Position findCapacity(NodeBaseG& leaf, const Position& from)
	{
		auto sizes = this->buffer_size();
		auto size  = sizes.sum();

		auto imax 			= size;
		decltype(imax) imin = 0;
		auto accepts 		= 0;

		while (accepts < 50 && imax > imin)
		{
			if (imax - 1 != imin)
			{
				auto mid = imin + ((imax - imin) / 2);

				if (this->checkSize(leaf, from, mid))
				{
					accepts++;
					imin = mid + 1;
				}
				else {
					imax = mid - 1;
				}
			}
			else {
				if (this->checkSize(leaf, from, imax))
				{
					accepts++;
				}
				break;
			}
		}

		return Position();
	}

	bool checkSize(const NodeBaseG& leaf, const Position& from, CtrSizeT target_size)
	{
		auto rank = this->rank(target_size);

		// FIXME: check if negation is necessary here?
		return ctr_.checkCapacities(leaf, rank);
	}

	void insertBuffer(NodeBaseG& leaf, const Position& from, const Position sizes)
	{
		// insert data to the leaf
		// remove it from input buffer, forming ancors
		// if necessaryy
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

	struct BufferSizeFn {
		template <Int Idx, typename Buffer>
		void process(Buffer&& buffer, Position& pos)
		{
			pos[Idx] = buffer.size();
		}
	};

	Position buffer_size() const
	{
		Position sizes;
		detail::ForAllTuple<Buffer>::process(buffer_, BufferSizeFn(), sizes);
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

		CtrSizeT cnt = 0;

		Position sizes;

		while (true)
		{
			Int symbol = populate(buffer_);

			if (symbol >= 0)
			{
				cnt++;

				if (symbol < last_symbol)
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

				if (cnt == capacity_) {
					break;
				}
			}
			else {
				break;
			}
		}

		return cnt > 0;
	}



	virtual Int populate(Buffer& buffer) const
	{
		return -1;
	}

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
		detail::RankFn<InputTupleSizeAccessor, CtrSizeT, Position> fn(prefix, target);
		detail::RankHelper<Buffer>::process(buffer_, fn, length);
		return fn.indexes_;
	}

	template <Int Idx>
	Position _extend(const Position& prefix, CtrSizeT size) const
	{
		detail::ExtendFn<InputTupleSizeAccessor, CtrSizeT, Position> fn(prefix);
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
