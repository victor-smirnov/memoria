
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


    BigInt Read(ArrayData& data, BigInt start, BigInt len);
    BigInt Read(ArrayData& data)
    {
    	return Read(data, 0, data.size());
    }

    
    void Insert(ArrayData& data, BigInt start, BigInt len);

    void Insert(ArrayData& data)
    {
    	Insert(data, 0, data.size());
    }

    void Update(ArrayData& data, BigInt start, BigInt len) {}

    void Remove(BigInt len)
    {
    	MyType to = me_.model().Seek(len);
    	me_.model().RemoveDataBlock(me_, to);
    }


    BigInt Skip(BigInt distance);
    BigInt SkipFw(BigInt distance);
    BigInt SkipBw(BigInt distance);

    void DumpState(const char* str)
    {
    	BigInt offset = me_.GetIndexValue(0) + me_.idx();
    	MEMORIA_INFO(me_.model(), str, "[node_id key_idx data_id page_offset offset bof eof empty]", (me_.page() != NULL ? me_.page()->id() : ID(0)), me_.key_idx(), (me_.data() != NULL ? me_.data()->id() : ID(0)), me_.idx(), offset, me_.IsStart(), me_.IsEnd(), me_.IsEmpty());
    }

    BigInt GetBlobId() {return 0;}

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::models::array::IteratorContainerAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::Read(ArrayData& data, BigInt start, BigInt len)
{
	BigInt sum = 0;

	while (len > 0)
	{
		Int to_read = me_.data()->data().size() - me_.idx();

		if (to_read > len) to_read = len;

		CopyBuffer(me_.data()->data().value_addr(me_.idx()), data.data() + start, to_read);

		len 	-= to_read;
		me_.Skip(to_read);

		sum 	+= to_read;
		start 	+= to_read;

		if (me_.IsEof())
		{
			break;
		}
	}

	return sum;
}

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

	//FIXME: handle START properly
	if (me_.IsNotEmpty())
	{
		Int data_size = me_.data()->data().size();
		Int idx = me_.idx();

		if (distance + idx <= data_size)
		{
			MEMORIA_TRACE(me_.model(), "Within the page");
			// A trivial case when the offset is within current data page

			// we need to check for EOF if a data page
			// is the last one in the index node
			if (distance + idx == data_size)
			{
				if (me_.key_idx() == me_.model().GetChildrenCount(me_.page()) - 1)
				{
					NodeBase* next = me_.GetNextNode();
					if (next == NULL)
					{
						me_.idx() = data_size;
					}
					else {
						me_.page() 		= next;
						me_.key_idx()	= 0;
						me_.idx() 		= 0;

						me_.data()		= me_.model().GetDataPage(me_.page(), me_.key_idx());
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

			if (end)
			{
				me_.idx() 	= me_.data()->data().size();
				return walker.sum() - idx;
			}
			else {
				me_.idx() 	= walker.remainder();
			}
		}


		//FIXME: return true distance
		return distance;
	}
	else {
		return 0;
	}
}


M_PARAMS
BigInt M_TYPE::SkipBw(BigInt distance)
{
	MEMORIA_TRACE(me_.model(), "Begin", distance);

	//FIXME: handle EOF properly
	if (me_.IsNotEmpty())
	{
		Int idx = me_.idx();

		if (distance <= idx)
		{
			// A trivial case when the offset is within current data page
			// we need to check for START if a data page
			// is the fist in the index node
			me_.idx() 	-= distance;
		}
		else
		{
			Int data_size = me_.data()->data().size();
			Int to_add = data_size - idx;
			NodeTreeWalker<Container, Key, false> walker(distance + to_add, me_.model());

			//FIXME: does 'end' means the same as for StepFw()?
			bool end 	= me_.WalkBw(me_.page(), me_.key_idx(), walker);
			me_.data() 	= me_.model().GetDataPage(me_.page(), me_.key_idx());

			if (end)
			{
				me_.idx() 	= 0;
				return walker.sum() - to_add;
			}
			else {
				me_.idx()	= me_.data()->data().size() - walker.remainder();
			}
		}

		return distance;
	}
	else {
		return 0;
	}
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
