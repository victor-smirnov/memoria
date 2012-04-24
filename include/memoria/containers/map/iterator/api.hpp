
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_ITERATOR_API_HPP
#define _MEMORIA_MODELS_IDX_MAP_ITERATOR_API_HPP

#include <iostream>


#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/names.hpp>
#include <memoria/core/container/iterator.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::models::idx_map::ItrApiName)


	typedef typename Base::Container::Key                                    	Key;
	typedef typename Base::Container::Value                                  	Value;

    IterPart(): Base(), prefix_(0) {}
	IterPart(ThisPartType&& other): Base(std::move(other)), prefix_(other.prefix_) {}
	IterPart(const ThisPartType& other): Base(other), prefix_(other.prefix_) {}

	Key prefix_;


	void Assign(ThisPartType&& other)
	{
		prefix_     = other.prefix_;
		Base::Assign(std::move(other));
	}

	void Assign(const ThisPartType& other)
	{
		prefix_     = other.prefix_;
		Base::Assign(other);
	}


	Key GetKey1() const {
		return prefix_ + me()->GetRawKey(0);
	}

	void SetupPrefix(Key prefix)
	{
		prefix_ = prefix;
	}

MEMORIA_ITERATOR_PART_END

}

#endif
