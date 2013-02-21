
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP

#include <memoria/containers/map/map_walkers.hpp>

#include <memoria/containers/map/container/map_c_api.hpp>
#include <memoria/containers/map/iterator/map_i_api.hpp>

#include <memoria/prototypes/bstree/factory.hpp>

namespace memoria    {



template <typename Profile, typename Key_, typename Value_, Int Indexes_>
struct BTreeTypes<Profile, memoria::Map<Key_, Value_, Indexes_> >: public BTreeTypes<Profile, memoria::BSTree> {

    typedef BTreeTypes<Profile, memoria::BSTree>                            Base;

    typedef Value_                                                          Value;
    typedef TypeList<Key_>                                                  KeysList;

    static const Int Indexes                                                = Indexes_;

    typedef typename MergeLists<
            	typename Base::ContainerPartsList,
                memoria::map::CtrApiName
    >::Result                                                               ContainerPartsList;

    typedef typename MergeLists<
            	typename Base::IteratorPartsList,
                memoria::map::ItrApiName
    >::Result                                                               IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorPrefixCache<Iterator, Container>               Type;
    };



    template <typename Types>
    using FindLTWalker 		= ::memoria::map::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::map::FindLEWalker<Types>;


    template <typename Types>
    using FindBeginWalker 	= ::memoria::map::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::map::FindREndWalker<Types>;
};

template <typename Profile, typename Key, typename Value, typename T, Int Indexes>
class CtrTF<Profile, memoria::Map<Key, Value, Indexes>, T>: public CtrTF<Profile, memoria::BSTree, T> {
};

}

#endif
