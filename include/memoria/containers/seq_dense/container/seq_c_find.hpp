
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_FIND_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_FIND_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/seq_dense/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqdense_walkers.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrFindName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Allocator::Page                                            Page;
	typedef typename Page::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
	typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
	typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
	typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
	typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;

	typedef typename Types::DataPage                                            DataPage;
	typedef typename Types::DataPageG                                           DataPageG;
	typedef typename Types::IDataSourceType                                     IDataSourceType;
	typedef typename Types::IDataTargetType                                     IDataTargetType;

	typedef typename Types::TreePath                                            TreePath;
	typedef typename Types::TreePathItem                                        TreePathItem;
	typedef typename Types::DataPathItem                                        DataPathItem;

	typedef typename Base::LeafNodeKeyValuePair                                 LeafNodeKeyValuePair;


	static const Int Indexes                                                    = Types::Indexes;
	typedef typename Types::Accumulator                                         Accumulator;

	typedef typename Types::ElementType                                         ElementType;


	BigInt rank(BigInt pos, Int symbol)
	{
		MyType& ctr = *me();

		BigInt rank = 0;

		auto rank_fn = [&rank](BigInt value, Int) {
			rank += value;
		};

		Int node_indexes[1] = {(Int)symbol + 1};
		Int data_indexes[1] = {(Int)symbol};

		typename Types::template SkipForwardWalker<
			Types,
			balanced_tree::NodeSumExtender,
			balanced_tree::RankExtender,
			balanced_tree::FunctorExtenderState<>
		> walker(
			pos,
			0,
			balanced_tree::FunctorExtenderState<>(1, node_indexes, rank_fn),
			balanced_tree::FunctorExtenderState<>(1, data_indexes, rank_fn)
		);

		ctr.find1(walker);

		return rank;
	}

	Iterator select(BigInt rank, Int symbol)
	{
		MyType& ctr = *me();

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
			balanced_tree::SelectForwardWalker,
			balanced_tree::NodeSumExtender,
			balanced_tree::SelectExtender,
			balanced_tree::FunctorExtenderState<>
		>
		walker(rank, symbol + 1, symbol, node_state, data_state);

		Iterator iter = ctr.find0(walker);

		iter.cache().setup(size - iter.dataPos() - 1 + walker.data_length(), 0);

		return iter;
	}

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrFindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
