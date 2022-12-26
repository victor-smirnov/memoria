
// Copyright 2017-2022 Victor Smirnov
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
#include <memoria/core/container/store.hpp>
#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/core/tools/pair.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/core/memory/object_pool.hpp>

namespace memoria {



template <typename CtrName, typename Profile>
struct ICtrApi;

template <typename CtrName, typename Profile>
struct ICtrApiTypes;

template <typename CtrName, typename Allocator, typename Profile> class SharedCtr;
template <typename CtrName, typename Allocator, typename Profile> class RWSharedCtr;
template <typename CtrName, typename Profile> class SharedIter;

namespace detail {

template <typename Profile, int32_t Streams> struct ProfileCtrSizesTS: HasType<
    core::StaticVector<ProfileCtrSizeT<Profile>, Streams>
> {};

template <typename Profile, int32_t Streams> struct ApiProfileCtrSizesTS: HasType<
    core::StaticVector<ApiProfileCtrSizeT<Profile>, Streams>
> {};

}

template <typename Profile, int32_t Streams> 
using ProfileCtrSizesT = typename detail::ProfileCtrSizesTS<Profile, Streams>::Type;

template <typename Profile, int32_t Streams>
using ApiProfileCtrSizesT = typename detail::ApiProfileCtrSizesTS<Profile, Streams>::Type;


#define MMA_DECLARE_ICTRAPI()           \
    template <typename ImplProfile>     \
    static void init_profile_metadata()


#define MMA_INSTANTIATE_CTR_BTSS(CtrName, Profile, ...)\
template struct ICtrApi<CtrName, ApiProfile<Profile>>; \
template void ICtrApi<CtrName, ApiProfile<Profile>>::init_profile_metadata<Profile>();


#define MMA_INSTANTIATE_CTR_BTFL(CtrName, Profile, ...) \
template struct ICtrApi<CtrName, ApiProfile<Profile>>;  \
template void ICtrApi<CtrName, ApiProfile<Profile>>::init_profile_metadata<Profile>();

template <typename T, typename ProfileT>
void InitCtrMetadata() {
    ICtrApi<T, ApiProfile<ProfileT>>::template init_profile_metadata<ProfileT>();
}

enum class ChunkDumpMode {
    HEADER, LEAF, PATH
};

template <typename MyType, typename Profile>
struct ChunkIteratorBase {
    virtual ~ChunkIteratorBase() = default;


    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
    using ChunkPtr = IterSharedPtr<MyType>;

    virtual CtrSizeT entry_offset() const = 0;
    virtual CtrSizeT chunk_offset() const = 0;

    virtual size_t chunk_size() const = 0;
    virtual size_t entry_offset_in_chunk() const = 0;

    virtual bool is_before_start() const = 0;
    virtual bool is_after_end() const = 0;

    virtual ChunkPtr next(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr next_chunk() const = 0;

    virtual ChunkPtr prev(CtrSizeT num = 1) const = 0;
    virtual ChunkPtr prev_chunk() const = 0;

    virtual void dump(ChunkDumpMode mode = ChunkDumpMode::HEADER, std::ostream& out = std::cout) const = 0;
};

template <typename ChunkIterT>
bool is_valid_chunk(const IterSharedPtr<ChunkIterT>& ptr) {
    return ptr && !(ptr->is_after_end() || ptr->is_before_start());
}


}
