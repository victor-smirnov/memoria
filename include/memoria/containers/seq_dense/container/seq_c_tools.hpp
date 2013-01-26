
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_SEQ_C_TOOLS_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/seq_dense/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrToolsName)

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

	typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
	typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

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
	static const Int Symbols                                                    = DataPage::Sequence::Symbols;


	typedef Accumulators<Key, Indexes>                                          Accumulator;

	typedef typename Types::ElementType                                         ElementType;

	Accumulator getDataIndexes(const DataPageG& data, Int from, Int to)
	{
		Accumulator keys;

		if (Indexes == 2)
		{
			keys[1] = data->sequence().rank(from, to - 1, 1);
		}
		else {
			BigInt sum = 0;

			for (Int c = 0; c < Symbols; c++)
			{
				keys[c + 1] = data->sequence().rank(from, to - 1, c);

				sum += keys[c + 1];
			}

			MEMORIA_ASSERT(sum, =, to - from);
		}

		keys[0] = to - from;

		return keys;
	}


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
