
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

    BigInt selectFw(BigInt rank, Symbol symbol);
    BigInt selectBw(BigInt rank, Symbol symbol);

    BigInt countFw(Symbol symbol);
    BigInt countBw(Symbol symbol);

    BigInt rank(BigInt size, Symbol symbol);

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::selectFw(BigInt rank, Symbol symbol)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		Int data_pos  = iter.dataPos();
		auto result   = iter.data()->sequence().selectFW(data_pos, symbol, rank);

		if (result.is_found())
		{
			iter.dataPos() = result.idx();
			return rank;
		}
		else {
			BigInt prefix = iter.prefix(0) + iter.data()->size();

			BigInt rank0 = rank - result.rank();

			Key prefixes[2] = {0, 0};
			Int key_nums[2] = {(Int)symbol + 1, 0};

			SequenceFWWalker<Types, SumCompareLT> walker(iter.model(), rank0, 2, key_nums, prefixes);

			Int idx = iter.model().findFw(iter.path(), iter.key_idx() + 1, walker);

			if (idx < iter.page()->children_count())
			{
				walker.finish(idx, *me());
				iter.cache().setup(prefix + prefixes[1], 0);

				auto result1 = iter.data()->sequence().selectFW(0, symbol, rank0 - prefixes[0]);

				if (result1.is_found())
				{
					iter.dataPos() = result1.idx();
					return rank;
				}
				else {
					iter.dataPos() = iter.data()->size();
					return result.rank() + prefixes[0] + result1.rank();
				}
			}
			else {
				walker.finish(iter.page()->children_count() - 1, *me());
				iter.dataPos() = iter.data()->size();

				iter.cache().setup(prefix + prefixes[1] - iter.dataPos(), 0);

				return result.rank() + prefixes[0];
			}
		}
	}
	else {
		return 0;
	}
}


M_PARAMS
BigInt M_TYPE::selectBw(BigInt rank, Symbol symbol)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		Int data_pos  = iter.dataPos();

		auto result = iter.data()->sequence().selectBW(data_pos + 1, symbol, rank);

		if (result.is_found())
		{
			iter.dataPos() = result.idx();
			return rank;
		}
		else {
			BigInt rank0  	= rank - result.rank();
			BigInt prefix 	= iter.prefix(0);

			Key prefixes[2] = {0, 0};
			Int key_nums[2] = {(Int)symbol + 1, 0};

			SequenceBWWalker<Types, SumCompareLT> walker(iter.model(), rank0, 2, key_nums, prefixes);

			Int idx = me()->model().findBw(iter.path(), iter.key_idx() - 1, walker);

			if (idx >= 0)
			{
				walker.finish(idx, iter);

				Int data_size = iter.data()->size();

				BigInt length_prefix = prefix - prefixes[1] - data_size;

				iter.cache().setup(length_prefix, 0);

				auto result1 = iter.data()->sequence().selectBW(data_size, symbol, rank0 - prefixes[0]);

				if (result1.is_found())
				{
					iter.dataPos() = result1.idx();
					return rank;
				}
				else {
					iter.dataPos() = -1;
					return result.rank() + prefixes[0] + result1.rank();
				}
			}
			else {
				walker.finish(0, *me());

				iter.cache().setup(0, 0);
				iter.dataPos() = -1;

				return result.rank() + prefixes[0];
			}
		}
	}
	else {
		return 0;
	}
}


M_PARAMS
BigInt M_TYPE::countFw(Symbol symbol)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		Int data_pos  = iter.dataPos();

		Int result = iter.data()->sequence().countFW(data_pos, symbol);

		if (result + data_pos < iter.data()->sequence().size())
		{
			iter.dataPos() += result;

			return result;
		}
		else {
			BigInt prefix = iter.prefix(0) + iter.data()->size();

			SequenceCountFWWalker<Types> walker(iter.model(), symbol + 1);

			Int idx = iter.model().findFw(iter.path(), iter.key_idx() + 1, walker);

			if (idx < iter.page()->children_count())
			{
				walker.finish(idx, iter);
				iter.cache().setup(prefix + walker.prefix(), 0);

				const DataPage* data = iter.data().page();

				Int result1 = data->sequence().countFW(0, symbol);

				if (result1 < data->size())
				{
					iter.dataPos() = result1;
				}
				else {
					iter.dataPos() = data->size();
				}

				return result + walker.prefix() + result1;
			}
			else {
				walker.finish(iter.page()->children_count() - 1, *me());
				iter.dataPos() = iter.data()->size();

				iter.cache().setup(prefix + walker.prefix() - iter.dataPos(), 0);

				return result + walker.prefix();
			}
		}
	}

	return 0;
}



M_PARAMS
BigInt M_TYPE::countBw(Symbol symbol)
{
	MyType& iter = *me();

	if (iter.isNotEmpty())
	{
		Int data_pos  = iter.dataPos() + 1;

		Int result = iter.data()->sequence().countBW(data_pos, symbol);

		if (result < data_pos)
		{
			iter.dataPos() -= result;

			return result;
		}
		else {
			BigInt prefix = iter.prefix(0);

			SequenceCountBWWalker<Types> walker(iter.model(), symbol + 1);

			Int idx = iter.model().findBw(iter.path(), iter.key_idx() - 1, walker);

			if (idx >= 0)
			{
				walker.finish(idx, iter);

				const DataPage* data = iter.data().page();

				Int data_size = data->size();


				BigInt prefix_len = prefix - walker.prefix() - data_size;
				iter.cache().setup(prefix_len, 0);

				Int result1 = data->sequence().countBW(data_size, symbol);

				if (result1 < data_size)
				{
					iter.dataPos() = data_size - result1 - 1;
				}
				else {
					iter.dataPos() = -1;
				}

				return result + walker.prefix() + result1;
			}
			else {
				walker.finish(0, *me());
				iter.cache().setup(0, 0);

				iter.dataPos() = -1;
				return result + walker.prefix();
			}
		}
	}

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
