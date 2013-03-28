
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

#include <memoria/containers/seq_dense/seqdense_walkers.hpp>

#include <memoria/core/tools/symbol_sequence.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;
using namespace memoria::sequence;


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

    BigInt rank(BigInt size, Symbol symbol) const;
    BigInt rank(Symbol symbol) const;

    bool test(Symbol value) const
    {
    	return me()->data()->sequence().test(me()->dataPos(), value);
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::selectFw(BigInt rank, Symbol symbol)
{
	MyType& iter = *me();

	BigInt pos = iter.pos();

	BigInt size = 0;

	auto size_fn = [&size](BigInt value, Int) {
		size += value;
	};

	Int node_indexes[1] = {0};
	balanced_tree::FunctorExtenderState<> node_state(1, node_indexes, size_fn);

	Int data_indexes[0] = {};
	balanced_tree::FunctorExtenderState<> data_state(0, data_indexes, size_fn);

	sequence::SequenceForwardWalker<
		Types,
		balanced_tree::NodeLTForwardWalker,
		SelectForwardWalker,
		balanced_tree::NodeSumExtender,
		SelectExtender,
		balanced_tree::FunctorExtenderState<>
	>
	walker(rank, symbol + 1, symbol, node_state, data_state);

	iter.findFw(walker);

	iter.cache().setup(pos + size - iter.dataPos() - 1 + walker.data_length(), 0);

	return walker.sum();
}


M_PARAMS
BigInt M_TYPE::selectBw(BigInt rank, Symbol symbol)
{
	MyType& iter = *me();

	BigInt pos = iter.pos();

	BigInt size = 0;

	auto size_fn = [&size](BigInt value, Int) {
		size += value;
	};

	Int node_indexes[1] = {0};
	balanced_tree::FunctorExtenderState<> node_state(1, node_indexes, size_fn);

	Int data_indexes[0] = {};
	balanced_tree::FunctorExtenderState<> data_state(0, data_indexes, size_fn);

	sequence::SequenceBackwardWalker<
		Types,
		balanced_tree::NodeLTBackwardWalker,
		SelectBackwardWalker,
		balanced_tree::NodeSumExtender,
		SelectExtender,
		balanced_tree::FunctorExtenderState<>
	>
	walker(rank, symbol + 1, symbol, node_state, data_state);

	iter.findBw(walker);

	iter.cache().setup(pos - (size + walker.data_length()) - iter.dataPos() , 0);

	return walker.sum();
}


M_PARAMS
BigInt M_TYPE::countFw(Symbol symbol)
{
	MyType& iter = *me();

	BigInt pos = iter.pos();

	sequence::SequenceForwardWalker<
		Types,
		NodeCountForwardWalker,
		PackedSequenceCountForwardWalker,
		balanced_tree::EmptyExtender,
		balanced_tree::EmptyExtender,
		balanced_tree::EmptyExtenderState
	>
	walker(0, symbol + 1, symbol);

	iter.findFw(walker);

	iter.cache().setup(pos + walker.sum() - iter.dataPos(), 0);

	return walker.sum();
}



M_PARAMS
BigInt M_TYPE::countBw(Symbol symbol)
{
	MyType& iter = *me();

	BigInt pos = iter.pos();

	sequence::SequenceCountBackwardWalker<
		Types,
		NodeCountBackwardWalker,
		PackedSequenceCountBackwardWalker,
		balanced_tree::EmptyExtender,
		balanced_tree::EmptyExtender,
		balanced_tree::EmptyExtenderState
	>
	walker(0, symbol + 1, symbol);

	iter.findBw(walker);

	if (!iter.isBeforeBegin())
	{
		iter.cache().setup(pos - (walker.sum()) - iter.dataPos() , 0);
	}
	else {
		iter.cache().setup(0, 0);
	}

	return walker.sum();
}

M_PARAMS
BigInt M_TYPE::rank(BigInt size, Symbol symbol) const
{
	Int data_size = me()->data()->size();
	Int data_pos  = me()->dataPos();

	if (data_pos + size < data_size)
	{
		return me()->data()->sequence().rank1(data_pos, data_pos + size, symbol);
	}
	else {
		MyType tmp = *me();

		BigInt rank = 0;

		auto rank_fn = [&rank](BigInt value, Int) {
			rank += value;
		};

		Int node_indexes[1] = {(Int)symbol + 1};
		balanced_tree::FunctorExtenderState<> node_state(1, node_indexes, rank_fn);

		Int data_indexes[1] = {(Int)symbol};
		balanced_tree::FunctorExtenderState<> data_state(1, data_indexes, rank_fn);

		typename Types::template SkipForwardWalker<
		   			Types,
		   			balanced_tree::NodeSumExtender,
		   			balanced_tree::RankExtender,
		   			balanced_tree::FunctorExtenderState<>
		> walker(size, 0, node_state, data_state);

		tmp.findFw(walker);

		return rank;
	}

	return 0;
}

M_PARAMS
BigInt M_TYPE::rank(Symbol symbol) const
{
	return 0;
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
