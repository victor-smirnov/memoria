
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/types/typehash.hpp>


#include <memoria/api/collection/collection_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api_factory.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/any_id.hpp>

#include <memoria/core/strings/format.hpp>

namespace memoria {

struct PkdAllocationMapTypes {};

template <typename Types>
class PkdAllocationMap;

template <typename Profile>
struct AllocationMapChunk: ChunkIteratorBase<AllocationMapChunk<Profile>, Profile> {

    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    virtual std::tuple<CtrSizeT, IterSharedPtr<AllocationMapChunk>> count_fw() const = 0;
    virtual CtrSizeT level0_pos() const = 0;

    virtual CtrSizeT leaf_size() const = 0;
    virtual const PkdAllocationMap<PkdAllocationMapTypes>* bitmap() const = 0;

    virtual size_t current_bit(size_t level) const = 0;

    virtual AnyID leaf_id() const = 0;
};



template <typename Profile>
class BasicBlockAllocation {
protected:
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;
    CtrSizeT position_;
    CtrSizeT size_;
public:
    BasicBlockAllocation(CtrSizeT position, CtrSizeT size) noexcept :
        position_(position), size_(size) {}

    BasicBlockAllocation() noexcept :
        position_(), size_() {}

    // Position is Level_0-scaled.
    CtrSizeT position() const noexcept {
        return position_;
    }

    CtrSizeT limit() const noexcept {
        return position_+ size_;
    }

    CtrSizeT size1() const noexcept {
        return size_;
    }

    void enlarge1(CtrSizeT amnt) noexcept {
        size_ += amnt;
    }

    bool joinable_with(const BasicBlockAllocation& alc) const noexcept {
        return alc.position_ == position_ + size_;
    }

    void join(const BasicBlockAllocation& alc) noexcept {
        size_ += alc.size_;
    }
};

template <typename Profile>
class AllocationMetadata: public BasicBlockAllocation<Profile> {
protected:
    using Base = BasicBlockAllocation<Profile>;
    using typename Base::CtrSizeT;
    int32_t level_;

    using Base::position_;
    using Base::size_;

public:
    AllocationMetadata() noexcept :
        level_()
    {}

    AllocationMetadata(CtrSizeT position, CtrSizeT size, int32_t level) noexcept :
        Base(position, size), level_(level)
    {}

    AllocationMetadata(const BasicBlockAllocation<Profile>& alloc, int32_t level) noexcept :
        Base(alloc.position(), alloc.size1()), level_(level)
    {}


    int32_t level() const noexcept {
        return level_;
    }

    AllocationMetadata as_level(int32_t level) const noexcept {
        return AllocationMetadata{position_, size_, level};
    }

    CtrSizeT size_at_level() const noexcept {
        return size_ >> level_;
    }

    AllocationMetadata take(int64_t amount) noexcept
    {
        CtrSizeT blocks_l0 = amount << level_;

        AllocationMetadata meta{position_, blocks_l0, level_};
        size_ -= blocks_l0;
        position_ += blocks_l0;
        return meta;
    }


    AllocationMetadata take_for(int64_t amount, int32_t level) noexcept
    {
        CtrSizeT blocks_l0 = amount << level;
        AllocationMetadata meta{position_, blocks_l0, level};
        size_ -= blocks_l0;
        position_ += blocks_l0;
        return meta;
    }

    AllocationMetadata take_all_for(int32_t level) noexcept
    {
        AllocationMetadata meta{position_, size_, level};
        size_ = 0;
        position_ += size_;
        return meta;
    }

    bool operator==(const AllocationMetadata& other) const noexcept {
        return position_ == other.position_;
    }
};


template <typename Profile, int32_t Levels> class AllocationPool;


template <typename Profile>
bool operator<(const AllocationMetadata<Profile>& one, const AllocationMetadata<Profile>& two) noexcept {
    return one.position() < two.position();
}

template <typename Profile>
std::ostream& operator<<(std::ostream& out, const AllocationMetadata<Profile>& meta)
{
    out << "[" << meta.position() << ", " << meta.size1() << ", " << meta.level() << "]";
    return out;
}

enum class AllocationMapEntryStatus: int32_t {
    FREE = 0, ALLOCATED = 1
};

template <typename Profile>
struct AllocationMapCompareHelper {
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    virtual ~AllocationMapCompareHelper() noexcept = default;

    virtual AnyID my_id() const = 0;
    virtual AnyID other_id() const = 0;

