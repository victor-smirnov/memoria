
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_I_API_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_I_API_HPP

#include <memoria/prototypes/sequence/names.hpp>
#include <memoria/prototypes/sequence/tools.hpp>
#include <memoria/prototypes/sequence/sequence_walkers.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>
#include <memoria/core/tools/sum_walker.hpp>

#include <memoria/core/tools/idata.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;
using namespace memoria::sequence;

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
   		me()->data()->sequence().value(me()->dataPos()) = value;
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

   	template <typename Functor>
   	void findFw(Functor&& fn);

   	template <typename Functor>
   	void findBw(Functor&& fn);


   	BigInt skipFw(BigInt distance)
   	{
   		EmptyExtenderState state;

   		typename Types::template SkipForwardWalker<
   			Types,
   			EmptyExtender,
   			EmptyExtender,
   			EmptyExtenderState
   		>
   		walker(distance, 0, state, state);

   		findFw(walker);

   		return walker.sum();
   	}

   	BigInt skipBw(BigInt distance)
   	{
   		EmptyExtenderState state;

   		typename Types::template SkipBackwardWalker<
   			Types,
   			EmptyExtender,
   			EmptyExtender,
   			EmptyExtenderState
   		> walker(distance, 0, state, state);

   		findBw(walker);

   		return walker.sum();
   	}



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

//M_PARAMS
//BigInt M_TYPE::skipFw(BigInt distance)
//{
//	MyType& iter = *me();
//
//	if (iter.isNotEmpty())
//	{
//		BigInt data_pos 	= iter.dataPos();
//		BigInt first_size   = iter.data()->size();
//
//		if (data_pos + distance < first_size)
//		{
//			iter.dataPos() += distance;
//
//			return distance;
//		}
//		else {
//			BigInt prefix = iter.prefix(0) + first_size;
//
//			BigInt start  = first_size - data_pos;
//			BigInt target = distance - start;
//
//			SequenceSkipFWWalker<Types> walker(iter.model(), 0, target);
//
//			Int idx = iter.model().findFw(iter.path(), iter.key_idx() + 1, walker);
//
//			if (idx < iter.page()->children_count())
//			{
//				walker.finish(idx, iter);
//				iter.cache().setup(prefix + walker.prefix(), 0);
//
//				BigInt remainder = target - walker.prefix();
//
//				const DataPage* data = iter.data().page();
//
//				if (remainder < data->size())
//				{
//					iter.dataPos() = remainder;
//				}
//				else {
//					iter.dataPos() = data->size();
//				}
//
//				return start + walker.prefix() + remainder;
//			}
//			else {
//				walker.finish(iter.page()->children_count() - 1, iter);
//				iter.dataPos() = iter.data()->size();
//				iter.cache().setup(prefix + walker.prefix() - iter.dataPos(), 0);
//
//				return start;
//			}
//		}
//	}
//
//	return 0;
//}



//MEMORIA_PUBLIC M_PARAMS
//BigInt M_TYPE::skipBw(BigInt distance)
//{
//	MyType& iter = *me();
//
//	if (iter.isNotEmpty())
//	{
//		Int data_pos  = iter.dataPos();
//
//		if (data_pos - distance >= 0)
//		{
//			iter.dataPos() -= distance;
//			return distance;
//		}
//		else {
//			BigInt prefix = iter.prefix(0);
//			BigInt target = distance - data_pos;
//
//			SequenceSkipBWWalker<Types> walker(iter.model(), 0, target);
//
//			Int idx = iter.model().findBw(iter.path(), iter.key_idx() - 1, walker);
//
//			if (idx >= 0)
//			{
//				walker.finish(idx, iter);
//
//				const DataPage* data = iter.data().page();
//
//				Int data_size = data->size();
//
//				BigInt prefix_len = prefix - walker.prefix() - data_size;
//				iter.cache().setup(prefix_len, 0);
//
//				BigInt remainder = target - walker.prefix();
//
//				if (remainder <= data_size)
//				{
//					iter.dataPos() = data_size - remainder;
//				}
//				else {
//					iter.dataPos() = -1;
//				}
//
//				return data_pos + walker.prefix() + remainder;
//			}
//			else {
//				walker.finish(0, iter);
//				iter.dataPos() = -1;
//				iter.cache().setup(0, 0);
//
//				return data_pos;
//			}
//		}
//	}
//
//	return 0;
//}

M_PARAMS
template <typename Functor>
void M_TYPE::findFw(Functor&& walker)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		DataPageG data = iter.data();

		walker.setup(iter);

		if (!walker.dispatchFirstData(iter))
		{
			Int idx = iter.model().findFw(iter.path(), iter.key_idx() + 1, walker);

			if (idx < iter.page()->children_count())
			{
				walker.finish(idx, iter);
				walker.dispatchLastData(iter);
			}
			else {
				walker.finishEof(iter);
			}
		}
	}
}


M_PARAMS
template <typename Functor>
void M_TYPE::findBw(Functor&& walker)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		DataPageG data = iter.data();

		walker.setup(iter);

		if (!walker.dispatchFirstData(iter))
		{
			Int idx = iter.model().findBw(iter.path(), iter.key_idx() - 1, walker);

			if (idx >= 0)
			{
				walker.finish(idx, iter);
				walker.dispatchLastData(iter);
			}
			else {
				walker.finishBof(iter);
			}
		}
	}
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
