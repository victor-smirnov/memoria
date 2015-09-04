
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria {
namespace bttl    {

template <typename FullPath>
using BTTLSizePath = typename SublistFromStart<FullPath, ListSize<FullPath>::Value - 1>::Type;

template <typename FullPath>
using BTTLSizePathBlockIdx = ListHead<typename SublistToEnd<FullPath, ListSize<FullPath>::Value - 1>::Type>;


namespace detail {


template <Int Idx, Int Size>
struct StreamsRankHelper {
	template <typename Node, typename Fn, typename... Args>
	static void process(const Node* node, Fn&& fn, Args&&... args)
	{
		using NextHelper = StreamsRankHelper<Idx + 1, Size>;
		fn.template process<Idx, NextHelper>(node, std::forward<Args>(args)...);
	}
};


template <Int Idx>
struct StreamsRankHelper<Idx, Idx> {
	template <typename Node, typename Fn, typename... Args>
	static void process(const Node* node, Fn&& fn, Args&&... args)
	{
		fn.template processLast<Idx>(node, std::forward<Args>(args)...);
	}
};



template <
	template <Int> class StreamsSizesPath,
	typename CtrSizeT,
	typename Position
>
struct StreamsRankFn {
	static const Int BufferSize = 8;

	CtrSizeT pos_;
	CtrSizeT target_;

	Position sizes_;
	Position prefix_;
	Position indexes_;

	StreamsRankFn(const Position& sizes, const Position& prefix, CtrSizeT target):
		target_(target),
		sizes_(sizes),
		prefix_(prefix),
		indexes_(prefix)
	{
		pos_ = prefix.sum();
	}

	template <Int Stream, typename NextHelper, typename Node>
	void process(const Node* node) {
		process<Stream, NextHelper>(node, sizes_[Stream]);
	}

	template <Int Stream, typename NextHelper, typename Node>
	void process(const Node* node, CtrSizeT length)
	{
		CtrSizeT buffer[BufferSize];
		for (auto& v: buffer) v = 0;

		Int buffer_pos = 0;
		Int buffer_size = 0;

		const Int size   = sizes_[Stream];
		const Int offset = indexes_[Stream] + prefix_[Stream];
		const Int limit  = (length + offset) < size ? length : (size - offset);

		for (auto i = 0; i < limit; i++)
		{
			if (pos_ < target_)
			{
				auto next_length = this->template buffered_get<Stream>(node, i + offset, buffer_pos, buffer_size, size, buffer);

				indexes_[Stream]++;
				pos_++;

				NextHelper::process(node, *this, next_length);
			}
			else {
				break;
			}
		}
	}


	template <Int Stream, typename Node>
	void processLast(const Node* node) {
		processLast<Stream>(node, sizes_[Stream]);
	}

	template <Int Stream, typename Node>
	void processLast(const Node* node, CtrSizeT length)
	{
		CtrSizeT size = node->template streamSize<Stream>();

		if (indexes_[Stream] + length > size)
		{
			length = size - indexes_[Stream];
		}

		if (pos_ + length < target_)
		{
			indexes_[Stream] += length;
			pos_ += length;
		}
		else
		{
			auto limit = target_ - pos_;
			indexes_[Stream] += limit;
			pos_ += limit;
		}
	}

private:

	template <Int Stream, typename Node>
	CtrSizeT buffered_get(const Node* node, Int idx, Int& p0, Int& s0, Int size, CtrSizeT buffer[BufferSize])
	{
		if (idx - p0 < s0) {
			return buffer[idx - p0];
		}
		else {
			Int limit = (idx + BufferSize) < size ? idx + BufferSize : size;

			using Path 		 = StreamsSizesPath<Stream>;
			using StreamPath = BTTLSizePath<Path>;
			const Int index  = BTTLSizePathBlockIdx<Path>::Value;

			node->template substream<StreamPath>()->read(index, idx, limit, buffer);

			p0 = idx;
			s0 = limit;

			return buffer[0];
		}
	}
};


template <
	Int Idx,
	Int Size
>
struct ZeroRankHelper
{
	template <typename Iter, typename Position, typename... Args>
	static auto process(const Iter* iter, Position&& sizes, Args&&... args)
	{
		auto size = sizes[Idx];
		if (size > 0)
		{
			return iter->template _streams_rank<Idx>(sizes, std::forward<Args>(args)...);
		}
		else {
			return ZeroRankHelper<Idx + 1, Size>::process(iter, sizes, std::forward<Args>(args)...);
		}
	}
};


template <Int Idx>
struct ZeroRankHelper<Idx, Idx> {
	template <typename Iter, typename Position, typename... Args>
	static auto process(const Iter*, Position&&, Args&&...)
	{
		return typename std::remove_reference<Position>::type();
	}
};






template <typename PathList, template <typename> class AccumItemH> struct ExpectedSizesHelper;

template <typename Path, typename... Tail, template <typename> class AccumItemH>
struct ExpectedSizesHelper<TL<Path, Tail...>, AccumItemH> {
	template <typename Accumulator, typename Position>
	static void process(Accumulator&& branch_prefix, Position& values, Int stream = 1)
	{
		using StreamPath = BTTLSizePath<Path>;
		const Int index  = BTTLSizePathBlockIdx<Path>::Value;

		values[stream] = AccumItemH<StreamPath>::value(index, branch_prefix);

		ExpectedSizesHelper<TL<Tail...>, AccumItemH>::process(branch_prefix, values, stream + 1);
	}
};


template <template <typename> class AccumItemH>
struct ExpectedSizesHelper<TL<>, AccumItemH> {
	template <typename Accumulator, typename Position>
	static void process(const Accumulator& branch_prefix, Position& values, Int stream = 1)
	{}
};


}


template <typename Iterator, typename Container>
class BTTLIteratorPrefixCache: public bt::BTree2IteratorPrefixCache<Iterator, Container> {

	using Base 		= bt::BTree2IteratorPrefixCache<Iterator, Container>;
	using Position 	= typename Container::Types::Position;

	static const Int Streams = Container::Types::Streams;

	using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;

	Position data_size_;
	Position data_pos_;

	LeafPrefixRanks ranks_;

public:
	using MyType = BTTLIteratorPrefixCache<Iterator, Container>;

	Position& data_size() {
		return data_size_;
	}

	const Position& data_size() const {
		return data_size_;
	}

	auto data_size(Int idx) const {
		return data_size_[idx];
	}

	Position& data_pos() {
		return data_pos_;
	}

	const Position& data_pos() const {
		return data_pos_;
	}

	auto data_pos(Int idx) const {
		return data_pos_[idx];
	}

	LeafPrefixRanks& ranks() {
		return ranks_;
	}

	const LeafPrefixRanks& ranks() const {
		return ranks_;
	}

    bool operator==(const MyType& other) const
    {
    	return data_size_ == other.data_size_ && Base::operator==(other);
    }

    bool operator!=(const MyType& other) const
	{
    	return data_size_ != other.data_size_ || Base::operator!=(other);
    }
};



template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTTLIteratorPrefixCache<I, C>& cache)
{
    out<<"BTTLIteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes();
    out<<", Size Prefixes: "<<cache.size_prefix();
    out<<", Data Size: "<<cache.data_size();
    out<<", Data Pos: "<<cache.data_pos();
    out<<"]";

    return out;
}



}
}

#endif
