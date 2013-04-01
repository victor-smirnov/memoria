
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_CTRWRAPPER_ITER_BASE_HPP
#define _MEMORIA_PROTOTYPES_CTRWRAPPER_ITER_BASE_HPP

#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>

namespace memoria {

MEMORIA_BALTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(CtrWrapperIteratorBase)

	typedef TypesType 															Types;
	typedef Ctr<typename Types::CtrTypes> 										Container;
	typedef typename Container::WrappedCtr 										WrappedCtr;
	typedef typename WrappedCtr::Iterator										WrappedIter;


	CtrWrapperIteratorBase():
		Base()
	{

	}

	CtrWrapperIteratorBase(ThisType&& other):
		Base(std::move(other))
	{

	}

	CtrWrapperIteratorBase(const ThisType& other): Base(other)
	{

	}

	void assign(ThisType&& other)
	{
		Base::assign(std::move(other));
	}

	void assign(const ThisType& other)
	{
		Base::assign(other);
	}

	bool isEqual(const ThisType& other) const
	{
		return Base::isEqual(other);
	}

	bool isNotEqual(const ThisType& other) const
	{
		return Base::isNotEqual(other);
	}

	void dump(ostream& out = cout, const char* header = NULL)
	{

	}

MEMORIA_BTREE_ITERATOR_BASE_CLASS_END;

}


#endif /* CTRWRP_I_BASE_HPP_ */
