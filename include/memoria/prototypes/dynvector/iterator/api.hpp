
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_API_H
#define MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_API_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/dynvector/names.hpp>



namespace memoria    {

using namespace memoria::btree;
using namespace memoria::dynvector;


MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dynvector::IteratorAPIName)

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Page                                                PageType;
    typedef typename Container::ID                                                  ID;

    typedef typename Container::Types::DataPage                                     DataPage;
    typedef typename Container::Types::DataPageG                                    DataPageG;
    typedef typename Base::Container::Allocator                                     Allocator;

    static const Int Indexes = Container::Indexes;

    BigInt          local_pos_;


    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;


    IterPart(): Base(), local_pos_(0) {}

    IterPart(ThisPartType&& other): Base(std::move(other)), local_pos_(other.local_pos_) {}

    IterPart(const ThisPartType& other): Base(other), local_pos_(other.local_pos_) {}

    void assign(const ThisPartType& other)
    {
        Base::assign(other);

        local_pos_      = other.local_pos_;
    }

    void assign(ThisPartType&& other)
    {
        Base::assign(std::move(other));

        local_pos_      = other.local_pos_;
    }

    bool isEqual(const ThisPartType& other) const
    {
        return local_pos_  == other.local_pos_ && Base::isEqual(other);
    }

    bool isNotEqual(const ThisPartType& other) const
    {
        return local_pos_  != other.local_pos_ || Base::isNotEqual(other);
    }

    bool isEof() const
    {
        return me()->data().isSet() ? me()->dataPos() >= me()->data()->size() : true;
    }

    DataPageG& data()
    {
        return me()->path().data();
    }

    const DataPageG& data() const
    {
        return me()->path().data();
    }

    BigInt &dataPos() {
        return local_pos_;
    }

    const BigInt dataPos() const {
        return local_pos_;
    }

    BigInt pos() const
    {
        return (me()->prefix() + me()->dataPos()) / me()->getElementSize();
    }


    bool nextKey()
    {
        me()->dataPos()     = 0;

        return Base::nextKey();
    }

    bool prevKey()
    {
        me()->dataPos()     = 0;

        return Base::prevKey();
    }

MEMORIA_ITERATOR_PART_END

}

#endif
