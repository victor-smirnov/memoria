
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_WT_FACTORY_HPP
#define _MEMORIA_CONTAINERS_WT_FACTORY_HPP


#include <memoria/containers/wt/wt_names.hpp>
#include <memoria/containers/wt/wt_tools.hpp>
#include <memoria/containers/wt/wt_iterator.hpp>
#include <memoria/containers/wt/wt_walkers.hpp>

#include <memoria/containers/wt/container/wt_c_base.hpp>
#include <memoria/containers/wt/container/wt_c_api.hpp>
#include <memoria/containers/wt/container/wt_c_ctree.hpp>

#include <memoria/containers/wt/iterator/wt_i_api.hpp>

#include <memoria/containers/labeled_tree/ltree_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>

#include <memoria/prototypes/composite/comp_factory.hpp>

#include <memoria/containers/wt/factory/wt_lt_factory.hpp>


namespace memoria {



template <typename Profile_>
struct CompositeTypes<Profile_, WT>: public CompositeTypes<Profile_, Composite> {

    typedef WT                                                                  ContainerTypeName;

    typedef CompositeTypes<Profile_, Composite>                                 Base;

    using CtrList = MergeLists<
            typename Base::ContainerPartsList,

            wt::CtrApiName,
            wt::CtrCTreeName
    >;

    using IterList = MergeLists<
            typename Base::IteratorPartsList,

            wt::ItrApiName
    >;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef wt::WTCtrBase<Types_>                                           Type;
    };
};


template <typename Profile_, typename T>
class CtrTF<Profile_, WT, T> {

    typedef CtrTF<Profile_, WT, T>                                              MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, WT>                                        ContainerTypes;

public:

    struct Types: public ContainerTypes
    {
        typedef Profile_                                        Profile;
        typedef MyType::Allocator                               Allocator;

        typedef WTCtrTypes<Types>                               CtrTypes;
        typedef WTIterTypes<Types>                              IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}
#endif
