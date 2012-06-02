
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
#include <memoria/core/tools/sum_walker.hpp>

#include <memoria/core/tools/array_data.hpp>

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


    Int GetElementSize() const
    {
    	return me()->model().GetElementSize();
    }


    BigInt Read(IData& data, BigInt start, BigInt length)
    {
    	return me()->model().Read(*me(), data, start, length);
    }

    BigInt Read(IData& data)
    {
    	return Read(data, 0, data.GetSize());
    }

    
    void Insert(const IData& data, BigInt start, BigInt length);
    void Insert(const IData& data);

    void Insert(const ArrayData& data) {
    	Insert((IData&)data);
    }

    template <typename T>
    void Insert(const T& value)
    {
    	me()->Insert(ArrayData(value));
    }

    void Update(const IData& data, BigInt start, BigInt length);
    void Update(const IData& data);

    void Remove(BigInt length)
    {
    	me()->model().RemoveDataBlock(*me(), length);
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


    bool operator++()
    {
    	Int size = me()->GetElementSize();
    	return me()->Skip(size) = size;
    }

    bool operator++(int)
    {
    	return me()->Skip(1) = 1;
    }

    bool operator+=(Int count)
    {
    	return me()->Skip(count) = count;
    }

    bool operator--()
    {
    	return me()->Skip(1);
    }

    bool operator--(int)
    {
    	return me()->Skip(-1) = 1;
    }

    bool operator-=(Int count)
	{
    	return me()->Skip(-count) = count;
	}


    void Assing(const IData& data)
    {
    	Update(data);
    }

    template <typename T>
    operator T()
    {
    	T value;

    	ArrayData data(value);

    	me()->Read(data);

    	return value;
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::models::array::IteratorContainerAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::Insert(const IData& data, BigInt start, BigInt length)
{
	me()->model().InsertData(*me(), data, start, length);
}

M_PARAMS
void M_TYPE::Insert(const IData& data)
{
	me()->model().InsertData(*me(), data);
}



M_PARAMS
void M_TYPE::Update(const IData& data, BigInt start, BigInt len)
{
	me()->model().UpdateData(*me(), data, start, len);
}

M_PARAMS
void M_TYPE::Update(const IData& data)
{
	me()->model().UpdateData(*me(), data, 0, data.GetSize());
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
BigInt M_TYPE::SkipFw(BigInt count)
{
	Int element_size = me()->GetElementSize();

	BigInt distance = count * element_size;

	//FIXME: handle START properly
	if (me()->IsNotEmpty())
	{
		Int 	data_size 	= me()->data()->size();
		Int 	data_pos 	= me()->data_pos();
		BigInt 	pos 		= me()->pos();

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
			SumTreeWalker<Container, Key, true> walker(distance + data_pos, me()->model());

			bool end = me()->model().WalkFw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= me()->data()->size();

				me()->cache().Setup(pos + (walker.sum() - data_pos) - me()->data_pos(), 0);

				return (walker.sum() - data_pos) / element_size;
			}
			else {

				me()->data_pos() 	= walker.remainder();

				me()->cache().Setup(pos + distance - me()->data_pos(), 0);
			}
		}

		//FIXME: return true distance
		return count;
	}
	else {
		return 0;
	}
}


M_PARAMS
BigInt M_TYPE::SkipBw(BigInt count)
{
	Int element_size = me()->GetElementSize();

	BigInt distance = count * element_size;


	//FIXME: handle EOF properly
	if (me()->IsNotEmpty())
	{
		BigInt pos = me()->pos();

		Int idx = me()->data_pos();

		if (distance <= idx)
		{
			// A trivial case when the offset is within current data page
			// we need to check for START if a data page
			// is the first in the index node
			me()->data_pos() 	-= distance;
		}
		else
		{
			Int data_size 	= me()->data()->size();
			Int to_add 		= data_size - idx;
			SumTreeWalker<Container, Key, false> walker(distance + to_add, me()->model());

			//FIXME: does 'end' means the same as for StepFw()?
			bool end 		= me()->model().WalkBw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= 0;

				me()->cache().Setup(0, 0);

				return (walker.sum() - to_add) / element_size;
			}
			else {
				me()->data_pos()		= me()->data()->size() - walker.remainder();

				me()->cache().Setup((pos - distance) - me()->data_pos(), 0);
			}
		}

		return count;
	}
	else {
		return 0;
	}
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
