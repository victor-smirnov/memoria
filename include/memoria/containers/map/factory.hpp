
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP_FACTORY_HPP

#include <memoria/containers/map/container/api.hpp>
#include <memoria/containers/map/iterator/api.hpp>

#include <memoria/prototypes/bstree/factory.hpp>

namespace memoria    {



template <typename Profile, Int Indexes_>
struct BTreeTypes<Profile, memoria::Map<Indexes_> >: public BTreeTypes<Profile, memoria::BSTreeCtr> {

    typedef BTreeTypes<Profile, memoria::BSTreeCtr>                         Base;

    typedef BigInt                                                          Value;

    static const Int Indexes                                                = Indexes_;

    typedef typename AppendTool<
            typename Base::ContainerPartsList,
            TypeList<
                memoria::models::idx_map::CtrApiName
            >
    >::Result                                                               ContainerPartsList;

    typedef typename AppendTool<
            typename Base::IteratorPartsList,
            TypeList<
                memoria::models::idx_map::ItrApiName
            >
    >::Result                                                               IteratorPartsList;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorPrefixCache<Iterator, Container>               Type;
    };


};

template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::Map<Indexes>, T>: public CtrTF<Profile, memoria::BSTreeCtr, T> {
};

}

#endif
