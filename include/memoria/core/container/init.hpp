
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_INIT_HPP
#define _MEMORIA_CORE_CONTAINER_INIT_HPP

#include <memoria/core/types/types.hpp>

namespace memoria {

template <Int Order = 200>
struct RootCtrListBuilder {
    typedef typename RootCtrListBuilder<Order - 1>::Type    Type;
};


template <>
struct RootCtrListBuilder<-1> {
    typedef NullType                                        Type;
};


#define MEMORIA_DECLARE_ROOT_CTR(CtrType, Order)                                \
template <>                                                                     \
struct RootCtrListBuilder<Order> {                                              \
    typedef TL<CtrType, typename RootCtrListBuilder<Order - 1>::Type>   Type;   \
};


template <typename T = void>
struct RootCtrListProvider;


template <Int Order = 20>
struct ProfileListBuilder {
    typedef typename ProfileListBuilder<Order - 1>::Type    Type;
};


template <>
struct ProfileListBuilder<-1> {
    typedef NullType                                        Type;
};


#define MEMORIA_DECLARE_PROFILE(Profile, Order)                                 \
template <>                                                                     \
struct ProfileListBuilder<Order> {                                              \
    typedef TL<Profile, typename ProfileListBuilder<Order - 1>::Type>   Type;   \
}

}

#endif

