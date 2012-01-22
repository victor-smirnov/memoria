
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP
#define	_MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/bstree/container/find.hpp>
#include <memoria/prototypes/bstree/container/tools.hpp>

#include <memoria/prototypes/bstree/iterator/base.hpp>
#include <memoria/prototypes/bstree/iterator/tools.hpp>
#include <memoria/prototypes/bstree/iterator/multiskip.hpp>

#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/prototypes/bstree/macros.hpp>

namespace memoria    {



template <typename Profile>
struct BTreeTypes<Profile, memoria::BSTree>: public BTreeTypes<Profile, memoria::BTree> {
	typedef BTreeTypes<Profile, memoria::BTree> 							Base;

	typedef BigInt															Value;

	static const bool MapType                                               = MapTypes::Index;

	typedef typename AppendLists<
			typename Base::ContainerPartsList,
			typename TLTool<
				memoria::bstree::ToolsName,
				memoria::bstree::FindName
			>::List
	>::Result                                                               ContainerPartsList;

	typedef typename AppendLists<
			typename Base::IteratorPartsList,
			typename TLTool<
				memoria::bstree::IteratorToolsName,
				memoria::bstree::IteratorMultiskipName
			>::List
	>::Result                                                               IteratorPartsList;

	template <
		typename Types_
	>
	struct IterBaseFactory {
		typedef ITreeIteratorBase<Types_> 										Type;
	};

};

template <typename Profile, typename T>
class CtrTF<Profile, memoria::BSTree, T>: public CtrTF<Profile, memoria::BTree, T> {};


}

#endif
