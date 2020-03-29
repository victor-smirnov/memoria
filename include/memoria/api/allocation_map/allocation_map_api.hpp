
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

#include <memoria/api/common/ctr_api_btss.hpp>
#include <memoria/api/common/iobuffer_adatpters.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/types/typehash.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/api/allocation_map/allocation_map_scanner.hpp>
#include <memoria/api/allocation_map/allocation_map_producer.hpp>
#include <memoria/api/allocation_map/allocation_map_api_factory.hpp>

#include <memoria/core/strings/string_codec.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>
#include <memoria/core/tools/span.hpp>

namespace memoria {

template <typename Profile>
struct AllocationMapIterator: BTSSIterator<Profile> {
    using CtrSizeT = ProfileCtrSizeT<Profile>;

    virtual ~AllocationMapIterator() noexcept {}

    virtual bool is_end() const noexcept = 0;
    virtual BoolResult next() noexcept = 0;

    virtual Result<CtrSizeT> count_fw() noexcept = 0;
    virtual Result<CtrSizeT> level0_pos() const noexcept = 0;
};

template <typename Profile>
class AllocationMetadata {
    using CtrSizeT = ProfileCtrSizeT<Profile>;
    CtrSizeT position_;
    CtrSizeT size_;
    int32_t level_;
public:
    AllocationMetadata() noexcept :
        position_(), size_(), level_()
    {}

    AllocationMetadata(CtrSizeT position, CtrSizeT size, int32_t level) noexcept :
        position_(position), size_(size), level_(level)
    {}

    CtrSizeT position() const noexcept {
        return position_;
    }

    CtrSizeT size() const noexcept {
        return size_;
    }

    int32_t level() const noexcept {
        return level_;
    }

    bool operator<(const AllocationMetadata& other) noexcept {
        return position_ < other.position_;
    }

    AllocationMetadata take(int64_t amount) noexcept
    {
        AllocationMetadata meta{position_, amount, level_};
        size_ -= amount;
        position_ += (amount << level_);
        return meta;
    }


    AllocationMetadata take_for(int64_t amount, int32_t level) noexcept
    {
        AllocationMetadata meta{position_, amount << (level_ - level), level};
        size_ -= amount;
        position_ += (amount << level_);
        return meta;
    }

    AllocationMetadata take_all_for(int32_t level) noexcept
    {
        AllocationMetadata meta{position_, size_ << (level_ - level), level};
        size_ = 0;
        position_ += size_ << level_;
        return meta;
    }
};

template <typename Profile>
std::ostream& operator<<(std::ostream& out, const AllocationMetadata<Profile>& meta)
{
    out << "[" << meta.position() << ", " << meta.size() << ", " << meta.level() << "]";
    return out;
}

enum class AllocationMapEntryStatus: int32_t {
    FREE = 0, ALLOCATED = 1
};


template <typename Profile>
struct ICtrApi<AllocationMap, Profile>: public CtrReferenceable<Profile> {
    using CtrSizeT = ProfileCtrSizeT<Profile>;

    using ApiTypes  = ICtrApiTypes<AllocationMap, Profile>;

    using IteratorResult = Result<CtrSharedPtr<AllocationMapIterator<Profile>>>;
    using CtrSizeTResult = Result<CtrSizeT>;

    static constexpr int32_t  LEVELS          = 9;
    static constexpr CtrSizeT ALLOCATION_SIZE = 512;

    virtual CtrSizeTResult size() const noexcept = 0;

    virtual IteratorResult seek(CtrSizeT position) noexcept = 0;
    virtual IteratorResult iterator() noexcept = 0;

    virtual CtrSizeTResult expand(CtrSizeT blocks) noexcept = 0;
    virtual VoidResult shrink(CtrSizeT size) noexcept = 0;

    virtual Result<Optional<AllocationMapEntryStatus>> get_allocation_status(int32_t level, CtrSizeT position) noexcept = 0;

    virtual CtrSizeTResult rank(CtrSizeT pos) noexcept = 0;

    virtual VoidResult find_unallocated(
            CtrSizeT from,
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<AllocationMetadata<Profile>>& buffer
    ) noexcept = 0;

    virtual CtrSizeTResult setup_bits(Span<const AllocationMetadata<Profile>> allocations, bool set_bits) noexcept = 0;
    virtual CtrSizeTResult touch_bits(Span<const AllocationMetadata<Profile>> allocations) noexcept = 0;

    virtual CtrSizeTResult mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size) noexcept = 0;
    virtual CtrSizeTResult mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size) noexcept = 0;

    virtual CtrSizeTResult unallocated_at(int32_t level) noexcept = 0;
    virtual VoidResult unallocated(Span<CtrSizeT> ranks) noexcept = 0;


    MMA_DECLARE_ICTRAPI();
};

}

namespace std {

template <typename Profile>
void swap(memoria::AllocationMetadata<Profile>& one, memoria::AllocationMetadata<Profile>& two) noexcept {
    auto tmp = one;
    one = two;
    two = tmp;
}

}
