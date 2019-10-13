
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_referenceable.hpp>

#include <memoria/v1/core/tools/pair.hpp>

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>
#include <memoria/v1/api/datatypes/datatypes.hpp>

namespace memoria {
namespace v1 {

template <typename CtrName, typename Profile = DefaultProfile<>>
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

}

template <typename Profile, int32_t Streams> 
using ProfileCtrSizesT = typename _::ProfileCtrSizesTS<Profile, Streams>::Type;


template <typename CtrName, typename Profile>
struct CtrMetadataInitializer {
    CtrMetadataInitializer() {
        ICtrApi<CtrName, Profile>::init_profile_metadata();
    }
};

#define MMA1_DECLARE_ICTRAPI() \
    static void init_profile_metadata()








#define MMA1_INSTANTIATE_CTR_BTSS(CtrName, Profile, ...)\
template struct ICtrApi<CtrName, Profile>;               \
namespace {                                             \
    DataTypeRegistryStore::Initializer<CtrName, TL<>> init_type_##__VA_ARGS__;\
    CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}


#define MMA1_INSTANTIATE_CTR_BTFL(CtrName, Profile, ...) \
template struct ICtrApi<CtrName, Profile>;               \
                                                         \
namespace {                                              \
    CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}

}
}
