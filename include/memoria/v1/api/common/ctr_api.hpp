
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
        bool FixedSize = DataTypeTraits<typename T::DataType>::isFixedSize
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
                                                        \
namespace {                                             \
    CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}


//template class IterApiBase<CtrName, Profile>;           \
//template class IterApiBTSSBase<CtrName, Profile>;       \
//template class CtrApiBase<CtrName, Profile>;            \
//template class CtrApiBTSSBase<CtrName, Profile>;        \


#define MMA1_INSTANTIATE_CTR_BTFL(CtrName, Profile, ...) \
template struct ICtrApi<CtrName, Profile>;               \
                                                         \
namespace {                                              \
    CtrMetadataInitializer<CtrName, Profile> init_##__VA_ARGS__ ;\
}




#define MMA1_DECLARE_CTRAPI_BASIC_METHODS()                                            \
    CtrApi(const CtrSharedPtr<AllocatorT>& allocator, int32_t command, const CtrID& ctr_id):\
        Base(allocator, command, ctr_id) {}                                                 \
    ~CtrApi() {}                                                                            \
                                                                                            \
    CtrApi(const CtrApi& other): Base(other) {}                                             \
    CtrApi(const CtrPtr& ptr): Base(ptr) {}                                                 \
    CtrApi(CtrApi&& other): Base(std::move(other)) {}                                       \
                                                                                            \
    CtrApi& operator=(const CtrApi& other) {Base::operator=(other); return *this;}          \
    CtrApi& operator=(CtrApi&& other){Base::operator=(std::move(other)); return *this;}
    
    
    
    


#define MMA1_DECLARE_ITERAPI_BASIC_METHODS()        \
    IterApi(IterPtr ptr):Base(ptr) {}               \
    ~IterApi() {}                                   \
                                                    \
    IterApi(const IterApi& other): Base(other) {}   \
    IterApi(IterApi&& other):                       \
        Base(std::move(other)) {}                   \
                                                    \
    IterApi& operator=(const IterApi& other) {      \
        Base::operator=(other); return *this;       \
    }                                               \
    IterApi& operator=(IterApi&& other) {           \
        Base::operator=(std::move(other));          \
        return *this;                               \
    }

}
}
