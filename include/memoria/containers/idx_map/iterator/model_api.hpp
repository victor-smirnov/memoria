
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_ITERATOR_MODEL_API_HPP
#define _MEMORIA_MODELS_IDX_MAP_ITERATOR_MODEL_API_HPP

#include <iostream>


#include <memoria/core/types/types.hpp>

#include <memoria/containers/idx_map/names.hpp>
#include <memoria/core/container/iterator.hpp>




namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::models::idx_map::ItrAPIName)

    typedef typename Base::NodeBase                                             	NodeBase;

    typedef typename Base::Container::ApiKeyType                                    ApiKeyType;
    typedef typename Base::Container::ApiValueType                                  ApiValueType;

    typedef typename Base::Container::Page                                          PageType;
    typedef typename Base::Container::ID                                            ID;

    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;

    virtual BigInt GetBase(Int i) {
        return me_.prefix(i);
    }
    

MEMORIA_ITERATOR_PART_END

}

#endif
