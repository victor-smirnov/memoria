
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_ITERATOR_MODEL_API_HPP
#define _MEMORIA_MODELS_ARRAY_ITERATOR_MODEL_API_HPP

#include <iostream>

#include <memoria/containers/array/names.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>


namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::models::array::IteratorContainerAPIName)

    typedef typename Base::NodeBase                                             	NodeBase;
    typedef typename Base::Container                                                Container;

    typedef typename Base::Container::ApiKeyType                                    ApiKeyType;
    typedef typename Base::Container::ApiValueType                                  ApiValueType;
    typedef typename Container::Key                                                 Key;

    typedef typename Base::Container::Page                                          PageType;
    typedef typename Base::Container::ID                                            ID;

    typedef typename Container::Types::DataPage                                		DataPage;
    typedef typename Container::Types::Buffer                                   	Buffer;
    typedef typename Container::Types::BufferContentDescriptor                  	BufferContentDescriptor;
    typedef typename Container::Types::CountData                               	 	CountData;
    typedef typename Container::Types::Pages::NodeDispatcher                        NodeDispatcher;


    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;


    virtual BigInt Read(ArrayData& data, BigInt start, BigInt len) {return 0;}
    
    virtual void Insert(ArrayData& data, BigInt start, BigInt len);

    virtual void Update(ArrayData& data, BigInt start, BigInt len) {}

    virtual void Remove(BigInt len)
    {
    	MyType to = me_.model().Seek(len);
    	me_.model().RemoveDataBlock(me_, to);
    }


    virtual BigInt Skip(BigInt distance);
    virtual BigInt SkipFw(BigInt distance);
    virtual BigInt SkipBw(BigInt distance);

    virtual void DumpState(const char* str)
    {
    	BigInt offset = me_.GetIndexValue(0) + me_.idx();
    	MEMORIA_INFO(me_.model(), str, "[node_id key_idx data_id page_offset offset bof eof empty]", (me_.page() != NULL ? me_.page()->id() : ID(0)), me_.key_idx(), (me_.data() != NULL ? me_.data()->id() : ID(0)), me_.idx(), offset, me_.IsBof(), me_.IsEof(), me_.IsEmpty());
    }

    virtual BigInt GetBlobId() {return 0;}

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::models::array::IteratorContainerAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::Insert(ArrayData& data, BigInt start, BigInt len)
{
	BufferContentDescriptor descriptor;

	descriptor.start() = start;
	descriptor.length() = len;

	me_.model().InsertDataBlock(me_, data, descriptor);
}

M_PARAMS
BigInt M_TYPE::Skip(BigInt distance)
{
	if (distance > 0)
	{
		return SkipFw(distance);
	}
	else {
		return SkipBw(-distance);
	}
}

M_PARAMS
BigInt M_TYPE::SkipFw(BigInt distance)
{
	MEMORIA_TRACE(me_.model(), "Begin", distance);

	//FIXME: handle BOF properly
	if (me_.IsEmpty())
	{
		return 0;
	}
	else {
		Int data_size = me_.data()->data().size();
		Int idx = me_.idx();

		if (distance + idx <= data_size)
		{
			MEMORIA_TRACE(me_.model(), "Within the page");
			//FIXME: reset EOF if necessary

			// A trivial case when the offset is within current data page

			// we need to check for EOF if a data page
			// is the last in the index node
			if (distance + idx == data_size)
			{
				if (me_.key_idx() == me_.model().GetChildrenCount(me_.page()) - 1)
				{
					NodeBase* next = me_.GetNextNode(me_.page());
					if (next == NULL)
					{
						me_.SetEof(true);
					}
					else {
						me_.idx() = 0;
					}
				}
				else {
					me_.key_idx()++;
					me_.idx() 	= 0;
					me_.data() 	= me_.model().GetDataPage(me_.page(), me_.key_idx());
				}
			}
			else {
				me_.idx() += distance;
			}
		}
		else
		{
			NodeTreeWalker<Container, Key, true> walker(distance + idx, me_.model());

			bool end 	= me_.WalkFw(me_.page(), me_.key_idx(), walker);
			me_.data() 	= me_.model().GetDataPage(me_.page(), me_.key_idx());

			//FIXME: BOF
			me_.SetEof(end);
			me_.SetBof(false);

			if (end)
			{
				me_.idx() 	= me_.data()->data().size();
				MEMORIA_TRACE(me_, walker.target_, walker.sum_ - idx);
				return walker.sum() - idx;
			}
			else {
				me_.idx() 	= walker.remainder();
			}
		}
	}

	//FIXME: return thrue distance
	return distance;
}


M_PARAMS
BigInt M_TYPE::SkipBw(BigInt distance)
{
	MEMORIA_TRACE(me_.model(), "Begin", distance);

	//FIXME: handle EOF properly
	if (me_.IsEmpty())
	{
		return 0;
	}
	else {
		Int idx = me_.idx();

		if (distance <= me_.idx())
		{
			//FIXME: reset EOF if necessary

			// A trivial case when the offset is within current data page
			// we need to check for BOF if a data page
			// is the fist in the index node
			if (me_.key_idx() == 0 && distance == idx)
			{
				NodeBase* prev = me_.GetPrevNode(me_.page());
				me_.SetBof(prev == NULL);
			}

			me_.idx() 	-= distance;
		}
		else
		{
			Int data_size = me_.data()->data().size();
			Int to_add = data_size - idx;
			NodeTreeWalker<Container, Key, false> walker(distance + to_add, me_.model());

			bool end 	= me_.WalkBw(me_.page(), me_.key_idx(), walker);
			me_.data() 	= me_.model().GetDataPage(me_.page(), me_.key_idx());

			//FIXME: EOF
			me_.SetEof(false);
			me_.SetBof(end);

			if (end)
			{
				me_.idx() 	= 0;
				MEMORIA_TRACE(me_, walker.target_, walker.sum_ - to_add);
				return walker.sum() - to_add;
			}
			else {
				me_.idx()	= me_.data()->data().size() - walker.remainder();
			}
		}

		return distance;
	}
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
