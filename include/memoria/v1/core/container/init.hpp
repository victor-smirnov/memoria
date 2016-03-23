
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {

template <Int Order = 200>
struct RootCtrListBuilder {
    typedef typename RootCtrListBuilder<Order - 1>::Type    Type;
};


template <>
struct RootCtrListBuilder<-1> {
    typedef TypeList<>                                        Type;
};


#define MEMORIA_DECLARE_ROOT_CTR(CtrType, Order)                                \
template <>                                                                     \
struct RootCtrListBuilder<Order> {                                              \
    typedef typename AppendTool<CtrType, typename RootCtrListBuilder<Order - 1>::Type>::Result   Type;   \
};


template <typename T = void>
struct RootCtrListProvider;


template <Int Order = 20>
struct ProfileListBuilder {
    typedef typename ProfileListBuilder<Order - 1>::Type    Type;
};


template <>
struct ProfileListBuilder<-1> {
    typedef TypeList<>                                        Type;
};


#define MEMORIA_DECLARE_PROFILE(Profile, Order)                                 \
template <>                                                                     \
struct ProfileListBuilder<Order> {                                              \
    typedef typename AppendTool<Profile, typename ProfileListBuilder<Order - 1>::Type>::Type   Type;   \
}

}}