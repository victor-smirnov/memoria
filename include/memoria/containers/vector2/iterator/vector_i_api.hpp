
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

	BigInt skipFw(BigInt amount);
	BigInt skipBw(BigInt amount);

	BigInt pos() const {
		return 0;
	}

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mvector2::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::skipFw(BigInt amount)
{
//	typedef mvector2::FindLTForwardWalker<Types> Walker;
//
//	auto& self = this->self();
//
//	Walker walker(amount, 0);
//
//	self.model().findFw(self.path(), self.key_idx(), walker, level);
//
//	return std::get<0>(walker.prefix())[0] + walker.idx();

	return 0;
}

M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
	return 0;
}


}

#undef M_TYPE
#undef M_PARAMS

#endif
