
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
    DataPageG   	data_;

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

    IterPart(): Base(), local_pos_(0), data_(NULL) {}

    IterPart(ThisPartType&& other): Base(std::move(other)), local_pos_(other.local_pos_), data_(std::move(other.data_)) {}

    IterPart(const ThisPartType& other): Base(other), local_pos_(other.local_pos_), data_(other.data_) {}

    void operator=(const ThisPartType& other)
    {
    	Base::operator=(other);

    	local_pos_    	= other.local_pos_;
    	data_   		= other.data_;
    }

    void operator=(ThisPartType&& other)
    {
    	Base::operator=(std::move(other));

    	local_pos_    	= other.local_pos_;
    	data_   		= std::move(other.data_);
    }

    void SetupAllocator(Allocator* allocator)
    {
    	//data_.set_allocator(allocator);
    	Base::SetupAllocator(allocator);
    }

    bool IsEof()
    {
    	return me()->data() != NULL ? me()->data_pos() >= me()->data()->data().size() : true;
    }

    DataPageG& data() {
    	return data_;
    }

    const DataPageG& data() const {
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
    	return me()->prefix(0) + me()->data_pos();
    }




    BigInt GetIndexValue(Int idx_number)
    {
    	if (me()->page() != NULL)
    	{
    		SumWalker walker(idx_number, *me());
    		me()->walk_to_the_root(me()->page(), me()->key_idx(), walker);
    		return walker.sum();
    	}
    	else {
    		return 0;
    	}
    }


    DataPageG GetNextDataPage(NodeBaseG& page, DataPageG& data)
    {
    	Int parent_idx = data->parent_idx();
    	Int children_count = page->children_count();

    	if (parent_idx < children_count - 1)
    	{
    		return me()->model().GetDataPage(page, parent_idx + 1, Allocator::READ);
    	}
    	else {
    		NodeBaseG next_node = me()->GetNextNode(page);
    		if (next_node.is_set())
    		{
    			return me()->model().GetDataPage(next_node, 0, Allocator::READ);
    		}
    		else {
    			return DataPageG();
    		}
    	}
    }

    DataPageG GetNextDataPage()
    {
    	return me()->GetNextDataPage(me()->page(), me()->data());
    }

    DataPageG GetPrevDataPage(NodeBaseG& page, DataPageG& data)
    {
    	Int parent_idx = data->parent_idx();

    	if (parent_idx > 0)
    	{
    		return me()->model().GetDataPage(page, parent_idx - 1, Allocator::READ);
    	}
    	else {
    		NodeBaseG prev_node = me()->GetPrevNode(page);
    		if (prev_node.is_set())
    		{
    			Int children_count = prev_node->children_count();
    			return me()->model().GetDataPage(prev_node, children_count - 1, Allocator::READ);
    		}
    		else {
    			return DataPageG();
    		}
    	}
    }

    DataPageG GetPrevDataPage()
    {
    	return me()->GetPrevDataPage(me()->page(), me()->data());
    }

////FIXME: dont't forget prefixes
//    bool NextData()
//    {
//    	if (me()->NextKey())
//    	{
//    		me()->data() = me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
//    		me()->data_pos() = me()->data()->data().size();
//    		return true;
//    	}
//    	return false;
//    }
//
//    bool PrevData()
//    {
//    	if (me()->PrevKey())
//    	{
//    		me()->data() = me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
//    		me()->data_pos() = me()->data()->data().size();
//    		return true;
//    	}
//    	return false;
//    }

//    bool NextKey()
//    {
//    	bool result = Base::NextKey();
//    	if (Next)
//    }
//
//    bool PrevKey()
//    {
//    	bool result = Base::PrevKey();
//    	if (result)
//    	{
//    		for (Int c = 0; c < Indexes; c++)
//    		{
//    			me()->prefix(c) -= me()->model().GetKey(me()->page(), c, me()->key_idx());
//    		}
//    	}
//    }

MEMORIA_ITERATOR_PART_END

}

#endif
