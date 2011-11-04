
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_ITERATOR_TOOLS_HPP
#define _MEMORIA_MODELS_ARRAY_ITERATOR_TOOLS_HPP

#include <iostream>

#include <memoria/containers/array/names.hpp>
#include <memoria/core/container/iterator.hpp>



namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::models::array::IteratorToolsName)

    typedef typename Base::NodeBase                                             	NodeBase;
    typedef typename Base::Container                                                Container;
    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

//    typedef typename Container::Types::DataPage                                 	DataPage;
//    typedef typename Container::Types::Buffer                                   	Buffer;
//    typedef typename Container::Types::BufferContentDescriptor                 		BufferContentDescriptor;
    typedef typename Container::Types::CountData                                	CountData;

MEMORIA_ITERATOR_PART_END

}



#endif
