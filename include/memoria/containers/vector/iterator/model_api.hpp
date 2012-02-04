
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_ITERATOR_MODEL_API_HPP
#define _MEMORIA_MODELS_ARRAY_ITERATOR_MODEL_API_HPP

#include <iostream>

#include <memoria/containers/vector/names.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>


namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::models::array::IteratorContainerAPIName)

    typedef typename Base::NodeBase                                             	NodeBase;
	typedef typename Base::NodeBaseG                                             	NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Base::Container::ApiKeyType                                    ApiKeyType;
    typedef typename Base::Container::ApiValueType                                  ApiValueType;
    typedef typename Container::Key                                                 Key;

    typedef typename Base::Container::Page                                          PageType;
    typedef typename Base::Container::ID                                            ID;

    typedef typename Container::Types::Allocator                                	Allocator;
    typedef typename Container::Types::DataPage                                		DataPage;
    typedef typename Container::Types::DataPageG                                	DataPageG;
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
    	MyType to = *me();

    	if (me()->model().debug()) {
    		int a = 0;
    		a++;
    	}

    	to.Skip(len);
    	me()->model().RemoveDataBlock(*me(), to);
    }

    void Remove(MyType& to)
    {
    	me()->model().RemoveDataBlock(*me(), to);
    }

    BigInt Skip(BigInt distance);
    BigInt SkipFw(BigInt distance);
    BigInt SkipBw(BigInt distance);

    void DumpState(const char* str)
    {
    	BigInt offset = me()->GetIndexValue(0) + me()->data_pos();
    	MEMORIA_INFO(me()->model(), str, "[node_id key_idx data_id page_offset offset bof eof empty]", (me()->page() != NULL ? me()->page()->id() : ID(0)), me()->key_idx(), (me()->data() != NULL ? me()->data()->id() : ID(0)), me()->data_pos(), offset, me()->IsStart(), me()->IsEnd(), me()->IsEmpty());
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
		Int to_read = me()->data()->data().size() - me()->data_pos();

		if (to_read > len) to_read = len;

		CopyBuffer(me()->data()->data().value_addr(me()->data_pos()), data.data() + start, to_read);

//		if (me()->model().debug())
//		{
//			me()->model().Dump(me()->data());
//		}

		len 	-= to_read;
		me()->Skip(to_read);

		sum 	+= to_read;
		start 	+= to_read;

		if (me()->IsEof())
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

	me()->model().InsertDataBlock(*me(), data, descriptor);
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
	//FIXME: handle START properly
	if (me()->IsNotEmpty())
	{
		Int data_size = me()->data()->data().size();
		Int data_pos = me()->data_pos();

		if (distance + data_pos <= data_size)
		{
			// A trivial case when the offset is within current data page

			// we need to check for EOF if a data page
			// is the last one in the index node
			if (distance + data_pos == data_size)
			{
				if (me()->key_idx() == me()->page()->children_count() - 1)
				{
					NodeBaseG next = me()->GetNextNode();
					if (next == NULL)
					{
						me()->data_pos() = data_size;
					}
					else {
						me()->page() 		= next;
						me()->key_idx()		= 0;
						me()->data_pos() 	= 0;

						me()->data()		= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
					}
				}
				else {
					me()->key_idx()++;
					me()->data_pos() 		= 0;
					me()->data() 			= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);
				}
			}
			else {
				me()->data_pos() += distance;
			}
		}
		else
		{
			NodeTreeWalker<Container, Key, true> walker(distance + data_pos, me()->model());

			bool end 	= me()->WalkFw(me()->page(), me()->key_idx(), walker);
			me()->data() 	= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);

			if (end)
			{
				me()->data_pos() 	= me()->data()->data().size();
				me()->Init();
				return walker.sum() - data_pos;
			}
			else {
				me()->data_pos() 	= walker.remainder();
			}
		}
		me()->Init();

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
	//FIXME: handle EOF properly
	if (me()->IsNotEmpty())
	{
		Int idx = me()->data_pos();

		if (distance <= idx)
		{
			// A trivial case when the offset is within current data page
			// we need to check for START if a data page
			// is the fist in the index node
			me()->data_pos() 	-= distance;
		}
		else
		{
			Int data_size = me()->data()->data().size();
			Int to_add = data_size - idx;
			NodeTreeWalker<Container, Key, false> walker(distance + to_add, me()->model());

			//FIXME: does 'end' means the same as for StepFw()?
			bool end 		= me()->WalkBw(me()->page(), me()->key_idx(), walker);
			me()->data() 	= me()->model().GetDataPage(me()->page(), me()->key_idx(), Allocator::READ);

			if (end)
			{
				me()->data_pos() 	= 0;
				me()->Init();
				return walker.sum() - to_add;
			}
			else {
				me()->data_pos()	= me()->data()->data().size() - walker.remainder();
			}
		}

		me()->Init();
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
