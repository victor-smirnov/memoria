
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

MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::models::array::IteratorToolsName)

    typedef typename Base::NodeBase                                             	NodeBase;
    typedef typename Base::Container                                                Container;
    typedef typename Container::Key                                                 Key;
    typedef typename Container::Value                                               Value;

    typedef typename Container::Types::DataPage                                 	DataPage;
    typedef typename Container::Types::Buffer                                   	Buffer;
    typedef typename Container::Types::BufferContentDescriptor                 		BufferContentDescriptor;
    typedef typename Container::Types::CountData                                	CountData;

    static const Int Indexes = Container::Indexes;

    BigInt          idx_;
    DataPage*   data_;

    IterPart(MyType &me): Base(me), me_(me), idx_(0), data_(NULL)
    {

    }

    DataPage*& data() {
        return data_;
    }

    const DataPage*& data() const {
        return data_;
    }

    BigInt &idx() {
        return idx_;
    }

    const BigInt &idx() const {
        return idx_;
    }

    CountData get_base_prefix() {
        
    }

    void setup(const MyType &other)
    {
        Base::setup(other);

        idx_    = other.idx_;
        data_   = other.data_;
    }

MEMORIA_ITERATOR_PART_END

}



#endif
