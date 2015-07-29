
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



template <typename CtrT, typename Rng>
class RandomDataInputProvider: public memoria::bt::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {

	using Base = memoria::bt::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

public:

	using Position 		= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;
	using Buffer 		= typename Base::Buffer;
	using Tuple 		= typename Base::Tuple;

	using NodeBaseG		= typename Base::NodeBaseG;

	template <Int StreamIdx>
	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<StreamIdx>;

	template <Int StreamIdx>
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

private:
	CtrSizeT keys_;
	CtrSizeT mean_data_size_;
	CtrSizeT data_size_cnt_ 	= 0;
	CtrSizeT data_size_limit_ 	= 0;

	CtrSizeT key_ = 0;

	Rng rng_;
public:
	RandomDataInputProvider(CtrT& ctr, CtrSizeT keys, CtrSizeT mean_data_size, const Rng& rng):
		Base(ctr, Position({1000, 10000})),
		keys_(keys), mean_data_size_(mean_data_size),
		rng_(rng)
	{}


	virtual Int get(Tuple& value)
	{
		if (key_ <= keys_)
		{
			int sym;

			if (key_ > 0)
			{
				sym = (data_size_cnt_++ < data_size_limit_ ? 1 : 0);
			}
			else {
				sym = 0;
			}

			if (sym == 0)
			{
				data_size_cnt_ = 0;
				data_size_limit_ = (rng_() % (mean_data_size_*2 + 1));
				cout<<"Limit: "<<data_size_limit_<<endl;


				if (++key_ <= keys_)
				{
					std::get<0>(value) = InputTupleAdapter<0>::convert(memoria::core::StaticVector<BigInt, 2>({1, 0}));
				}
				else {
					return -1;
				}
			}
			else {
				std::get<1>(value) = InputTupleAdapter<1>::convert(key_ % 256);
			}

			return sym;
		}
		else {
			return -1;
		}
	}

};



}
}

#endif
