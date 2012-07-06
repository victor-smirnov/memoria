
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


    Int getElementSize() const
    {
    	return me()->model().getElementSize();
    }


    BigInt read(IData& data, BigInt start, BigInt length)
    {
    	return me()->model().read(*me(), data, start, length);
    }

    BigInt read(IData& data)
    {
    	return read(data, 0, data.getSize());
    }

    
    void insert(const IData& data, BigInt start, BigInt length);
    void insert(const IData& data);

    void insert(const ArrayData& data) {
    	insert((IData&)data);
    }

    template <typename T>
    void insert(const T& value)
    {
    	me()->insert(ArrayData(value));
    }

    void update(const IData& data, BigInt start, BigInt length);
    void update(const IData& data);

    void remove(BigInt length)
    {
    	me()->model().removeDataBlock(*me(), length);
    }

    void remove(MyType& to)
    {
    	me()->model().removeDataBlock(*me(), to);
    }

    BigInt skip(BigInt distance);
    BigInt skipFw(BigInt distance);
    BigInt skipBw(BigInt distance);

    void dumpKeys(ostream& out)
    {
    	Base::dumpKeys(out);

    	out<<"Pos:     "<<me()->pos()<<endl;
    	out<<"DataPos: "<<me()->data_pos()<<endl;
    }

    void dumpPages(ostream& out)
    {
    	Base::dumpPages(out);
    	me()->model().Dump(me()->data(), out);
    }

    void dumpPath(ostream& out)
    {
    	Base::dumpPath(out);

    	if (me()->data().isSet())
    	{
    		out<<"Data:    "<<IDValue(me()->data()->id())<<" at "<<me()->path().data().parent_idx()<<endl;
    	}
    	else {
    		out<<"No Data page is set"<<endl;
    	}
    }


    bool operator++()
    {
    	Int size = me()->getElementSize();
    	return me()->skip(size) = size;
    }

    bool operator++(int)
    {
    	return me()->skip(1) = 1;
    }

    bool operator+=(Int count)
    {
    	return me()->skip(count) = count;
    }

    bool operator--()
    {
    	return me()->skip(1);
    }

    bool operator--(int)
    {
    	return me()->skip(-1) = 1;
    }

    bool operator-=(Int count)
	{
    	return me()->skip(-count) = count;
	}


//    void assign(const IData& data)
//    {
//    	update(data);
//    }

    template <typename T>
    operator T()
    {
    	T value;

    	ArrayData data(value);

    	me()->read(data);

    	return value;
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::models::array::IteratorContainerAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::insert(const IData& data, BigInt start, BigInt length)
{
	me()->model().insertData(*me(), data, start, length);
}

M_PARAMS
void M_TYPE::insert(const IData& data)
{
	me()->model().insertData(*me(), data);
}



M_PARAMS
void M_TYPE::update(const IData& data, BigInt start, BigInt len)
{
	me()->model().updateData(*me(), data, start, len);
}

M_PARAMS
void M_TYPE::update(const IData& data)
{
	me()->model().updateData(*me(), data, 0, data.getSize());
}



M_PARAMS
BigInt M_TYPE::skip(BigInt distance)
{
	if (distance > 0)
	{
		return skipFw(distance);
	}
	else {
		return skipBw(-distance);
	}
}

M_PARAMS
BigInt M_TYPE::skipFw(BigInt count)
{
	Int element_size = me()->getElementSize();

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

			bool end = me()->model().walkFw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= me()->data()->size();

				me()->cache().setup(pos + (walker.sum() - data_pos) - me()->data_pos(), 0);

				return (walker.sum() - data_pos) / element_size;
			}
			else {

				me()->data_pos() 	= walker.remainder();

				me()->cache().setup(pos + distance - me()->data_pos(), 0);
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
BigInt M_TYPE::skipBw(BigInt count)
{
	Int element_size = me()->getElementSize();

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
			bool end 		= me()->model().walkBw(me()->path(), me()->key_idx(), walker);

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			if (end)
			{
				me()->data_pos() 	= 0;

				me()->cache().setup(0, 0);

				return (walker.sum() - to_add) / element_size;
			}
			else {
				me()->data_pos()		= me()->data()->size() - walker.remainder();

				me()->cache().setup((pos - distance) - me()->data_pos(), 0);
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
