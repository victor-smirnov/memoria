
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_vctr_TOOLS_HPP
#define _MEMORIA_CONTAINERS_vctr_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt_ss/btss_input.hpp>

namespace memoria       {
namespace mvector       {


template <typename CtrT, typename InputIterator, Int EntryBufferSize = 1000>
class VectorIteratorInputProvider: public memoria::btss::AbstractIteratorBTSSInputProvider<
	CtrT,
	VectorIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
	InputIterator
>
{
	using Base = memoria::btss::AbstractIteratorBTSSInputProvider<
			CtrT,
			VectorIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
			InputIterator
	>;

public:

	using typename Base::CtrSizeT;

public:
	VectorIteratorInputProvider(CtrT& ctr, const InputIterator& start, const InputIterator& end, Int capacity = 10000):
		Base(ctr, start, end, capacity)
	{}

	auto buffer(StreamTag<0>, StreamTag<0>, Int idx, Int block) {
		return CtrSizeT();
	}

	const auto& buffer(StreamTag<0>, StreamTag<1>, Int idx, Int block) {
		return Base::input_value_buffer_[idx];
	}
};





}
}

#endif
