
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_DEFAULTS_HPP
#define _MEMORIA_CORE_CONTAINER_DEFAULTS_HPP

#include <memoria/core/container/names.hpp>
#include <memoria/core/container/pages.hpp>

#include <memoria/core/container/allocator.hpp>

namespace memoria    {


class AbstractTransaction {
public:
    AbstractTransaction() {}
};

using namespace memoria::vapi;


template <
	typename Profile, typename IDValueType = BigInt, int FlagsCount = 32, typename TransactionType = AbstractTransaction
>
struct BasicContainerCollectionCfg {

    typedef PageID<IDValueType>                                                 ID;
    typedef AbstractPage <ID, FlagsCount>                                       Page;
    typedef TransactionType                                                     Transaction;

    typedef IAllocator<Page>                                                    AbstractAllocator;

    typedef AbstractAllocator                                                   AllocatorType;
};


}


#endif

