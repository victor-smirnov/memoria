
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
	typedef typename Base::Container                                                Container;

    typedef typename Container::ApiKeyType                                    		ApiKeyType;
    typedef typename Container::ApiValueType                                  		ApiValueType;

    typedef typename Container::Page                                          		PageType;
    typedef typename Container::ID                                            		ID;

    typedef typename Container::Types::DataPage                                 	DataPage;

    static const Int Indexes = Container::Indexes;

    BigInt          local_pos_;
    DataPage*   	data_;

    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;

    class SumWalker {
    	Int idx_;
    	Int key_;
    	BigInt sum_;
    	MyType& me_;

    public:
    	SumWalker(Int key, MyType& me): key_(key), sum_(0), me_(me) {}

    	BigInt sum() const {
    		return sum_;
    	}

    	void operator()(NodeBase* node, Int idx)
    	{
    		idx_ = idx;
    		Base::Container::NodeDispatcher::Dispatch(node, *this);
    	}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		for (Int c = 0; c < idx_; c++)
    		{
    			sum_ += node->map().key(key_, c);
    		}
    	}
    };

    IterPart(MyType &me): Base(me), me_(me), local_pos_(0), data_(NULL)
    {

    }

    bool IsEof()
    {
    	return me_.data() != NULL ? me_.data_pos() >= me_.data()->data().size() : true;
    }

    DataPage*& data() {
    	return data_;
    }

    const DataPage* data() const {
    	return data_;
    }

    BigInt &data_pos() {
    	return local_pos_;
    }

    const BigInt data_pos() const {
    	return local_pos_;
    }

    BigInt pos()
    {
    	return me_.prefix(0) + me_.data_pos();
    }

    void setup(const MyType &other)
    {
    	Base::setup(other);

    	local_pos_    	= other.local_pos_;
    	data_   		= other.data_;
    }


    BigInt GetIndexValue(Int idx_number)
    {
    	if (me_.page() != NULL)
    	{
    		SumWalker walker(idx_number, me_);
    		me_.walk_to_the_root(me_.page(), me_.key_idx(), walker);
    		return walker.sum();
    	}
    	else {
    		return 0;
    	}
    }


    DataPage *GetNextDataPage(NodeBase* page, DataPage* data)
    {
    	Int parent_idx = data->parent_idx();
    	Int children_count = me_.model().GetChildrenCount(page);
    	MEMORIA_TRACE(me_.model(), page->id(), data->id(), parent_idx, children_count);
    	if (parent_idx < children_count - 1)
    	{
    		return me_.model().GetDataPage(page, parent_idx + 1);
    	}
    	else {
    		page = me_.GetNextNode(page);
    		if (page != NULL)
    		{
    			return me_.model().GetDataPage(page, 0);
    		}
    		else {
    			return NULL;
    		}
    	}
    }

    DataPage *GetNextDataPage()
    {
    	return me_.GetNextDataPage(me_.page(), me_.data());
    }

    DataPage *GetPrevDataPage(NodeBase* page, DataPage* data)
    {
    	Int parent_idx = data->parent_idx();

    	if (parent_idx > 0)
    	{
    		return me_.model().GetDataPage(page, parent_idx - 1);
    	}
    	else {
    		page = me_.GetPrevNode(page);
    		if (page != NULL)
    		{
    			Int children_count = me_.model().GetChildrenCount(page);
    			return me_.model().GetDataPage(page, children_count - 1);
    		}
    		else {
    			return NULL;
    		}
    	}
    }

    DataPage *GetPrevDataPage()
    {
    	return me_.GetPrevDataPage(me_.page(), me_.data());
    }

MEMORIA_ITERATOR_PART_END

}

#endif
