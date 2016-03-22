
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/containers/vector_tree/vtree_names.hpp>
#include <memoria/containers/vector_tree/vtree_tools.hpp>
#include <memoria/containers/vector_tree/vtree_iterator.hpp>
#include <memoria/containers/vector_tree/vtree_walkers.hpp>

#include <memoria/containers/vector_tree/container/vtree_c_base.hpp>
#include <memoria/containers/vector_tree/container/vtree_c_api.hpp>


#include <memoria/containers/vector_tree/iterator/vtree_i_api.hpp>

#include <memoria/containers/labeled_tree/ltree_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/prototypes/composite/comp_factory.hpp>



namespace memoria {



template <typename Profile_>
struct CompositeTypes<Profile_, VTree>: public CompositeTypes<Profile_, Composite> {

    typedef VTree                                                               ContainerTypeName;

    typedef CompositeTypes<Profile_, Composite>                                 Base;

    using CtrList = MergeLists<
            typename Base::ContainerPartsList,

            vtree::CtrApiName
    >;

    using IterList = MergeLists<
            typename Base::IteratorPartsList,

            vtree::ItrApiName
    >;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef vtree::VTreeCtrBase<Types_>                                     Type;
    };
};


template <typename Profile_, typename T>
class CtrTF<Profile_, VTree, T> {

    typedef CtrTF<Profile_, VTree, T>                                           MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, VTree>                                     ContainerTypes;

public:

    struct Types: public ContainerTypes
    {
        typedef Profile_                                        Profile;
        typedef MyType::Allocator                               Allocator;

        typedef VTreeCtrTypes<Types>                            CtrTypes;
        typedef VTreeIterTypes<Types>                           IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}
