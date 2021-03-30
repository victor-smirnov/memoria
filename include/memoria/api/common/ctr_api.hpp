
// Copyright 2017 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/memory/memory.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/core/tools/pair.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/core/iovector/io_vector.hpp>
#include <memoria/core/datatypes/datatypes.hpp>

namespace memoria {

template <typename CtrName, typename Profile>
struct ICtrApi;


template <typename CtrName, typename Profile>
struct ICtrApiTypes;


template <typename DataType_, typename IOSustreamTag_, template <typename DataType> class EncodingTF_ = ValueCodec>
struct ICtrApiSubstream
{
    using DataType      = DataType_;
    using IOSustreamTag = IOSustreamTag_;

    template <typename DataType0>
    using EncodingTF = EncodingTF_<DataType0>;
};

template <
        typename T,
        bool FixedSize = DTTIs1DFixedSize<typename T::DataType>
>
struct IOSubstreamAdapter;


template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;
template <typename CtrName, typename Profile> class SharedIter;

namespace _ {

template <typename Profile, int32_t Streams> struct ProfileCtrSizesTS: HasType<
    core::StaticVector<ProfileCtrSizeT<Profile>, Streams>
> {};

template <typename Profile, int32_t Streams> struct ApiProfileCtrSizesTS: HasType<
    core::StaticVector<ApiProfileCtrSizeT<Profile>, Streams>
> {};

}

template <typename Profile, int32_t Streams> 
using ProfileCtrSizesT = typename _::ProfileCtrSizesTS<Profile, Streams>::Type;

template <typename Profile, int32_t Streams>
using ApiProfileCtrSizesT = typename _::ApiProfileCtrSizesTS<Profile, Streams>::Type;


#define MMA_DECLARE_ICTRAPI()           \
    template <typename ImplProfile>     \
    static void init_profile_metadata()


#define MMA_INSTANTIATE_CTR_BTSS(CtrName, Profile, ...)\
template struct ICtrApi<CtrName, ApiProfile<Profile>>; \
template void ICtrApi<CtrName, ApiProfile<Profile>>::init_profile_metadata<Profile>();


#define MMA_INSTANTIATE_CTR_BTFL(CtrName, Profile, ...) \
template struct ICtrApi<CtrName, ApiProfile<Profile>>;  \
template void ICtrApi<CtrName, ApiProfile<Profile>>::init_profile_metadata<Profile>();

}
