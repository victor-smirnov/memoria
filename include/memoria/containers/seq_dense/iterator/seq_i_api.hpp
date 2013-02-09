
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_ITERATOR_API_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_ITERATOR_API_HPP

#include <memoria/containers/seq_dense/names.hpp>
#include <memoria/prototypes/sequence/tools.hpp>
#include <memoria/core/container/iterator.hpp>

#include <memoria/core/tools/walkers.hpp>

#include <memoria/containers/seq_dense/walkers.hpp>

#include <memoria/core/tools/symbol_sequence.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterAPIName)

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

    typedef typename Base::TreePath                                             TreePath;

    typedef typename Types::Value                              					Symbol;


    static const Int Indexes = Container::Indexes;

    typedef SymbolSequence<
    		Types::BitsPerSymbol,
    		ElementType,
    		typename DataPage::IndexType
    > 																			SymbolSequenceType;

    
    MEMORIA_PUBLIC SymbolSequenceType subSequence(BigInt length) const
    {
        MyType tmp = *me();
        return tmp.readSequence(length);
    }

    MEMORIA_PUBLIC SymbolSequenceType readSequence(BigInt length)
    {
        BigInt max_size = me()->model().size() - me()->pos();

        if (length > max_size)
        {
            length = max_size;
        }

        SymbolSequenceType seq(length);
        seq.resize(length);

        auto tgt = seq.target();

        me()->read(tgt);

        return seq;
    }

    BigInt selectNext(BigInt rank, Symbol symbol);
    BigInt selectPrev(BigInt rank, Symbol symbol);

    BigInt countNext();
    BigInt countPrev();

    BigInt rank(BigInt size, Symbol symbol);

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::selectNext(BigInt rank, Symbol symbol)
{
	BigInt pos = me()->pos();

	Int data_pos  = me()->dataPos();

	auto result = me()->data()->sequence().selectFW(data_pos, symbol, rank);

	if (result.is_found())
	{
		me()->dataPos() = result.idx();
		return rank;
	}
	else {
		BigInt rank0 = rank + me()->data()->sequence().maxIndex(symbol) - result.rank();

		SelectWalker<BigInt> walker(rank0, 0, symbol + 1);

		bool end = me()->model().walkFw(me()->path(), me()->key_idx(), walker);

		me()->model().finishPathStep(me()->path(), me()->key_idx());

		if (!end)
		{
			result = me()->data()->sequence().selectFW(0, symbol, walker.remainder());

			if (result.is_found())
			{
				me()->dataPos() = result.idx();

				//me()->cache().setup(pos - data_pos + walker.distance() - me()->dataPos(), 0);

				return rank;
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"selectNext("<<rank<<", "<<symbol<<") failed");
			}
		}
		else
		{
			me()->dataPos() = me()->data()->size();

			//me()->cache().setup(pos + (walker.distance() - data_pos) - me()->dataPos(), 0);

			return walker.rank() - result.rank();
		}
	}
}



M_PARAMS
BigInt M_TYPE::selectPrev(BigInt rank, Symbol symbol)
{
	BigInt pos = me()->pos();

	Int data_pos  = me()->dataPos();

	auto result = me()->data()->sequence().selectBW(data_pos + 1, symbol, rank);

	if (result.is_found())
	{
		me()->dataPos() = result.idx();

		return rank;
	}
	else {
		BigInt rank0 = rank + me()->data()->sequence().maxIndex(symbol) - result.rank();

		SelectWalker<BigInt, false> walker(rank0, -me()->data()->size(), symbol + 1);

		bool end = me()->model().walkBw(me()->path(), me()->key_idx(), walker);

		me()->model().finishPathStep(me()->path(), me()->key_idx());

		if (!end)
		{
			Int data_size = me()->data()->size();

			result = me()->data()->sequence().selectBW(data_size, symbol, walker.remainder());

			if (result.is_found())
			{
				me()->dataPos() = result.idx();

				me()->cache().setup(pos - data_pos - walker.distance() - me()->dataPos(), 0);

				return rank;
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"selectPrev("<<rank<<", "<<symbol<<") failed");
			}
		}
		else
		{
			me()->dataPos() = 0;

			me()->cache().setup(0, 0);

			return walker.rank() - result.rank();
		}
	}
}

M_PARAMS
BigInt M_TYPE::countNext()
{
	Int data_pos  = me()->dataPos();

	auto result = me()->data()->sequence().countFW(data_pos, symbol, rank);

	if (result.is_found())
	{
		me()->dataPos() = result.idx();
		return rank;
	}
	else {
		BigInt rank0 = rank + me()->data()->sequence().maxIndex(symbol) - result.rank();

		SelectWalker<BigInt> walker(rank0, 0, symbol + 1);

		bool end = me()->model().walkFw(me()->path(), me()->key_idx(), walker);

		me()->model().finishPathStep(me()->path(), me()->key_idx());

		if (!end)
		{
			result = me()->data()->sequence().countFW(0, symbol, walker.remainder());

			if (result.is_found())
			{
				me()->dataPos() = result.idx();

				//me()->cache().setup(pos - data_pos + walker.distance() - me()->dataPos(), 0);

				return rank;
			}
			else {
				throw Exception(MA_SRC, SBuf()<<"selectNext("<<rank<<", "<<symbol<<") failed");
			}
		}
		else
		{
			me()->dataPos() = me()->data()->size();

			//me()->cache().setup(pos + (walker.distance() - data_pos) - me()->dataPos(), 0);

			return walker.rank() - result.rank();
		}
	}

	return 0;
}

M_PARAMS
BigInt M_TYPE::countPrev()
{


	return 0;
}

M_PARAMS
BigInt M_TYPE::rank(BigInt size, Symbol symbol)
{
	Int data_size = me()->data_size();
	Int data_pos  = me()->dataPos();

	if (data_pos + size < data_size)
	{
		return me()->data()->sequence().rank(data_pos, data_pos + size, symbol);
	}
	else {
		MyType tmp = *me();

		tmp.skipFw(size);

		return tmp.prefix(symbol + 1) - me()->prefix(symbol + 1);
	}

	return 0;
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
