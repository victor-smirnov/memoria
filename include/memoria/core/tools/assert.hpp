
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




#ifndef _MEMORIA_CORE_TOOLS_ASSERT_H
#define _MEMORIA_CORE_TOOLS_ASSERT_H

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>

#include <iostream>

namespace memoria    {

template <bool Value> class STATIC_ASSERT_FAILURE;
template <> class STATIC_ASSERT_FAILURE <true> {};

#define MEMORIA_STATIC_ASSERT(B) \
    enum { MEMORIA_JOIN(MEMORIA_static_assert_, __LINE__) = sizeof(::memoria::STATIC_ASSERT_FAILURE<(bool)(B)>)}

}

#endif
