
// Copyright 2019-2023 Victor Smirnov
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
#include <memoria/core/reflection/typehash.hpp>


#include <memoria/api/collection/collection_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api_factory.hpp>

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
    constexpr BasicBlockAllocation(CtrSizeT position, CtrSizeT size) noexcept :
        position_(position), size_(size) {}

    constexpr BasicBlockAllocation() noexcept :
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
class AllocationMetadata {
public:
    using SizeT = uint64_t; // 64 bits is enough for everyone (not)
    static constexpr SizeT LEVELS_MAX      = 16;
    static constexpr SizeT LEVELS_BITS     = 4;
    static constexpr SizeT LEVELS_MASK     = LEVELS_BITS - 1;
    static constexpr SizeT POSITION_OFFSET = 12;
    static constexpr SizeT POSITION_MAX    = 1ull << (64 - POSITION_OFFSET);
    static constexpr SizeT POSITION_MASK   = ~((1ull << POSITION_OFFSET) - 1);


    static constexpr SizeT ALC_SIZE_OFFSET = LEVELS_BITS;
    static constexpr SizeT ALC_SIZE_BITS   = POSITION_OFFSET - LEVELS_BITS;
    static constexpr SizeT ALC_SIZE_MAX    = 1ull << ALC_SIZE_BITS;
    static constexpr SizeT ALC_SIZE_MASK   = ALC_SIZE_BITS - 1;

protected:
    SizeT data_;

    constexpr AllocationMetadata(SizeT position, SizeT size, size_t level):
        data_((position << POSITION_OFFSET) | (size << ALC_SIZE_OFFSET) | level)
    {
        if (MMA_UNLIKELY(level >= LEVELS_MAX)) {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Allocation level exceeds maximum of {}: {}",
                        LEVELS_MAX,
                        level
            ).do_throw();
        }


        if (MMA_UNLIKELY(size >= ALC_SIZE_MAX)) {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Allocation size exceeds maximum of {}: {}",
                        ALC_SIZE_MAX, size
            ).do_throw();
        }

        if (MMA_UNLIKELY(position >= POSITION_MAX)) {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Allocation position exceeds maximum of {}: {}",
                        POSITION_MAX, position
            ).do_throw();
        }
    }

    constexpr AllocationMetadata(SizeT data):
        data_(data)
    {}


public:
    constexpr AllocationMetadata() noexcept :
        data_()
    {}

    constexpr AllocationMetadata(BasicBlockAllocation<Profile> alc, SizeT level):
        AllocationMetadata(alc.position(), alc.size1() >> level, level)
    {
    }

    constexpr operator BasicBlockAllocation<Profile>() const {
        return BasicBlockAllocation<Profile>(position(), size_n() << level());
    }

    static constexpr AllocationMetadata from_l0(SizeT position, SizeT size, SizeT level) {
        return AllocationMetadata(position, size >> level, level);
    }

    static constexpr AllocationMetadata from_ln(SizeT position, SizeT size, SizeT level) {
        return AllocationMetadata(position, size, level);
    }

    static constexpr AllocationMetadata from_raw(SizeT data) {
        return AllocationMetadata(data);
    }

    constexpr SizeT raw_data() const noexcept {
        return data_;
    }

    bool fits(SizeT size) const noexcept
    {
        SizeT current = size_n();
        return (current + size) < ALC_SIZE_MAX;
    }

    // Position is Level_0-scaled.
    SizeT position() const noexcept {
        return data_ >> POSITION_OFFSET;
    }

    SizeT limit() const noexcept {
        return position() + size1();
    }

    SizeT size_n() const noexcept {
        return (data_ >> ALC_SIZE_OFFSET) & ALC_SIZE_MASK;
    }

    void set_size_n(SizeT size)
    {
        if (MMA_LIKELY(size < ALC_SIZE_MAX))
        {
            size <<= ALC_SIZE_OFFSET;
            data_ &= ~(ALC_SIZE_MASK << ALC_SIZE_OFFSET);
            data_ |= size;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Allocation size exceeds maximum of {}: {}",
                        ALC_SIZE_MAX, size
            ).do_throw();
        }
    }

    void set_position(SizeT position)
    {
        if (MMA_LIKELY(position < POSITION_MAX)) {
            data_ &= ~POSITION_MASK;
            data_ |= position << POSITION_OFFSET;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Allocation position exceeds maximum of {}: {}",
                        POSITION_MAX, position
            ).do_throw();
        }
    }

    SizeT size1() const noexcept {
        return size_n() << level();
    }

    SizeT level() const noexcept {
        return data_ & LEVELS_MASK;
    }

    void enlarge1(SizeT amnt)
    {
        SizeT current = size_n();
        current += amnt;
        set_size_n(current);
    }

    void shrink(SizeT amnt)
    {
        SizeT current = size_n();
        if (MMA_LIKELY(amnt <= current)) {
            current -= amnt;
            set_size_n(current);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Invalid allocation size amount of {}: current is {}",
                        amnt, current
            ).do_throw();
        }
    }

    bool joinable_with(const AllocationMetadata& alc) const noexcept {
        return alc.position() == position() + size1();
    }

    void join(const AllocationMetadata& alc)
    {
        if (MMA_LIKELY(alc.level() == level())) {
            enlarge1(alc.size_at_level());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Invalid allocation level for join {}: current level is {}",
                        alc.level(), level()
            ).do_throw();
        }
    }

    AllocationMetadata as_level(SizeT level) const noexcept {
        return AllocationMetadata{data_};
    }

    SizeT size_at_level() const noexcept {
        return size_n();
    }

    AllocationMetadata take(SizeT amount)
    {
        SizeT lvl = level();
        SizeT pos = position();

        AllocationMetadata meta{pos, amount, lvl};
        shrink(lvl);

        SizeT blocks_l0 = amount << lvl;
        set_position(pos + blocks_l0);

        return meta;
    }

    AllocationMetadata take_for(SizeT amount, SizeT for_level)
    {
        SizeT my_level = level();

        if (for_level <= my_level)
        {
            SizeT blocks_l0 = amount << for_level;
            SizeT pos = position();

            SizeT tgt_size = amount << (my_level - for_level);
            AllocationMetadata meta{pos, tgt_size, for_level};

            shrink(amount);
            set_position(pos + blocks_l0);

            return meta;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Invalid allocation level for take_for {}: current level is {}",
                        my_level, for_level
            ).do_throw();
        }
    }

    AllocationMetadata take_all_for(SizeT level) noexcept
    {
        return take_for(size_n(), level);
    }

    bool operator==(const AllocationMetadata& other) const noexcept {
        return position() == other.position();
    }
};


