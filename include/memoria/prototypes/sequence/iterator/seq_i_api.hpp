
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_I_API_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_I_API_HPP

#include <memoria/prototypes/sequence/names.hpp>
#include <memoria/prototypes/sequence/tools.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>
#include <memoria/core/tools/sum_walker.hpp>

#include <memoria/core/tools/idata.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::sequence::IterAPIName)

	typedef typename Base::NodeBase                                             NodeBase;
   	typedef typename Base::NodeBaseG                                            NodeBaseG;
   	typedef typename Base::Container                                            Container;

   	typedef typename Container::Key                                             Key;

   	typedef typename Container::Page                                      		PageType;
   	typedef typename Container::ID                                        		ID;

   	typedef typename Types::Allocator                                			Allocator;
   	typedef typename Types::DataPage                                 			DataPage;
   	typedef typename Types::DataPageG                                			DataPageG;
   	typedef typename Types::Pages::NodeDispatcher                    			NodeDispatcher;
   	typedef typename Types::ElementType                              			ElementType;

   	typedef typename Types::IDataSourceType                                 	IDataSourceType;
   	typedef typename Types::IDataTargetType                                 	IDataTargetType;

   	typedef typename Base::TreePath                                             TreePath;

   	static const Int Indexes = Container::Indexes;

   	BigInt          local_pos_;

   	IterPart(): Base(), local_pos_(0) {}

   	IterPart(ThisPartType&& other): Base(std::move(other)), local_pos_(other.local_pos_) {}

   	IterPart(const ThisPartType& other): Base(other), local_pos_(other.local_pos_) {}


   	void assign(const ThisPartType& other)
   	{
   		Base::assign(other);

   		local_pos_      = other.local_pos_;
   	}

   	void assign(ThisPartType&& other)
   	{
   		Base::assign(std::move(other));

   		local_pos_      = other.local_pos_;
   	}

   	bool isEqual(const ThisPartType& other) const
   	{
   		return local_pos_  == other.local_pos_ && Base::isEqual(other);
   	}

   	bool isNotEqual(const ThisPartType& other) const
   	{
   		return local_pos_  != other.local_pos_ || Base::isNotEqual(other);
   	}

   	bool isEof() const
   	{
   		return me()->data().isSet() ? me()->dataPos() >= me()->data()->size() : true;
   	}

   	bool isBof() const
   	{
   		return local_pos_ == 0 && me()->isBegin();
   	}

   	DataPageG& data()
   	{
   		return me()->path().data();
   	}

   	const DataPageG& data() const
   	{
   		return me()->path().data();
   	}

   	BigInt &dataPos() {
   		return local_pos_;
   	}

   	const BigInt dataPos() const {
   		return local_pos_;
   	}

   	BigInt pos() const
   	{
   		return me()->prefix() + me()->dataPos();
   	}


   	bool nextKey()
   	{
   		me()->dataPos()     = 0;

   		return Base::nextKey();
   	}

   	bool prevKey()
   	{
   		me()->dataPos()     = 0;

   		return Base::prevKey();
   	}


   	MEMORIA_PUBLIC BigInt read(IDataTargetType& data)
   	{
   		return me()->model().read(*me(), data);
   	}

   	void insert(IDataSourceType& data);
   	void update(IDataSourceType& data);

   	void update(const vector<ElementType>& other)
   	{
   		MemBuffer<const ElementType> buffer(&other[0], other.size());
   		me()->update(buffer);
   	}

   	void insert(const vector<ElementType>& other)
   	{
   		MemBuffer<const ElementType> buffer(&other[0], other.size());
   		me()->insert(buffer);
   	}

   	void assignElement(const ElementType& value)
   	{
   		me()->data().update();
   		me()->data()->data().value(me()->dataPos()) = value;
   	}

   	MEMORIA_PUBLIC void remove(BigInt length)
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

    MEMORIA_PUBLIC void dumpKeys(ostream& out)
    {
    	Base::dumpKeys(out);

    	out<<"Pos:     "<<me()->pos()<<endl;
    	out<<"DataPos: "<<me()->dataPos()<<endl;
    }

    MEMORIA_PUBLIC void dumpPages(ostream& out)
    {
    	Base::dumpPages(out);
    	me()->model().dump(me()->data(), out);
    }

    MEMORIA_PUBLIC void dumpPath(ostream& out)
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


    MEMORIA_PUBLIC bool operator++()
    {
    	return me()->skip(1) == 1;
    }

    MEMORIA_PUBLIC bool operator++(int)
    {
    	return me()->skip(1) == 1;
    }

    MEMORIA_PUBLIC bool operator+=(Int count)
    {
    	return me()->skip(count) == count;
    }

    MEMORIA_PUBLIC bool operator--()
    {
    	return me()->skip(1) == 1;
    }

    MEMORIA_PUBLIC bool operator--(int)
    {
    	return me()->skip(-1) == 1;
    }

    MEMORIA_PUBLIC bool operator-=(Int count)
    {
    	return me()->skip(-count) == count;
    }

    MEMORIA_PUBLIC operator ElementType() const
    {
    	return element();
    }




    MEMORIA_PUBLIC ElementType element() const
    {
    	return me()->data()->sequence().value(me()->dataPos());
    }


    MEMORIA_PUBLIC IDataAdapter<MyType> asData(BigInt length = -1) const
    {
    	return IDataAdapter<MyType>(*me(), length);
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::sequence::IterAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

MEMORIA_PUBLIC M_PARAMS
void M_TYPE::insert(IDataSourceType& data)
{
    me()->model().insertData(*me(), data);
}


MEMORIA_PUBLIC M_PARAMS
void M_TYPE::update(IDataSourceType& data)
{
    me()->model().updateData(*me(), data);
}



MEMORIA_PUBLIC M_PARAMS
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

MEMORIA_PUBLIC M_PARAMS
BigInt M_TYPE::skipFw(BigInt distance)
{
    //FIXME: handle START properly
    if (me()->isNotEmpty())
    {
        Int     data_size   = me()->data()->size();
        Int     data_pos    = me()->dataPos();
        BigInt  pos         = me()->pos();

        if (distance + data_pos <= data_size)
        {
            // A trivial case when the offset is within current data page

            // we need to check for EOF if a data page
            // is the last one in the index node
            if (distance + data_pos == data_size)
            {
                if (me()->nextKey())
                {
                    // do nothing
                }
                else {
                    // Eof
                    me()->prevKey();
                    me()->dataPos() = me()->data()->size();
                }
            }
            else {
                me()->dataPos() += distance;
            }
        }
        else
        {
            SumTreeWalker<Container, Key, true> walker(distance + data_pos, me()->model());

            bool end = me()->model().walkFw(me()->path(), me()->key_idx(), walker);

            me()->model().finishPathStep(me()->path(), me()->key_idx());

            if (end)
            {
                me()->dataPos()     = me()->data()->size();

                me()->cache().setup(pos + (walker.sum() - data_pos) - me()->dataPos(), 0);

                return walker.sum() - data_pos;
            }
            else {

                me()->dataPos()     = walker.remainder();

                me()->cache().setup(pos + distance - me()->dataPos(), 0);
            }
        }

        //FIXME: return true distance
        return distance;
    }
    else {
        return 0;
    }
}


MEMORIA_PUBLIC M_PARAMS
BigInt M_TYPE::skipBw(BigInt distance)
{
    //FIXME: handle EOF properly
    if (me()->isNotEmpty())
    {
        BigInt pos = me()->pos();

        Int idx = me()->dataPos();

        if (distance <= idx)
        {
            // A trivial case when the offset is within current data page
            // we need to check for START if a data page
            // is the first in the index node
            me()->dataPos()     -= distance;
        }
        else
        {
            Int data_size   = me()->data()->size();
            Int to_add      = data_size - idx;
            SumTreeWalker<Container, Key, false> walker(distance + to_add, me()->model());

            //FIXME: does 'end' means the same as for StepFw()?
            bool end        = me()->model().walkBw(me()->path(), me()->key_idx(), walker);

            me()->model().finishPathStep(me()->path(), me()->key_idx());

            if (end)
            {
                me()->dataPos()     = 0;

                me()->cache().setup(0, 0);

                return walker.sum() - to_add;
            }
            else {
                me()->dataPos()     = me()->data()->size() - walker.remainder();

                me()->cache().setup((pos - distance) - me()->dataPos(), 0);
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
