
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/core/packed/sseq/packed_allocation_map_so.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include <ostream>

namespace memoria {

template <typename Types>
class PkdAllocationMap: public PackedAllocator {

    using Base = PackedAllocator;

public:
    static const uint32_t VERSION = 1;

    using MyType = PkdAllocationMap;

    using Allocator = PackedAllocator;

    using ExtData = std::tuple<>;
    using SparseObject = PackedAllocationMapSO<ExtData, MyType>;

    using BitmapType = uint64_t;
    using IndexType  = uint16_t;

    static constexpr size_t ValuesPerBranch        = 512;
    static constexpr size_t Indexes                = 9;

    enum {
        METADATA = 0, INDEX = 1, SYMBOLS = INDEX + Indexes
    };

    class Metadata {
        psize_t size_;
        psize_t capacity_;
    public:
        psize_t& size()                 {return size_;}
        const psize_t& size() const     {return size_;}

        psize_t& capacity()                 {return capacity_;}
        const psize_t& capacity() const     {return capacity_;}
    };


public:
    using Base::block_size;
    using Base::allocate;
    using Base::allocate_array_by_size;

    PkdAllocationMap() noexcept = default;

    size_t size() const noexcept {return metadata()->size();}
    void set_size(size_t size) noexcept {
        metadata()->size() = size;
    }

    size_t capacity() const noexcept {
        return metadata()->capacity();
    }

    size_t available_space() const noexcept {
        auto meta = this->metadata();
        return meta->capacity() - meta->size();
    }

    size_t size(size_t level) const noexcept {
        return bitmap_level_size(size(), level);
    }

    // ====================================== Accessors ================================= //

    Metadata* metadata() noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const noexcept {
        return Base::template get<Metadata>(METADATA);
    }

    IndexType* index(size_t idx_num) noexcept
    {
        return Base::template get<IndexType>(INDEX + idx_num);
    }

    const IndexType* index(size_t idx_num) const noexcept
    {
        return Base::template get<IndexType>(INDEX + idx_num);
    }

    BitmapType* symbols(size_t blk_num) noexcept
    {
        return Base::template get<BitmapType>(SYMBOLS + blk_num);
    }

    const BitmapType* symbols(size_t blk_num) const noexcept
    {
        return Base::template get<BitmapType>(SYMBOLS + blk_num);
    }







public:

    // ===================================== Allocation ================================= //

    void init_default(size_t provided_block_size) noexcept {
        size_t bitmap_size = find_max_bitmap_size(provided_block_size);
        return init_by_size(bitmap_size);
    }

    void init_by_size(size_t bitmap_size) noexcept
    {
        size_t bitmap_blk_size = block_size(bitmap_size);
        Base::init(bitmap_blk_size, 1 + Indexes * 2);

        auto meta = allocate<Metadata>(METADATA);

        size_t single_bitmap_size = bitmap_size;
        for (size_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            size_t index_size = index_level_size(single_bitmap_size);
            allocate_array_by_size<IndexType>(INDEX + c, index_size);
        }

        single_bitmap_size = bitmap_size;
        for (size_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            size_t bitmap_uints_size = div_up(single_bitmap_size, static_cast<size_t>(sizeof(BitmapType) * 8));
            allocate_array_by_size<BitmapType>(SYMBOLS + c, bitmap_uints_size);
        }

        meta->capacity() = bitmap_size;
        meta->size()     = 0;

        return reindex();
    }


    size_t block_size(const MyType* other) const noexcept
    {
        return MyType::block_size(capacity() + other->capacity());
    }

