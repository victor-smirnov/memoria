
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_FACTORY_HPP
#define _MEMORIA_MODELS_ARRAY_FACTORY_HPP




#include <memoria/containers/map/factory.hpp>

#include <memoria/containers/vector/iterator/api.hpp>

#include <memoria/containers/vector/pages/data_page.hpp>
#include <memoria/containers/vector/pages/metadata.hpp>

#include <memoria/containers/vector/container/api.hpp>

#include <memoria/containers/vector/names.hpp>
#include <memoria/containers/vector/tools.hpp>

#include <memoria/prototypes/dynvector/dynvector.hpp>

namespace memoria {

template <typename Profile, typename ElementType>
struct BTreeTypes<Profile, memoria::VectorCtr<ElementType>>: public BTreeTypes<Profile, memoria::DynVectorCtr<ElementType>>  {

    typedef BTreeTypes<Profile, memoria::DynVectorCtr<ElementType>>                 Base;

    typedef typename AppendTool<
            typename Base::ContainerPartsList,
            TypeList<
                memoria::models::array::ApiName
            >
    >::Result                                                                       ContainerPartsList;

    typedef typename AppendTool<
            typename Base::IteratorPartsList,
            TypeList<
                memoria::models::array::IteratorContainerAPIName
            >
    >::Result                                                                       IteratorPartsList;

    typedef ArrayData<ElementType>                                                  Buffer;

    template <Int Size>
    struct DataBlockTypeFactory {
        typedef memoria::array::DynVectorData<ElementType, Size>                    Type;
    };

    typedef VectorMetadata<typename Base::ID>                                       Metadata;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorScalarPrefixCache<Iterator, Container>                 Type;
    };
};


template <typename Profile, typename T, typename ElementType>
class CtrTF<Profile, memoria::VectorCtr<ElementType>, T>: public CtrTF<Profile, memoria::DynVectorCtr<ElementType>, T> {

};

}

#endif
