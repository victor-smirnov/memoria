
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP2_FACTORY_HPP

#include <memoria/containers/map2/map_walkers.hpp>

#include <memoria/containers/map2/container/map_c_api.hpp>
#include <memoria/containers/map2/iterator/map_i_api.hpp>

#include <memoria/prototypes/balanced_tree/balanced_tree.hpp>

#include <memoria/containers/map2/names.hpp>

namespace memoria    {



template <typename Profile, typename Key_, typename Value_, Int Indexes_>
struct BalancedTreeTypes<Profile, memoria::Map2<Key_, Value_, Indexes_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BSTree>                     Base;

    typedef Value_                                                          Value;
    typedef TypeList<Key_>                                                  KeysList;

    static const Int Indexes                                                = Indexes_;

    typedef typename MergeLists<
            	typename Base::ContainerPartsList,
                memoria::map2::CtrApiName
    >::Result                                                               ContainerPartsList;

    typedef typename MergeLists<
            	typename Base::IteratorPartsList,
                memoria::map2::ItrApiName
    >::Result                                                               IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef balanced_tree::BTreeIteratorPrefixCache<Iterator, Container>               Type;
    };



    template <typename Types>
    using FindLTWalker 		= ::memoria::map2::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::map2::FindLEWalker<Types>;



    template <typename Types>
    using FindBeginWalker 	= ::memoria::map2::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::map2::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::map2::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::map2::FindREndWalker<Types>;
};

template <typename Profile, typename Key, typename Value, typename T, Int Indexes>
class CtrTF<Profile, memoria::Map2<Key, Value, Indexes>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};

}

#endif
