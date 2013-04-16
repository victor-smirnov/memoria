
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTOR2_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_VECTOR2_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::mvector2::ItrApiName)

	typedef Ctr<typename Types::CtrTypes>                      	Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     Value;
	typedef typename Container::Key                                       Key;
	typedef typename Container::Element                                   Element;
	typedef typename Container::Accumulator                               Accumulator;




	void insert(std::vector<Value>& data)
	{
		MemBuffer<Value> buf(data);
		self().model().insert(self(), buf);
	}

	void remove(BigInt size) {}

	std::vector<Value> subVector(BigInt size) {
		return std::vector<Value>();
	}

	BigInt skip(BigInt distance) {
		return 0;
	}

	BigInt pos() const {
		return 0;
	}

MEMORIA_ITERATOR_PART_END

}

#endif
