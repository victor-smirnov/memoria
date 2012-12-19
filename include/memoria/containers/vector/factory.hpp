
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_FACTORY_HPP
#define _MEMORIA_MODELS_ARRAY_FACTORY_HPP

#include <memoria/containers/map/factory.hpp>

#include <memoria/containers/vector/iterator/api.hpp>
#include <memoria/containers/vector/iterator.hpp>

#include <memoria/containers/vector/pages/data_page.hpp>
#include <memoria/containers/vector/pages/metadata.hpp>

#include <memoria/containers/vector/container/api.hpp>

#include <memoria/containers/vector/names.hpp>
#include <memoria/containers/vector/tools.hpp>

#include <memoria/prototypes/dynvector/dynvector.hpp>

#include <memoria/containers/vector/tools.hpp>

namespace memoria {

template <typename Profile, typename ElementType>
struct BTreeTypes<Profile, memoria::Vector<ElementType>>: public BTreeTypes<Profile, memoria::DynVector<ElementType>>  {

    typedef BTreeTypes<Profile, memoria::DynVector<ElementType>>                Base;

    typedef typename AppendTool<
            typename Base::ContainerPartsList,
            TypeList<
                memoria::mvector::ApiName
            >
    >::Result                                                                   ContainerPartsList;

    typedef typename AppendTool<
            typename Base::IteratorPartsList,
            TypeList<
                memoria::mvector::IteratorContainerAPIName
            >
    >::Result                                                                   IteratorPartsList;

    typedef MemBuffer<ElementType>                                              Buffer;

    typedef memoria::DynVectorData<ElementType>                                 DataBlock;

    typedef VectorMetadata<typename Base::ID>                                   Metadata;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorScalarPrefixCache<Iterator, Container>             Type;
    };
};


template <typename Profile, typename T, typename ElementType>
class CtrTF<Profile, memoria::Vector<ElementType>, T>: public CtrTF<Profile, memoria::DynVector<ElementType>, T> {
    typedef CtrTF<Profile, memoria::DynVector<ElementType>, T> Base;
public:

    struct Types: Base::Types {
        typedef VectorCtrTypes<Types>   CtrTypes;
        typedef VectorIterTypes<Types>  IterTypes;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;

    typedef Ctr<CtrTypes>                                                       Type;

};

}

#endif
