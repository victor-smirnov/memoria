
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

    typedef typename Base::TreePath                                             	TreePath;

    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;


    BigInt Read(ArrayData& data, BigInt start, BigInt len);
    BigInt Read(ArrayData& data)
    {
    	return Read(data, 0, data.size());
    }

    
    void Insert(const ArrayData& data, BigInt start, BigInt len);
    void Insert(const ArrayData& data);


    void Update(const ArrayData& data, BigInt start, BigInt len);
    void Update(const ArrayData& data);

    void Remove(BigInt len)
    {
    	MyType to = *me();
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

    void DumpKeys(ostream& out)
    {
    	Base::DumpKeys(out);

    	out<<"Pos:     "<<me()->pos()<<endl;
    	out<<"DataPos: "<<me()->data_pos()<<endl;
    }

    void DumpPages(ostream& out)
    {
    	Base::DumpPages(out);
    	me()->model().Dump(me()->data(), out);
    }

    void DumpPath(ostream& out)
    {
    	Base::DumpPath(out);

    	if (me()->data().is_set())
    	{
    		out<<"Data:    "<<IDValue(me()->data()->id())<<" at "<<me()->path().data().parent_idx()<<endl;
    	}
    	else {
    		out<<"No Data page is set"<<endl;
    	}
    }

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
void M_TYPE::Insert(const ArrayData& data, BigInt start, BigInt len)
{
	const ArrayData data1(len, data.data() + start);
	me()->model().InsertData(*me(), data1);
}

M_PARAMS
void M_TYPE::Insert(const ArrayData& data)
{
	me()->model().InsertData(*me(), data);
}



M_PARAMS
void M_TYPE::Update(const ArrayData& data, BigInt start, BigInt len)
{
	me()->model().UpdateData(*me(), data, start, len);
}

M_PARAMS
void M_TYPE::Update(const ArrayData& data)
{
	me()->model().UpdateData(*me(), data, 0, data.size());
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
		Int data_size 	= me()->data()->size();
		Int data_pos 	= me()->data_pos();

		if (distance + data_pos <= data_size)
		{
			// A trivial case when the offset is within current data page

			// we need to check for EOF if a data page
			// is the last one in the index node
			if (distance + data_pos == data_size)
			{
				if (me()->NextKey())
				{
					// do nothing
				}
				else {
					// Eof
					me()->PrevKey();
					me()->data_pos() = me()->data()->size();
				}
			}
			else {
				me()->data_pos() += distance;
			}
		}
		else
		{
			NodeTreeWalker<Container, Key, true> walker(distance + data_pos, me()->model());

			bool end 		= me()->model().WalkFw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= me()->data()->size();
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
			Int data_size 	= me()->data()->size();
			Int to_add 		= data_size - idx;
			NodeTreeWalker<Container, Key, false> walker(distance + to_add, me()->model());

			//FIXME: does 'end' means the same as for StepFw()?
			bool end 		= me()->model().WalkBw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= 0;
				me()->Init();
				return walker.sum() - to_add;
			}
			else {
				me()->data_pos()	= me()->data()->size() - walker.remainder();
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
