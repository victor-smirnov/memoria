
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {
/*
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

*/

}}
