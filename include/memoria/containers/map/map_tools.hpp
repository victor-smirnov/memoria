
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt_ss/btss_input.hpp>

namespace memoria       {
namespace map           {

template <typename CtrT, typename EntryProvider, Int EntryBufferSize = 1000>
class MapEntryInputProvider: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

	using typename Base::CtrSizeT;
	using typename Base::Position;
	using typename Base::InputBuffer;

	using Entry		= typename CtrT::Types::Entry;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;



	EntryProvider provider_;


	Int input_start_ = 0;
	Int input_size_ = 0;
	static constexpr Int INPUT_END = EntryBufferSize;

	InputTuple input_value_buffer_[INPUT_END];

public:
	MapEntryInputProvider(CtrT& ctr, Int capacity = 10000):
		Base(ctr, capacity)
	{}

	MapEntryInputProvider(CtrT& ctr, const EntryProvider& provider, Int capacity = 10000):
		Base(ctr, capacity),
		provider_(provider)
	{}

	virtual Int get(InputBuffer* buffer, Int pos)
	{
		if (input_start_ == input_size_)
		{
			input_start_ = 0;

			for (input_size_ = 0 ; provider_.has_next() && input_size_ < INPUT_END; input_size_++)
			{
				auto pair = provider_.next();

				using Key = decltype(pair.first);
//				using Value = decltype(pair.second);

				input_value_buffer_[input_size_] = InputTupleAdapter::convert(
						0,
						memoria::core::StaticVector<Key, 1>{pair.first},
						//memoria::core::StaticVector<Value, 1>{pair.second}
						pair.second
				);
			}
		}

		if (input_start_ < input_size_)
		{
			auto inserted = buffer->append(input_value_buffer_, input_start_, input_size_ - input_start_);

			input_start_ += inserted;

			return inserted;
		}

		return -1;
	}
};




}
}

#endif
