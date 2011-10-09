
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_TOOLS_H
#define MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_TOOLS_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/dynvector/names.hpp>




namespace memoria    {

using namespace memoria::btree;
using namespace memoria::dynvector;


MEMORIA_ITERATOR_PART_BEGIN(memoria::dynvector::IteratorToolsName)

    typedef typename Base::NodeBase                                             	NodeBase;

    typedef typename Base::Container::ApiKeyType                                    ApiKeyType;
    typedef typename Base::Container::ApiValueType                                  ApiValueType;

    typedef typename Base::Container::Page                                          PageType;
    typedef typename Base::Container::ID                                            ID;

    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;

MEMORIA_ITERATOR_PART_END

}


#endif
