
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTAINERS_LOUDS_FACTORY_HPP_
#define MEMORIA_CONTAINERS_LOUDS_FACTORY_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/containers/louds/louds_names.hpp>

#include <memoria/containers/louds/container/louds_c_api.hpp>
#include <memoria/containers/louds/iterator/louds_i_api.hpp>


#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

namespace memoria {

template <typename Profile_>
struct WrapperTypes<Profile_, CtrWrapper<LOUDS>>: WrapperTypes<Profile_, LOUDS> {

	typedef WrapperTypes<Profile_, LOUDS>   			Base;

	typedef typename AppendTool<
			typename Base::ContainerPartsList,
			TypeList<
				memoria::louds::CtrApiName
			>
	>::Result                                           CtrList;

	typedef typename AppendTool<
			typename Base::IteratorPartsList,
			TypeList<
				memoria::louds::ItrApiName
			>
	>::Result                                           IterList;

	typedef Sequence<1, true>           				WrappedCtrName;
};



template <typename Profile_, typename T>
class CtrTF<Profile_, LOUDS, T>: public CtrTF<Profile_, CtrWrapper<LOUDS>, T> {

	typedef CtrTF<Profile_, CtrWrapper<LOUDS>, T> 								Base;

public:

    struct Types: Base::Types {

    	typedef LoudsCtrTypes<Types> 		CtrTypes;
        typedef LoudsIterTypes<Types> 		IterTypes;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}


#endif
