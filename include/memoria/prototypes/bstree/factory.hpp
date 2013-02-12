
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP

#include <memoria/prototypes/btree/btree.hpp>
#include <memoria/prototypes/bstree/tools.hpp>
#include <memoria/prototypes/bstree/bstree_walkers.hpp>

#include <memoria/prototypes/bstree/container/bstree_c_find.hpp>
#include <memoria/prototypes/bstree/container/bstree_c_tools.hpp>

#include <memoria/prototypes/bstree/iterator/bstree_i_base.hpp>
#include <memoria/prototypes/bstree/iterator/bstree_i_api.hpp>
#include <memoria/prototypes/bstree/iterator/bstree_i_find.hpp>

#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/prototypes/bstree/macros.hpp>

namespace memoria    {



template <typename Profile>
struct BTreeTypes<Profile, memoria::BSTree>: public BTreeTypes<Profile, memoria::BTree> {
    typedef BTreeTypes<Profile, memoria::BTree>                             Base;

    typedef BigInt                                                          Value;

    typedef typename AppendTool<
            typename Base::ContainerPartsList,
            TypeList<
                memoria::bstree::ToolsName,
                memoria::bstree::FindName
            >
    >::Result                                                               ContainerPartsList;

    typedef typename AppendTool<
            typename Base::IteratorPartsList,
            TypeList<
                memoria::bstree::IterApiName,
                memoria::bstree::IterFindName
            >
    >::Result                                                               IteratorPartsList;

    template <
        typename Types_
    >
    struct IterBaseFactory {
        typedef ITreeIteratorBase<Types_>                                   Type;
    };


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorPrefixCache<Iterator, Container>               Type;
    };




    template <typename Types>
    using FindLTWalker 		= ::memoria::bstree::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::bstree::FindLEWalker<Types>;


    template <typename Types>
    using FindBeginWalker 	= ::memoria::bstree::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::bstree::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::bstree::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::bstree::FindREndWalker<Types>;
};


template <typename Profile, typename T>
class CtrTF<Profile, memoria::BSTree, T>: public CtrTF<Profile, memoria::BTree, T> {

    typedef CtrTF<Profile, memoria::BTree, T> Base;

public:

    struct Types: Base::Types {
        typedef BSTreeCtrTypes<Types> CtrTypes;
        typedef BSTreeIterTypes<Types> IterTypes;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;
};

}

#endif
