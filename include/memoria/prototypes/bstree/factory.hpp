
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_ITREE_FACTORY_HPP

#include <memoria/prototypes/btree/btree.hpp>
#include <memoria/prototypes/bstree/tools.hpp>

#include <memoria/prototypes/bstree/container/find.hpp>
#include <memoria/prototypes/bstree/container/tools.hpp>

#include <memoria/prototypes/bstree/iterator/base.hpp>
#include <memoria/prototypes/bstree/iterator/api.hpp>

#include <memoria/prototypes/bstree/names.hpp>
#include <memoria/prototypes/bstree/macros.hpp>

namespace memoria    {



template <typename Profile>
struct BTreeTypes<Profile, memoria::BSTreeCtr>: public BTreeTypes<Profile, memoria::BTreeCtr> {
    typedef BTreeTypes<Profile, memoria::BTreeCtr>                          Base;

    typedef BigInt                                                          Value;

    static const bool MapType                                               = MapTypes::Sum;

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
                memoria::bstree::ItrApiName
            >
    >::Result                                                               IteratorPartsList;

    template <
        typename Types_
    >
    struct IterBaseFactory {
        typedef ITreeIteratorBase<Types_>                                       Type;
    };


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorPrefixCache<Iterator, Container>                   Type;
    };

};

template <typename Profile, typename T>
class CtrTF<Profile, memoria::BSTreeCtr, T>: public CtrTF<Profile, memoria::BTreeCtr, T> {};


}

#endif
