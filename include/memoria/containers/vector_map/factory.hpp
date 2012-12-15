
// Copyright Victor Smirnov, Ivan Yurchenko 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP

#include <memoria/containers/vector_map/names.hpp>

#include <memoria/containers/vector_map/container/base.hpp>
#include <memoria/containers/vector_map/container/api.hpp>

#include <memoria/containers/vector_map/iterator/api.hpp>
#include <memoria/containers/vector_map/iterator.hpp>

#include <memoria/containers/vector_map/iterator/api.hpp>

#include <memoria/containers/set/factory.hpp>
#include <memoria/containers/vector/factory.hpp>

#include <memoria/prototypes/composite/factory.hpp>



namespace memoria {

using namespace memoria::vector_map;






template <typename Profile, typename Key, Int Indexes_>
struct BTreeTypes<Profile, memoria::VMSet<Key, Indexes_> >: public BTreeTypes<Profile, memoria::BSTree> {

    typedef BTreeTypes<Profile, memoria::BSTree>                            Base;

    typedef EmptyValue                                                      Value;

    static const Int Indexes                                                = Indexes_;
};


template <typename Profile, typename T, typename Key, Int Indexes>
class CtrTF<Profile, memoria::VMSet<Key, Indexes>, T>: public CtrTF<Profile, memoria::BSTree, T> {
};


template <typename Profile_, typename Key_, typename Value_>
struct CompositeTypes<Profile_, VectorMap<Key_,Value_>>: public CompositeTypes<Profile_, Composite> {

    typedef VectorMap<Key_,Value_>                                              ContainerTypeName;

    typedef Key_                                                                Key;
    typedef Value_                                                              Value;


    typedef CompositeTypes<Profile_, Composite>                                 Base;

    typedef typename AppendTool<
                typename Base::ContainerPartsList,
                TypeList<
                    memoria::vector_map::CtrApiName
                >
    >::Result                                                                   CtrList;

    typedef typename AppendTool<
                typename Base::IteratorPartsList,
                TypeList<
                    memoria::vector_map::ItrApiName
                >
    >::Result                                                                   IterList;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef VectorMapContainerBase<Types_>                                  Type;
    };

};


template <typename Profile_, typename T, typename Key, typename Value>
class CtrTF<Profile_, VectorMap<Key, Value>, T> {

    typedef CtrTF<Profile_, VectorMap<Key, Value>, T>                           MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, VectorMap<Key, Value> >                    ContainerTypes;

public:

    struct Types: public ContainerTypes
    {
        typedef Profile_                        Profile;
        typedef MyType::Allocator               Allocator;

        typedef VectorMapCtrTypes<Types>        CtrTypes;
        typedef VectorMapIterTypes<Types>       IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}

#endif
