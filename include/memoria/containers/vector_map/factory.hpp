
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






template <typename Profile, Int Indexes_>
struct BTreeTypes<Profile, memoria::VMset<Indexes_> >: public BTreeTypes<Profile, memoria::BSTreeCtr> {

    typedef BTreeTypes<Profile, memoria::BSTreeCtr >                        Base;

    typedef EmptyValue                                                      Value;

    static const Int Indexes                                                = Indexes_;
};


template <typename Profile, typename T, Int Indexes>
class CtrTF<Profile, memoria::VMset<Indexes>, T>: public CtrTF<Profile, memoria::BSTreeCtr, T> {
};




template <typename Profile_>
struct CompositeTypes<Profile_, VectorMapCtr>: public CompositeTypes<Profile_, CompositeCtr> {

    typedef VectorMapCtr                                                        ContainerTypeName;

    typedef CompositeTypes<Profile_, CompositeCtr>                              Base;

    typedef typename appendLists<
                typename Base::ContainerPartsList,
                typename TLTool<
                    memoria::vector_map::CtrApiName
                >::List
    >::Result                                                                   CtrList;

    typedef typename appendLists<
                typename Base::IteratorPartsList,
                typename TLTool<
                    memoria::vector_map::ItrApiName
                >::List
    >::Result                                                                   IterList;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef VectorMapContainerBase<Types_>                                  Type;
    };

};


template <typename Profile_, typename T>
class CtrTF<Profile_, VectorMapCtr, T> {

    typedef CtrTF<Profile_, VectorMapCtr, T>                                    MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, VectorMapCtr>                              ContainerTypes;

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
