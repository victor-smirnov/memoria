
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_PROTOTYPES_BTSS_INPUT_HPP_
#define MEMORIA_PROTOTYPES_BTSS_INPUT_HPP_



#include <memoria/core/types/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

namespace memoria 	{
namespace btss 		{


template <Int StreamIdx>
struct InputTupleSizeH {
	template <typename Tuple>
	static auto get(Tuple&& buffer, Int idx) -> Int {
		return 0;
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{}
};


template <Int StreamIdx>
struct LeafStreamSizeH {
	template <typename Stream, typename SizeT>
	static void stream(Stream* buffer, Int idx, SizeT value)
	{}
};



template <typename CtrT>
class AbstractBTSSInputProvider: public AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {
	using Base = AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

public:



	using Buffer 	= typename Base::Buffer;
	using CtrSizeT	= typename Base::CtrSizeT;
	using Position	= typename Base::Position;
	using NodeBaseG	= typename Base::NodeBaseG;


	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

public:
	AbstractBTSSInputProvider(CtrT& ctr, const Position& capacity): Base(ctr, capacity) {}

	// must be an abstract method
	virtual bool get(InputTuple& value) = 0;

	virtual Position findCapacity(const NodeBaseG& leaf, const Position& sizes)
	{
		Int capacity = this->ctr_.getLeafNodeCapacity(leaf, 0);

		if (capacity > sizes[0])
		{
			return sizes;
		}
		else {
			return Position(capacity);
		}
	}

	virtual bool populate_buffer()
	{
		Int cnt = 0;

		this->start_.clear();
		this->size_.clear();

		Int capacity = this->buffer_capacity()[0];

		while (true)
		{
			InputTuple value;

			if (get(std::get<0>(this->buffer_)[cnt]))
			{
				if (cnt++ == capacity)
				{
					break;
				}
			}
			else {
				break;
			}
		}

		this->size_[0] = cnt;

		return cnt > 0;
	}

private:
	virtual Int populate (Buffer& buffer) {return -1;}
};



}
}






#endif