    static size_t block_size(size_t bitmap_size) noexcept
    {
        size_t metadata_length = PackedAllocatable::round_up_bytes(sizeof(Metadata));

        size_t bitmaps_length = 0;
        size_t indexes_length = 0;

        size_t single_bitmap_size = bitmap_size;
        for (size_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            size_t bitmap_length = PackedAllocatable::round_up_bytes(
                    div_up(single_bitmap_size, static_cast<size_t>(sizeof(BitmapType) * 8)) * 8
            );
            bitmaps_length += bitmap_length;

            size_t index_size   = index_level_size(single_bitmap_size);
            size_t index_length = PackedAllocatable::round_up_bytes(index_size * sizeof(IndexType));
            indexes_length += index_length;
        }

        return Base::block_size(metadata_length + indexes_length + bitmaps_length, 1 + Indexes * 2);
    }

    static size_t default_size(size_t available_space) noexcept
    {
        return find_block_size(available_space);
    }

    static size_t find_block_size(size_t client_area) noexcept
    {
        size_t max_blk_size = 0;

        for (size_t bitmap_size = 0; bitmap_size < client_area * 8; bitmap_size += ValuesPerBranch)
        {
            size_t blk_size = block_size(bitmap_size);
            if (blk_size <= client_area) {
                max_blk_size = blk_size;
            }
            else {
                break;
            }
        }

        return max_blk_size;
    }

    static size_t find_max_bitmap_size(size_t client_area) noexcept
    {
        size_t max_blk_size = 0;
        size_t last_bitmap_size{};

        for (size_t bitmap_size = 0; bitmap_size < client_area * 8; bitmap_size += ValuesPerBranch)
        {
            size_t blk_size = block_size(bitmap_size);
            if (blk_size <= client_area) {
                max_blk_size = blk_size;
                last_bitmap_size = bitmap_size;
            }
            else {
                break;
            }
        }

        return last_bitmap_size;
    }


public:

    void enlarge(size_t size) noexcept
    {
        if (size % ValuesPerBranch)
        {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Size argument {} must me multiple of {}", size, ValuesPerBranch
            ).do_throw();
        }

        auto meta = this->metadata();

        size_t capacity = meta->capacity();
        if (size + meta->size() > capacity) {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Requested size {} is too large, maximum is {}", size, capacity - meta->size()
            ).do_throw();
        }

        BitmapType* bitmap = this->symbols(0);
        FillZero(bitmap, meta->size(), meta->size() + size);

        meta->size() += size;

