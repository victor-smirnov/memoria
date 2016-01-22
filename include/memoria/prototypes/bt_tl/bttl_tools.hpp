
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
	static constexpr Int BufferSize = 32;
	static constexpr Int Streams = Position::Indexes;

	CtrSizeT pos_;
	CtrSizeT target_;

	Position sizes_;
	Position prefix_;
	Position indexes_;


	using AnchorValueT 	= core::StaticVector<CtrSizeT, Streams - 1>;
	using AnchorPosT 	= core::StaticVector<Int, Streams - 1>;

	AnchorPosT anchors_;
	AnchorValueT anchor_values_;

	StreamsRankFn(const Position& sizes, const Position& prefix, CtrSizeT target, const AnchorPosT& anchors, const AnchorValueT& anchor_values):
		target_(target),
		sizes_(sizes),
		prefix_(prefix),
		indexes_(),
		anchors_(anchors),
		anchor_values_(anchor_values)
	{
		pos_ = prefix.sum();
	}

	StreamsRankFn(const Position& sizes, const Position& prefix, CtrSizeT target):
		target_(target),
		sizes_(sizes),
		prefix_(prefix),
		indexes_(),
		anchors_(-1)
	{
		pos_ = prefix.sum();
	}

	template <Int StreamIdx, typename NextHelper, typename Node>
	void process(const Node* node) {
		process<StreamIdx, NextHelper>(node, prefix_[StreamIdx], sizes_[StreamIdx]);
	}

	template <Int StreamIdx, typename NextHelper, typename Node>
	void process(const Node* node, CtrSizeT start, CtrSizeT end)
	{
		CtrSizeT buffer[BufferSize];
		for (auto& v: buffer) v = 0;

		Int buffer_pos = 0;
		Int buffer_size = 0;

		const Int size   = sizes_[StreamIdx];
		const Int limit  = end <= size ? end : size;

		for (auto i = start; i < limit; i++)
		{
			if (pos_ < target_)
			{
				auto next_length = this->template buffered_get<StreamIdx>(node, i, buffer_pos, buffer_size, size, buffer);

				indexes_[StreamIdx]++;
				pos_++;

				auto next_offset = prefix_[StreamIdx  + 1] + indexes_[StreamIdx + 1];

				NextHelper::process(node, *this, next_offset, next_offset + next_length);
			}
			else {
				break;
			}
		}
	}






	template <Int StreamIdx, typename Node>
	void processLast(const Node* node) {
		processLast<StreamIdx>(node, prefix_[StreamIdx], sizes_[StreamIdx]);
	}

	template <Int StreamIdx, typename Node, typename... Args>
	void processLast(const Node* node, CtrSizeT start, CtrSizeT end, Args&&...)
	{
		CtrSizeT size = node->template streamSize<StreamIdx>();

		const Int limit = end <= size ? end : size;

		auto delta = target_ - pos_;

		if (start + delta <= limit)
		{
			indexes_[StreamIdx] += delta;
			pos_ += delta;
		}
		else {
			indexes_[StreamIdx] += limit - start;
			pos_ += limit - start;
		}
	}

private:

	template <Int StreamIdx, typename Node>
	CtrSizeT buffered_get(const Node* node, Int idx, Int& p0, Int& s0, Int size, CtrSizeT buffer[BufferSize])
	{
		if (idx < s0) {
			return buffer[idx - p0];
		}
		else {
			Int limit = (idx + BufferSize) < size ? idx + BufferSize : size;

			using Path = StreamsSizesPath<StreamIdx>;

			node->template substream<Path>()->read(0, idx, limit, buffer);

			Int anchor = anchors_[StreamIdx];
			if (anchor >= idx && anchor < limit)
			{
				buffer[anchor - idx] += anchor_values_[StreamIdx];
			}

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
	template <typename Iter, typename Node, typename Position, typename... Args>
	static auto process(const Iter* iter, Node&& node, Position&& sizes, Args&&... args)
	{
		auto size = sizes[Idx];
		if (size > 0)
		{
			return iter->template _streamsrank_<Idx>(std::forward<Node>(node), sizes, std::forward<Args>(args)...);
		}
		else {
			return ZeroRankHelper<Idx + 1, Size>::process(
					iter,
					std::forward<Node>(node),
					sizes,
					std::forward<Args>(args)...
			);
		}
	}
};


template <Int Idx>
struct ZeroRankHelper<Idx, Idx> {
	template <typename Iter, typename Node, typename Position, typename... Args>
	static auto process(const Iter*, Node&&, Position&&, Args&&...)
	{
		return typename std::remove_reference<Position>::type();
	}
};






template <
	Int Size,
	template <Int> class SizesPathT,
	template <typename> class AccumItemH,
	Int StreamIdx = 0
> struct ExpectedSizesHelper
{
	template <typename BranchNodeEntry, typename Position>
	static void process(BranchNodeEntry&& branch_prefix, Position& values, Int stream = 1)
	{
		using Path = SizesPathT<StreamIdx>;

		values[stream] = AccumItemH<Path>::value(0, branch_prefix);

		ExpectedSizesHelper<Size, SizesPathT, AccumItemH, StreamIdx + 1>::process(branch_prefix, values, stream + 1);
	}
};


template <
	Int Size,
	template <Int> class SizesPathT,
	template <typename> class AccumItemH
> struct ExpectedSizesHelper<Size, SizesPathT, AccumItemH, Size>
 {
	template <typename BranchNodeEntry, typename Position>
	static void process(const BranchNodeEntry& branch_prefix, Position& values, Int stream = 1)
	{}
};


}


template <typename Iterator, typename Container>
class BTTLIteratorPrefixCache: public bt::BTreeIteratorPrefixCache<Iterator, Container> {

	using Base 		= bt::BTreeIteratorPrefixCache<Iterator, Container>;
	using Position 	= typename Container::Types::Position;

	static const Int Streams = Container::Types::Streams;

	Position data_size_;
	Position data_pos_;
	Position abs_pos_;



public:
	using MyType = BTTLIteratorPrefixCache<Iterator, Container>;

	BTTLIteratorPrefixCache():
		data_size_(-1),
		data_pos_(-1),
		abs_pos_(-1)
	{}

	Position& data_size() {
		return data_size_;
	}

	const Position& data_size() const {
		return data_size_;
	}

	Position& data_pos() {
		return data_pos_;
	}

	const Position& data_pos() const {
		return data_pos_;
	}

	Position& abs_pos() {
		return abs_pos_;
	}

	const Position& abs_pos() const {
		return abs_pos_;
	}

	void reset() {
		Base::reset();

//		data_pos_ 	= Position(-1);
//		data_size_ 	= Position(-1);
//		abs_pos_ 	= Position(-1);
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
    out<<", Abs Pos: "<<cache.abs_pos();
    out<<"]";

    return out;
}



}
}

#endif
