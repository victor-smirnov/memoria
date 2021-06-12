
// Copyright 2019-2021 Victor Smirnov
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

#include <memoria/core/strings/format.hpp>

namespace memoria {

template <typename Profile>
struct AllocationMapIterator: BTSSIterator<Profile> {
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    virtual ~AllocationMapIterator() noexcept = default;

    virtual bool is_end() const = 0;
    virtual bool next() = 0;

    virtual CtrSizeT count_fw() = 0;
    virtual CtrSizeT level0_pos() const = 0;
};

template <typename Profile>
class AllocationMetadata {
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
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

    // Position is Level_0-scaled.
    CtrSizeT position() const noexcept {
        return position_;
    }

    CtrSizeT size() const noexcept {
        return size_;
    }

    int32_t level() const noexcept {
        return level_;
    }

    void enlarge(CtrSizeT amnt) noexcept {
        size_ += amnt;
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
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    using ApiTypes  = ICtrApiTypes<AllocationMap, Profile>;

    using IteratorPtr = CtrSharedPtr<AllocationMapIterator<Profile>>;

    using ALCMeta = AllocationMetadata<Profile>;

    static constexpr int32_t  LEVELS          = 9;
    static constexpr CtrSizeT ALLOCATION_SIZE = 512;

    virtual CtrSizeT size() const = 0;

    virtual IteratorPtr seek(CtrSizeT position) = 0;
    virtual IteratorPtr iterator() = 0;

    virtual CtrSizeT expand(CtrSizeT blocks) = 0;
    virtual void shrink(CtrSizeT size) = 0;

    virtual Optional<AllocationMapEntryStatus> get_allocation_status(int32_t level, CtrSizeT position) = 0;

    virtual CtrSizeT rank(CtrSizeT pos) = 0;

    virtual CtrSizeT find_unallocated(
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<ALCMeta>& buffer
    ) = 0;

    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn) = 0;

    virtual CtrSizeT setup_bits(Span<const ALCMeta> allocations, bool set_bits) = 0;
    virtual CtrSizeT touch_bits(Span<const ALCMeta> allocations) = 0;

    virtual CtrSizeT mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size) = 0;
    virtual CtrSizeT mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size) = 0;

    virtual CtrSizeT unallocated_at(int32_t level) = 0;
    virtual void unallocated(Span<CtrSizeT> ranks) = 0;


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

namespace fmt {

template <typename Profile>
struct formatter<memoria::AllocationMetadata<Profile>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::AllocationMetadata<Profile>& alc, FormatContext& ctx) {
        return format_to(ctx.out(), "[{}, {}, {}]", alc.position(), alc.size(), alc.level());
    }
};

}
