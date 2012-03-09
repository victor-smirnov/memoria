
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_API_H
#define MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_API_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/vapi/models/logs.hpp>



namespace memoria    {

using namespace memoria::btree;
using namespace memoria::dynvector;


MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dynvector::IteratorAPIName)

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
	typedef typename Base::Container                                                Container;

    typedef typename Container::ApiKeyType                                    		ApiKeyType;
    typedef typename Container::ApiValueType                                  		ApiValueType;

    typedef typename Container::Page                                          		PageType;
    typedef typename Container::ID                                            		ID;

    typedef typename Container::Types::DataPage                                 	DataPage;
    typedef typename Container::Types::DataPageG                                 	DataPageG;
    typedef typename Base::Container::Allocator										Allocator;

    static const Int Indexes = Container::Indexes;

    BigInt          local_pos_;


    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;

//    class SumWalker {
//    	Int idx_;
//    	Int key_;
//    	BigInt sum_;
//    	MyType& me_;
//
//    public:
//    	SumWalker(Int key, MyType& me): key_(key), sum_(0), me_(me) {}
//
//    	BigInt sum() const {
//    		return sum_;
//    	}
//
//    	void operator()(NodeBase* node, Int idx)
//    	{
//    		idx_ = idx;
//    		Base::Container::NodeDispatcher::Dispatch(node, *this);
//    	}
//
//    	template <typename Node>
//    	void operator()(Node* node)
//    	{
//    		for (Int c = 0; c < idx_; c++)
//    		{
//    			sum_ += node->map().key(key_, c);
//    		}
//    	}
//    };

    IterPart(): Base(), local_pos_(0) {}

    IterPart(ThisPartType&& other): Base(std::move(other)), local_pos_(other.local_pos_) {}

    IterPart(const ThisPartType& other): Base(other), local_pos_(other.local_pos_) {}

    void operator=(const ThisPartType& other)
    {
    	Base::operator=(other);

    	local_pos_    	= other.local_pos_;
    }

    void operator=(ThisPartType&& other)
    {
    	Base::operator=(std::move(other));

    	local_pos_    	= other.local_pos_;
    }

    bool IsEof()
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

    BigInt pos()
    {
    	return me()->prefix(0) + me()->data_pos();
    }


    bool NextKey()
    {
    	if (Base::NextKey())
    	{
    		me()->path().data().node() 			= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
    		me()->path().data().parent_idx() 	= me()->key_idx();

    		me()->data_pos() 	= 0;

    		return true;
    	}

    	return false;
    }

    bool PrevKey()
    {
    	if (Base::PrevKey())
    	{
    		me()->data() 						= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
    		me()->path().data().parent_idx() 	= me()->key_idx();

    		me()->data_pos() 	= 0;

    		return true;
    	}

    	return false;
    }

MEMORIA_ITERATOR_PART_END

}

#endif
