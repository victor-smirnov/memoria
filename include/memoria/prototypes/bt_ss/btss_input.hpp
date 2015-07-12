
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

template <typename CtrT>
class AbstractBTSSInputProvider: public AbstractCtrInputProvider<CtrT, 1, CtrT::Types::LeafDataLength> {
	using Base = AbstractCtrInputProvider<CtrT, 1, CtrT::Types::LeafDataLength>;

public:

	using LeafType = typename Base::LeafType;
	using Position = typename Base::Position;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

public:
	AbstractBTSSInputProvider(CtrT& ctr): Base(ctr) {}

	virtual bool hasData() const {
		return false;
	};

	virtual Position fill(LeafType* leaf, const Position& from)
	{

	}
};



}
}






#endif
