
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTAINERS_LOUDS_FACTORY_HPP_
#define MEMORIA_CONTAINERS_LOUDS_FACTORY_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/containers/louds/louds_names.hpp>

#include <memoria/containers/louds/container/louds_c_api.hpp>
#include <memoria/containers/louds/container/louds_c_base.hpp>

#include <memoria/containers/louds/iterator/louds_i_api.hpp>

namespace memoria {

template <typename Profile_>
struct CompositeTypes<Profile_, LOUDS>: public CompositeTypes<Profile_, Composite> {

    typedef LOUDS                                             					ContainerTypeName;

    typedef CompositeTypes<Profile_, Composite>                                 Base;

    typedef typename AppendTool<
                typename Base::ContainerPartsList,
                TypeList<
                    memoria::louds::CtrApiName
                >
    >::Result                                                                   CtrList;

    typedef typename AppendTool<
                typename Base::IteratorPartsList,
                TypeList<
                    memoria::louds::ItrApiName
                >
    >::Result                                                                   IterList;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef LoudsContainerBase<Types_>					Type;
    };
};


template <typename Profile_, typename T>
class CtrTF<Profile_, LOUDS, T> {

    typedef CtrTF<Profile_, LOUDS, T>                           				MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, LOUDS>                    					ContainerTypes;

public:

    struct Types: public ContainerTypes
    {
        typedef BigInt 							Key;
        typedef UBigInt							Value;

    	typedef LOUDS           				Name;

        typedef Profile_                        Profile;
        typedef MyType::Allocator               Allocator;

        typedef LoudsCtrTypes<Types>        	CtrTypes;
        typedef LoudsIterTypes<Types>       	IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}


#endif