        return reindex(true);
    }

    void check() const
    {
        for (size_t ll = 0; ll < Indexes - 1; ll++)
        {
            size_t ll_size = size(ll);
            for (size_t ii = 0; ii < ll_size; ii += 2)
            {
                size_t b0 = get_bit(ll, ii);
                size_t b1 = get_bit(ll, ii + 1);

                size_t b_up_expect = b0 || b1;
                size_t b_up_actual = get_bit(ll + 1, ii / 2);

                if (b_up_expect != b_up_actual) {
                    MEMORIA_MAKE_GENERIC_ERROR("Bitmap leyering mismatch: level: {}, idx: {}, bits: {} {} {}", ll, ii, b0, b1, b_up_actual).do_throw();
                }
            }
        }

        for (size_t ll = 0; ll < Indexes; ll++)
        {
            check_level_index(ll);
        }
    }

    void reindex(bool recompute_bitmaps = false) noexcept
    {
        size_t bitmap_size = this->size();

        size_t local_bitmap_size = bitmap_size;

        if (recompute_bitmaps)
        {
            local_bitmap_size = bitmap_size;
            for (size_t c = 0; c < Indexes - 1; c++, local_bitmap_size /= 2)
            {
                const BitmapType* src_bitmap = this->symbols(c);
                BitmapType* tgt_bitmap = this->symbols(c + 1);

                size_t atom_bits_size = sizeof(BitmapType) * 8;
                size_t bitmap_atoms_size = local_bitmap_size / atom_bits_size;

                for (size_t bms = 0; bms < bitmap_atoms_size; bms += 2)
                {
                    BitmapType a1 = src_bitmap[bms];
                    BitmapType a2 = src_bitmap[bms + 1];
                    BitmapType rs = a1 | (a2 << atom_bits_size / 2);
                    tgt_bitmap[bms / 2] = rs;
                }
            }
        }

        for (size_t c = 0; c < Indexes; c++) //, local_bitmap_size /= 2
        {
            reindex_level(c);
        }
    }

    void reindex_level(size_t level) noexcept
    {
        size_t local_bitmap_size = this->size() >> level;

        size_t index_size = index_level_size(local_bitmap_size);
        if (index_size > 0)
        {
            IndexType* index = this->index(level);
            const BitmapType* bitmap = symbols(level);

            for (size_t bi = 0, icnt = 0; bi < local_bitmap_size; bi += ValuesPerBranch, icnt++)
            {
                size_t limit = (bi + ValuesPerBranch <= local_bitmap_size) ? (bi + ValuesPerBranch) : local_bitmap_size;
                size_t span_size = static_cast<size_t>(limit - bi);

                index[icnt] = static_cast<IndexType>(span_size - PopCount(bitmap, bi, limit));
            }
        }
    }

    void check_level_index(size_t level) const
    {
        size_t local_bitmap_size = this->size() >> level;

        size_t index_size = index_level_size(local_bitmap_size);
        if (index_size > 0)
        {
            const IndexType* index = this->index(level);
            const BitmapType* bitmap = symbols(level);

            for (size_t bi = 0, icnt = 0; bi < local_bitmap_size; bi += ValuesPerBranch, icnt++)
            {
                size_t limit = (bi + ValuesPerBranch <= local_bitmap_size) ? (bi + ValuesPerBranch) : local_bitmap_size;
                size_t span_size = static_cast<size_t>(limit - bi);

                size_t expected = static_cast<IndexType>(span_size - PopCount(bitmap, bi, limit));
                size_t actual = index[icnt];

                if (expected != actual) {
                    MEMORIA_MAKE_GENERIC_ERROR("Invalid bitmap index: level: {}, expected: {}, actual: {}, [{}, {})", level, expected, actual, bi, limit).do_throw();
                }
            }
        }
    }

    void set_bits(size_t level, size_t idx, size_t size) noexcept
    {
        set_bits_down(level, idx, size);

        size_t start = idx;
        size_t stop  = idx + size;

        for (size_t ll = level + 1; ll < Indexes; ll++)
        {
            start = start / 2;
            stop  = div_up(stop, static_cast<size_t>(2));

            BitmapType* bitmap = this->symbols(ll);
            FillOne(bitmap, start, stop);
        }
    }

    void set_bits_down(size_t level, size_t idx, size_t size) noexcept
    {
        size_t start = idx;
        size_t stop  = idx + size;

        size_t local_bitmap_size = this->size() >> level;

        if (!(idx >= 0 && idx <= local_bitmap_size && (idx + size <= local_bitmap_size))) {
            MEMORIA_MAKE_GENERIC_ERROR("PackedAllocationMap range ckeck error: level: {}, idx: {}, size: {}, limit: {}", level, idx, size, local_bitmap_size).do_throw();
        }

        for (size_t ll = level + 1; ll > 0; ll--)
        {
            BitmapType* bitmap = this->symbols(ll - 1);
            FillOne(bitmap, start, stop);

            start *= 2;
            stop  *= 2;
        }
    }

    size_t get_bit(size_t level, size_t idx) const noexcept
    {
        const BitmapType* bitmap = this->symbols(level);
        return GetBit(bitmap, idx);
    }


    void clear_bits(size_t level, size_t idx, size_t size) noexcept
    {
        size_t start = idx;
        size_t stop  = idx + size;

        size_t local_bitmap_size = this->size() >> level;

        if (!(idx >= 0 && idx <= local_bitmap_size && (idx + size <= local_bitmap_size))) {
            MEMORIA_MAKE_GENERIC_ERROR("PackedAllocationMap range ckeck error: level: {}, idx: {}, size: {}, limit: {}", level, idx, size, local_bitmap_size).do_throw();
        }

        for (size_t ll = level + 1; ll > 0; ll--)
        {
            BitmapType* bitmap = this->symbols(ll - 1);
            FillZero(bitmap, start, stop);

            start *= 2;
            stop  *= 2;
        }

        rebuild_bitmaps(level);
        return reindex(false);
    }

    void clear_bits_opt(size_t level, size_t idx, size_t size) noexcept
    {
        size_t start = idx;
        size_t stop  = idx + size;

        size_t local_bitmap_size = this->size() >> level;

        if (!(idx >= 0 && idx <= local_bitmap_size && (idx + size <= local_bitmap_size))) {
            MEMORIA_MAKE_GENERIC_ERROR("PackedAllocationMap range ckeck error: level: {}, idx: {}, size: {}, limit: {}", level, idx, size, local_bitmap_size).do_throw();
        }

        for (size_t ll = level + 1; ll > 0; ll--)
        {
            BitmapType* bitmap = this->symbols(ll - 1);
            FillZero(bitmap, start, stop);

            start *= 2;
            stop  *= 2;
        }
    }

    size_t sum(size_t level) const noexcept
    {
        size_t value = 0;

        size_t bitmap_size = bitmap_level_size(size(), level);
        size_t index_size = this->index_level_size(bitmap_size);

        if (MMA_LIKELY(index_size > 0))
        {
            const IndexType* idxs = this->index(level);
            for (size_t c = 0; c < index_size; c++) {
                value += idxs[c];
            }
        }
        else {
            const BitmapType* bitmap = symbols(level);
            value = bitmap_size - PopCount(bitmap, 0, bitmap_size);
        }

        return static_cast<size_t>(value);
    }


    struct SelectResult {
        size_t idx;
        size_t size;
        size_t rank;

        bool is_found() const {return idx < size;}
    };


    SelectResult select_fw(size_t rank_base0, size_t level) const noexcept
    {
        auto rank = rank_base0 + 1;

        size_t bm_size  = bitmap_level_size(size(), level);
        size_t idx_size = index_level_size(bm_size);

        size_t bm_start = 0;
        size_t sum{};

        size_t rank_tmp = rank;

        if (idx_size > 0)
        {
            const IndexType* idxs = this->index(level);

            for (size_t c = 0; c < idx_size; c++, bm_start += ValuesPerBranch)
            {
                sum += idxs[c];
                if (sum >= rank) {                    
                    break;
                }
                else {
                    rank_tmp -= idxs[c];
                }
            }
        }

        const BitmapType* bitmap = this->symbols(level);

        auto result = Select0FW(bitmap, bm_start, bm_size, rank_tmp);

        return SelectResult{
            static_cast<size_t>(result.idx) << level,
            bm_size << level,
            static_cast<size_t>(result.rank) + sum
        };
    }

    size_t rank(size_t pos, size_t level) const noexcept
    {
        size_t block_start = (pos / ValuesPerBranch) * ValuesPerBranch;

        const BitmapType* bitmap = symbols(level);

        size_t ones     = PopCount(bitmap, block_start, pos);
        size_t zeroes  = (pos - block_start) - static_cast<size_t>(ones);

        if (MMA_LIKELY(block_start > 0))
        {
            const IndexType* idxs = index(level);
            for (size_t c  = 0; c < pos / ValuesPerBranch; c++)
            {
                zeroes += idxs[c];
            }
        }

        return zeroes;
    }


    SelectResult select_fw(size_t start, size_t rank, size_t level) const noexcept
    {
        size_t startrank_ = this->rank(start, level);

        auto result = select_fw(startrank_ + rank, level);
        result.rank -= startrank_;

        return result;
    }


    class CountResult {
        size_t count_;
        size_t symbol_;
    public:
        CountResult(size_t count, size_t symbol) noexcept:
            count_(count), symbol_(symbol)
        {}

        size_t count() const noexcept {return count_;}
        size_t symbol() const noexcept {return symbol_;}
    };

    CountResult count_fw(size_t start, size_t level) const noexcept
    {
        size_t bm_size     = bitmap_level_size(size(), level);
        size_t block_end   = (start / ValuesPerBranch + 1) * ValuesPerBranch;
        size_t block_limit = block_end > bm_size ? bm_size : block_end;

        const BitmapType* bitmap = symbols(level);
        size_t zeroes = CountTrailingZeroes(bitmap, start, block_limit);

        if (zeroes < block_limit - start) {
            return CountResult{zeroes, level};
        }
        else {
            size_t index_size = index_level_size(bm_size);
            const IndexType* idxs = index(level);

            size_t index_start = start / ValuesPerBranch + 1;
            size_t sum{};

            for (size_t ii = index_start; ii < index_size; ii++)
            {
                size_t index_block_start = ii * ValuesPerBranch;
                size_t index_block_end = (ii + 1) * ValuesPerBranch;
                size_t index_block_limit = index_block_end > bm_size ? bm_size : index_block_end;
                size_t index_block_size  = index_block_limit - index_block_start;

                size_t cell_value = idxs[ii];

                if (cell_value == index_block_size) {
                    sum += cell_value;
                }
                else {
                    size_t blk_cnt = CountTrailingZeroes(bitmap, index_block_start, index_block_limit);
                    sum += blk_cnt;
                    break;
                }
            }

            return CountResult{zeroes + sum, level};
        }
    }

    static size_t initial_size(size_t available_space) noexcept
    {
        return find_block_size(available_space);
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_ALLOCATION_MAP");

        size_t bm_size = this->size();
        handler->value("SIZE", &bm_size);

        handler->startGroup("BITMAPS", bm_size);

        for (size_t ll = 0; ll < Indexes; ll++)
        {
            size_t bm_level_size  = bitmap_level_size(bm_size, ll);
            size_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                handler->value("INDEX", index(ll), idx_level_size);
            }

            handler->symbols("SYMBOLS", symbols(ll), bm_level_size, 1);
        }

        handler->endGroup();

        handler->endGroup();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        psize_t bm_size = meta->size();
        FieldFactory<psize_t>::serialize(buf, bm_size);

        for (size_t ll = 0; ll < Indexes; ll++)
        {
            size_t bm_level_size  = bitmap_level_size(bm_size, ll);
            size_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                FieldFactory<IndexType>::serialize(buf, index(ll), idx_level_size);
            }

            size_t bm_atoms = div_up(bm_level_size, static_cast<size_t>(sizeof(BitmapType) * 8));
            FieldFactory<BitmapType>::serialize(buf, symbols(ll), bm_atoms);
        }
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<psize_t>::deserialize(buf, meta->size());
        size_t bm_size = meta->size();

        for (size_t ll = 0; ll < Indexes; ll++)
        {
            size_t bm_level_size  = bitmap_level_size(bm_size, ll);
            size_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                FieldFactory<IndexType>::deserialize(buf, index(ll), idx_level_size);
            }

            size_t bm_atoms = div_up(bm_level_size, static_cast<size_t>(sizeof(BitmapType) * 8));
            FieldFactory<BitmapType>::deserialize(buf, symbols(ll), bm_atoms);
        }
    }


    void scan_unallocated(size_t level, const std::function<bool (size_t, size_t)>& fn) const noexcept
    {
        size_t idx = 0;
        while (true)
        {
            SelectResult sel_res = select_fw(idx, 0, level);
            if (sel_res.is_found())
            {
                size_t l_pos = sel_res.idx >> level;
                CountResult cnt_res = count_fw(l_pos, level);

                auto cont = fn(l_pos, cnt_res.count());

                if (cont) {
                    idx += cnt_res.count();
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
    }


    template <typename AllocationPool>
    bool populate_allocation_pool(size_t base, AllocationPool& pool, size_t level) noexcept
    {
        bool updated = false;
        for (size_t ll = Indexes; ll > level; ll--)
        {
            size_t level = ll - 1;

            scan_unallocated(level, [&](size_t pos, size_t size) {
                if (pool.add(base + (pos << level), size << level, level)) {
                    updated = true;
                    set_bits(level, pos, size);
                    return true;
                }
                else {
                    return false;
                }
            });

            reindex_level(level);
            if (level > 0) {
                reindex_level(level - 1);
            }
        }

        if (updated) {
            reindex(false);
        }

        return updated;
    }

    template <typename Fn>
    bool compare_with(const PkdAllocationMap* other, size_t my_start, size_t other_start, size_t size, Fn&& fn) const noexcept
    {
        for (size_t ll = 0; ll < Indexes; ll++)
        {
            size_t ll_size = size >> ll;
            size_t my_ll_start = my_start >> ll;
            size_t other_ll_start = other_start >> ll;

            for (size_t ii = 0; ii < ll_size ; ii++)
            {
                size_t my_bit = this->get_bit(ll, ii + my_ll_start);
                size_t other_bit = other->get_bit(ll, ii + other_ll_start);

                if (my_bit != other_bit)
                {
                    auto do_continue = fn(ii + my_ll_start, ii + other_ll_start, ll, my_bit, other_bit);
                    if (!do_continue) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

private:

    constexpr static size_t index_level_size(size_t bitmap_size) noexcept {
        return bitmap_size > ValuesPerBranch ? div_up(bitmap_size, ValuesPerBranch) : 0;
    }

    constexpr static size_t bitmap_level_size(size_t l0_bitmap_size, size_t level) noexcept {
        return l0_bitmap_size >> level;
    }

    constexpr static BitmapType gather_bits(BitmapType bits) noexcept
    {
        BitmapType src = (bits | bits >> 1);
        BitmapType tgt{};

        size_t atom_size = sizeof(BitmapType) * 4;

        constexpr BitmapType ONE = 0x1;

        for (size_t c = 0; c < atom_size; c++)
        {
            BitmapType mask = ONE << (c * 2);
            BitmapType value = src & mask;
            value >>= c;
            tgt |= value;
        }

        return tgt;
    }

public:
    void rebuild_bitmaps(size_t level_from) noexcept
    {
        size_t bitmap_size = this->size();
        size_t local_bitmap_size = bitmap_level_size(bitmap_size, level_from);
        for (size_t c = level_from; c < Indexes - 1; c++, local_bitmap_size /= 2)
        {
            const BitmapType* src_bitmap = this->symbols(c);
            BitmapType* tgt_bitmap = this->symbols(c + 1);

            size_t atom_bits_size = sizeof(BitmapType) * 8;
            size_t bitmap_atoms_size = local_bitmap_size / atom_bits_size;

            for (size_t bms = 0; bms < bitmap_atoms_size; bms += 2)
            {
                BitmapType a1 = src_bitmap[bms];
                BitmapType a2 = src_bitmap[bms + 1];

                //BitmapType b1 = a1 | (a1 >> 1);
                //BitmapType b2 = a2 | (a2 >> 1);

                //BitmapType rs = b1 | (b2 << atom_bits_size / 2);
                tgt_bitmap[bms / 2] = gather_bits(a1) | (gather_bits(a2) << atom_bits_size / 2);
            }
        }
    }
};


template <typename Types>
struct PackedStructTraits<PkdAllocationMap<Types>> {
    using SearchKeyDataType = BigInt;

    using AccumType = DTTViewType<SearchKeyDataType>;
    using SearchKeyType = DTTViewType<SearchKeyDataType>;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;
    static constexpr size_t Indexes = PkdAllocationMap<Types>::Indexes;
};



}
