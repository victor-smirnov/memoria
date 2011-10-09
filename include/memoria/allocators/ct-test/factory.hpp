
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_ALLOCATORS_CTTEST_FACTORY_HPP
#define _MEMORIA_ALLOCATORS_CTTEST_FACTORY_HPP

//#include <memoria/core/container/checker.hpp>
//#include <memoria/core/container/collection.hpp>
//
//#include <memoria/containers/root/factory.hpp>
//#include <memoria/containers/kvmap/factory.hpp>
//#include <memoria/containers/idx_map/factory.hpp>
//#include <memoria/containers/array/factory.hpp>


#include "names.hpp"
#include "allocator.hpp"

namespace memoria    {


template <typename Profile>
class ContainerCollectionCfg;

template <typename T>
class ContainerCollectionCfg<CompileTimeTestProfile<T> > {

    struct StreamContainerCollectionCfg: public BasicContainerCollectionCfg<CompileTimeTestProfile<T> > {
        typedef BasicContainerCollectionCfg<CompileTimeTestProfile<T> > Base;

        typedef memoria::CtTestAllocator<
        		CompileTimeTestProfile<T>,
					typename Base::Page,
					typename Base::Transaction
				>  	 															AllocatorType;
    };

public:
    typedef StreamContainerCollectionCfg                      							Types;

};




}

#endif
