
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

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
	typedef typename Base::Container                                                Container;

    typedef typename Container::Page                                          		PageType;
    typedef typename Container::ID                                            		ID;

    typedef typename Container::Types::DataPage                                 	DataPage;
    typedef typename Container::Types::DataPageG                                 	DataPageG;
    typedef typename Base::Container::Allocator										Allocator;

    static const Int Indexes = Container::Indexes;

    BigInt          local_pos_;


    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;


    IterPart(): Base(), local_pos_(0) {}

    IterPart(ThisPartType&& other): Base(std::move(other)), local_pos_(other.local_pos_) {}

    IterPart(const ThisPartType& other): Base(other), local_pos_(other.local_pos_) {}

    void Assign(const ThisPartType& other)
    {
    	Base::Assign(other);

    	local_pos_    	= other.local_pos_;
    }

    void Assign(ThisPartType&& other)
    {
    	Base::Assign(std::move(other));

    	local_pos_    	= other.local_pos_;
    }

    bool IsEqual(const ThisPartType& other) const
    {
    	return local_pos_  == other.local_pos_ && Base::IsEqual(other);
    }

    bool IsNotEqual(const ThisPartType& other) const
    {
    	return local_pos_  != other.local_pos_ || Base::IsNotEqual(other);
    }

    bool IsEof() const
    {
    	return me()->data().is_set() ? me()->data_pos() >= me()->data()->size() : true;
    }

    DataPageG& data()
    {
    	return me()->path().data();
    }

    const DataPageG& data() const
    {
    	return me()->path().data();
    }

    BigInt &data_pos() {
    	return local_pos_;
    }

    const BigInt data_pos() const {
    	return local_pos_;
    }

    BigInt pos() const
    {
    	return (me()->prefix() + me()->data_pos()) / me()->GetElementSize();
    }


    bool NextKey()
    {
    	me()->data_pos() 	= 0;

    	return Base::NextKey();
    }

    bool PrevKey()
    {
    	me()->data_pos() 	= 0;

    	return Base::PrevKey();
    }

MEMORIA_ITERATOR_PART_END

}

#endif
