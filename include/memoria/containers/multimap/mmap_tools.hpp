
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria {
namespace mmap    {

template <Int StreamIdx> struct InputTupleSizeH;

template <>
struct InputTupleSizeH<0> {
	template <typename Tuple>
	static auto get(Tuple&& buffer, Int idx) -> decltype(std::get<0>(buffer[idx])[1]) {
		return std::get<0>(buffer[idx])[1];
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{
		std::get<0>(buffer[idx])[1] += value;
	}
};


template <>
struct InputTupleSizeH<1> {
	template <typename Tuple>
	static Int get(Tuple&& buffer, Int idx) {
		return 0;
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{}
};


template <Int StreamIdx> struct LeafStreamSizeH;

template <>
struct LeafStreamSizeH<0> {
	template <typename Stream, typename SizeT>
	static void stream(Stream* buffer, Int idx, SizeT value)
	{
		buffer->value(1, idx) += value;
	}
};




template <typename Iterator, typename Container>
class MultimapIteratorPrefixCache: public bt::BTree2IteratorPrefixCache<Iterator, Container> {

	using Base = bt::BTree2IteratorPrefixCache<Iterator, Container>;
	using CtrSizeT = typename Container::Types::CtrSizeT;

	CtrSizeT data_size_ = 0;
	CtrSizeT data_pos_ 	= 0;

public:
	using MyType = MultimapIteratorPrefixCache<Iterator, Container>;

	CtrSizeT& data_size() {
		return data_size_;
	}

	const CtrSizeT& data_size() const {
		return data_size_;
	}

	CtrSizeT& data_pos() {
		return data_pos_;
	}

	const CtrSizeT& data_pos() const {
		return data_pos_;
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
std::ostream& operator<<(std::ostream& out, const MultimapIteratorPrefixCache<I, C>& cache)
{
    out<<"MitimapIteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes()<<", Size Prefixes: "<<cache.size_prefix();
    out<<", Data Size: "<<cache.data_size();
    out<<", Data Pos: "<<cache.data_pos();
    out<<"]";

    return out;
}



template <typename CtrT>
class RandomDataInputProvider: public memoria::bt::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {

	using Base = memoria::bt::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

public:
	using Position 		= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;
	using InputBuffer 	= typename CtrT::Types::InputBuffer;

	using NodeBaseG		= typename Base::NodeBaseG;

private:
	CtrSizeT keys_;
	CtrSizeT mean_data_size_;


	CtrSizeT buffer_size_ = 0;
	CtrSizeT pos_ = 0;

	CtrSizeT consumed_keys_ = 0;
	CtrSizeT consumed_data_ = 0;

public:
	RandomDataInputProvider(CtrT& ctr, CtrSizeT keys, CtrSizeT mean_data_size):
		Base(ctr, 10000),
		keys_(keys), mean_data_size_(mean_data_size)
	{}

	virtual bool hasData() {
		return false;
	}

	virtual Position fill(NodeBaseG& leaf, const Position& from)	{
		return Position();
	}
};



}
}

#endif