    virtual int32_t my_idx() const = 0;
    virtual int32_t other_idx() const = 0;
    virtual int32_t level() const = 0;

    virtual int32_t my_bit() const = 0;
    virtual int32_t other_bit() const = 0;

    virtual CtrSizeT my_base() const = 0;
    virtual CtrSizeT other_base() const = 0;

    virtual void dump_my_leaf() = 0;
    virtual void dump_other_leaf() = 0;
};


template <typename Profile>
struct ICtrApi<AllocationMap, Profile>: public CtrReferenceable<Profile> {
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    using ApiTypes  = ICtrApiTypes<AllocationMap, Profile>;

    using ChunkT = AllocationMapChunk<Profile>;
    using ChunkPtr = IterSharedPtr<ChunkT>;

    using ALCMeta = AllocationMetadata<Profile>;

    static constexpr int32_t  LEVELS          = 9;
    static constexpr CtrSizeT ALLOCATION_SIZE = 512;

    virtual CtrSizeT size() const = 0;

    virtual ChunkPtr seek(CtrSizeT position) const = 0;
    virtual ChunkPtr iterator() const {
        return seek(0);
    }


    virtual CtrSizeT expand(CtrSizeT blocks) MEMORIA_READ_ONLY_API
    virtual void shrink(CtrSizeT size) MEMORIA_READ_ONLY_API

    virtual Optional<AllocationMapEntryStatus> get_allocation_status(int32_t level, CtrSizeT position) const = 0;
    virtual bool check_allocated(const ALCMeta& meta) const = 0;

    virtual CtrSizeT rank(size_t level, CtrSizeT pos) const = 0;

    virtual CtrSizeT find_unallocated(
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<ALCMeta>& buffer
    ) = 0;

    virtual CtrSizeT allocate(
            int32_t level,
            CtrSizeT required,
            ArenaBuffer<ALCMeta>& buffer
    ) MEMORIA_READ_ONLY_API

    using OnLeafListener = std::function<void()>;

    virtual void scan(const std::function<bool (Span<ALCMeta>)>& fn) const = 0;

    virtual void setup_bits(
            Span<ALCMeta> allocations,
            bool set_bits,
            const OnLeafListener& lestener = []{}
    ) MEMORIA_READ_ONLY_API

    virtual void touch_bits(
            Span<ALCMeta> allocations,
            const OnLeafListener& lestener = []{}
    ) MEMORIA_READ_ONLY_API

    virtual CtrSizeT mark_allocated(CtrSizeT pos, int32_t level, CtrSizeT size) MEMORIA_READ_ONLY_API
    virtual CtrSizeT mark_allocated(const ALCMeta& allocation) MEMORIA_READ_ONLY_API
    virtual CtrSizeT mark_unallocated(CtrSizeT pos, int32_t level, CtrSizeT size) MEMORIA_READ_ONLY_API

    virtual CtrSizeT unallocated_at(int32_t level) const = 0;
    virtual void unallocated(Span<CtrSizeT> ranks) const = 0;

    virtual bool populate_allocation_pool(AllocationPool<Profile, LEVELS>&pool, int32_t level) MEMORIA_READ_ONLY_API

    virtual CtrSizeT compare_with(
            CtrSharedPtr<ICtrApi> other,
            const std::function<bool (AllocationMapCompareHelper<Profile>&)>& consumer
    ) const = 0;

    virtual void dump(ChunkDumpMode mode = ChunkDumpMode::LEAF, std::ostream& out = std::cout) const = 0;

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

template <typename Profile>
class hash<memoria::AllocationMetadata<Profile>> {
public:
    size_t operator()(const memoria::AllocationMetadata<Profile>& alc) const noexcept {
        return std::hash<int64_t>()(alc.position());
    }
};

}

namespace fmt {

template <typename Profile>
struct formatter<memoria::AllocationMetadata<Profile>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::AllocationMetadata<Profile>& alc, FormatContext& ctx) {
        return format_to(ctx.out(), "[{}, {}, {}, {}]", alc.position(), alc.size1(), alc.level(), alc.size_at_level());
    }
};

template <typename Profile>
struct formatter<memoria::BasicBlockAllocation<Profile>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::BasicBlockAllocation<Profile>& alc, FormatContext& ctx) {
        return format_to(ctx.out(), "[{}, {}]", alc.position(), alc.size1());
    }
};

}