template <typename Profile, size_t Levels> class AllocationPool;


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


/**
 * Note that in the context of persistent (CoW) data structures (PDS), a
 * plain bitmap can be considered as a degenerate case of a block
 * reference counters map.
 *
 * Block reference counters for PDS are naturally compressible and
 * may have an arbitrary 'width'. When a counter is incremented, if
 * its value reaches the limit, we just clone the block for that
 * counter and use it as a pointee. Depending on the counter's
 * 'width' (number of bits) we will have some amount of memory
 * overhead.
 *
 * For a bitmap, each bit can be considered as a 1-bit counter. And
 * for this specific and degenerate case, memory overhead will be
 * 100%, because blocks will be cloned all the time we try to
 * increment the counter (each block mah have only up to
 * one reference to it).
 *
 * For 2-bit counter, overhead will be 30% on average. Worst
 * case is 100% but, probabilistically, it's unlikely to happen.
 * For 3-bit counter space overhead will be about 1/7 of the
 * storage. And so on...
 *
 * For a pretty machine-friendly 8-bit counter, space overhead
 * will be less than 0.5%. It's exponentially decaying with the
 * number of bits in counters. Traditionally, counters are of
 * 32 or 64 bit width, so reaching the precision limit is highly
 * unlikely anyway (conditions for that are pretty hard to be
 * observed in all practical cases of PDS usages).
 *
 * Mathematics behind this property of block reference counters
 * is eaxctly the same as the mathematics behind basic properties
 * of a Copy-on-Write trees. Without persistent tree, counters,
 * representing number of version the block is reachable from,
 * will have pretty high values. With persistent tree, this value
 * is spread between different nodes' counters in the tree, so
 * on average, it's rather small.
 *
 * So, counters can be easily represented as a CoW-tree and
 * they can be versioned and transactional the same way
 * AllocationMap in the SWMRStore is transactional. Both ZFS
 * and BTRFS use variants of reference counting trees to manage
 * block's lifetime. But some implementations may consider
 * using traditional counters stored in a fast hash map
 * just for performance reasons: there may be hundreds of
 * counter increments _per each tree node cloning_ (if btree
 * fanout is high, that is especially the case for 8K blocks
 * and larger). And btrees are inherently slower than
 * hash maps.
 *
 * SWMRStore variants are currently using fast open-addressing hash
 * map for the reason that AllocationMap (that is a btree)
 * is not yet optimized enough to host block counters
 * in itself. But future revisions of AllocationMap will probably be, so
 * the will be at least an option to use transactional
 * block reference counters and have true instant crash
 * recovery and low main memory footprint (no need to
 * load/store entire set of couners to/from RAM each time
 * a store is opened/closed).
 *
 *
 */


template <typename Profile>
struct ICtrApi<AllocationMap, Profile>: public CtrReferenceable<Profile> {
    using CtrSizeT = ApiProfileCtrSizeT<Profile>;

    using ApiTypes  = ICtrApiTypes<AllocationMap, Profile>;

    using ChunkT   = AllocationMapChunk<Profile>;
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

    virtual bool populate_allocation_pool(AllocationPool<Profile, LEVELS>& pool, int32_t level) MEMORIA_READ_ONLY_API
    virtual void drain_allocation_pool(AllocationPool<Profile, LEVELS>& pool, size_t level) MEMORIA_READ_ONLY_API

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
