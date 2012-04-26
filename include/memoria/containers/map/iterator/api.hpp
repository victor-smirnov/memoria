
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


MEMORIA_ITERATOR_PART_BEGIN(memoria::models::idx_map::ItrApiName)


	typedef typename Base::Container::Key                                    	Key;
	typedef typename Base::Container::Value                                  	Value;

	Key key() const {
		return me()->cache().prefix() + me()->GetRawKey(0);
	}

MEMORIA_ITERATOR_PART_END

}

#endif
