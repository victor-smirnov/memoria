
// Copyright 2020 Victor Smirnov
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
#include <memoria/core/packed/sseq/packed_allocation_map_view.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>

#include <ostream>

namespace memoria {

struct PkdAllocationMapTypes {};

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

    using GrowableIOSubstream = PackedAllocationMapView<MyType>;
    using IOSubstreamView = PackedAllocationMapView<MyType>;

    static constexpr int32_t ValuesPerBranch        = 512;
    static constexpr int32_t Indexes                = 9;


    enum {
        METADATA = 0, INDEX = 1, SYMBOLS = INDEX + Indexes
    };

    class Metadata {
        int32_t size_;
        int32_t capacity_;
    public:
        int32_t& size()                 {return size_;}
        const int32_t& size() const     {return size_;}

        int32_t& capacity()                 {return capacity_;}
        const int32_t& capacity() const     {return capacity_;}
    };


public:
    using Base::block_size;
    using Base::allocate;
    using Base::allocateArrayBySize;

    PkdAllocationMap() noexcept = default;

    int32_t& size() noexcept {return metadata()->size();}
    const int32_t& size() const noexcept {return metadata()->size();}

    int32_t& capacity() noexcept {return metadata()->capacity();}
    const int32_t& capacity() const noexcept {return metadata()->capacity();}

    int32_t available_space() const noexcept {
        auto meta = this->metadata();
        return meta->capacity() - meta->size();
    }

    int32_t size(int32_t level) const noexcept
    {
        return bitmap_level_size(size(), level);
    }

    static int do_nothing() {
        return 0;
    }

    // ====================================== Accessors ================================= //

    Metadata* metadata() noexcept
    {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const noexcept
    {
        return Base::template get<Metadata>(METADATA);
    }

    IndexType* index(int32_t idx_num) noexcept
    {
        return Base::template get<IndexType>(INDEX + idx_num);
    }

    const IndexType* index(int32_t idx_num) const noexcept
    {
        return Base::template get<IndexType>(INDEX + idx_num);
    }

    BitmapType* symbols(int32_t blk_num) noexcept
    {
        return Base::template get<BitmapType>(SYMBOLS + blk_num);
    }

    const BitmapType* symbols(int32_t blk_num) const noexcept
    {
        return Base::template get<BitmapType>(SYMBOLS + blk_num);
    }







public:

    // ===================================== Allocation ================================= //

    VoidResult init_default(int32_t provided_block_size) noexcept {
        int32_t bitmap_size = find_max_bitmap_size(provided_block_size);
        return init_by_size(bitmap_size);
    }

    VoidResult init_by_size(int32_t bitmap_size) noexcept
    {
        int32_t bitmap_blk_size = block_size(bitmap_size);
        MEMORIA_TRY_VOID(Base::init(bitmap_blk_size, 1 + Indexes * 2));

        MEMORIA_TRY(meta, allocate<Metadata>(METADATA));

        int32_t single_bitmap_size = bitmap_size;
        for (int32_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            int32_t index_size = index_level_size(single_bitmap_size);
            MEMORIA_TRY_VOID(allocateArrayBySize<IndexType>(INDEX + c, index_size));
        }

        single_bitmap_size = bitmap_size;
        for (int32_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            int32_t bitmap_uints_size = divUp(single_bitmap_size, static_cast<int32_t>(sizeof(BitmapType) * 8));
            MEMORIA_TRY_VOID(allocateArrayBySize<BitmapType>(SYMBOLS + c, bitmap_uints_size));
        }

        meta->capacity() = bitmap_size;
        meta->size()     = 0;

        return reindex();
    }


    int32_t block_size(const MyType* other) const noexcept
    {
        return MyType::block_size(capacity() + other->capacity());
    }

    static int32_t block_size(int32_t bitmap_size) noexcept
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t bitmaps_length = 0;
        int32_t indexes_length = 0;

        int32_t single_bitmap_size = bitmap_size;
        for (int32_t c = 0; c < Indexes; c++, single_bitmap_size /= 2)
        {
            int32_t bitmap_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    divUp(single_bitmap_size, static_cast<int32_t>(sizeof(BitmapType) * 8)) * 8
            );
            bitmaps_length += bitmap_length;

            int32_t index_size   = index_level_size(single_bitmap_size);
            int32_t index_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexType));
            indexes_length += index_length;
        }

        return Base::block_size(metadata_length + indexes_length + bitmaps_length, 1 + Indexes * 2);
    }

    static int32_t default_size(int32_t available_space) noexcept
    {
        return find_block_size(available_space);
    }

    static int32_t find_block_size(int32_t client_area) noexcept
    {
        int32_t max_blk_size = 0;

        for (int32_t bitmap_size = 0; bitmap_size < client_area * 8; bitmap_size += ValuesPerBranch)
        {
            int32_t blk_size = block_size(bitmap_size);
            if (blk_size <= client_area) {
                max_blk_size = blk_size;
            }
            else {
                break;
            }
        }

        return max_blk_size;
    }

    static int32_t find_max_bitmap_size(int32_t client_area) noexcept
    {
        int32_t max_blk_size = 0;
        int32_t last_bitmap_size;

        for (int32_t bitmap_size = 0; bitmap_size < client_area * 8; bitmap_size += ValuesPerBranch)
        {
            int32_t blk_size = block_size(bitmap_size);
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

    VoidResult enlarge(int32_t size) noexcept
    {
        if (size % ValuesPerBranch)
        {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "Size argument {} must me multiple of {}", size, ValuesPerBranch
            );
        }

        auto meta = this->metadata();

        int32_t capacity = meta->capacity();
        if (size + meta->size() > capacity) {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "Requested size {} is too large, maximum is {}", size, capacity - meta->size()
            );
        }

        BitmapType* bitmap = this->symbols(0);
        FillZero(bitmap, meta->size(), meta->size() + size);

        meta->size() += size;

        return reindex(true);
    }

    VoidResult check() const noexcept {
        return VoidResult::of();
    }

    VoidResult reindex(bool recompute_bitmaps = false) noexcept
    {
        int32_t bitmap_size = this->size();

        int32_t local_bitmap_size = bitmap_size;

        if (recompute_bitmaps)
        {
            local_bitmap_size = bitmap_size;
            for (int32_t c = 0; c < Indexes - 1; c++, local_bitmap_size /= 2)
            {
                const BitmapType* src_bitmap = this->symbols(c);
                BitmapType* tgt_bitmap = this->symbols(c + 1);

                int32_t atom_bits_size = sizeof(BitmapType) * 8;
                int32_t bitmap_atoms_size = local_bitmap_size / atom_bits_size;

                for (int32_t bms = 0; bms < bitmap_atoms_size; bms += 2)
                {
                    BitmapType a1 = src_bitmap[bms];
                    BitmapType a2 = src_bitmap[bms + 1];
                    BitmapType rs = a1 | (a2 << atom_bits_size / 2);
                    tgt_bitmap[bms / 2] = rs;
                }
            }
        }

        local_bitmap_size = bitmap_size;
        for (int32_t c = 0; c < Indexes; c++, local_bitmap_size /= 2)
        {
            int32_t index_size = index_level_size(local_bitmap_size);
            if (index_size > 0)
            {
                IndexType* index = this->index(c);
                const BitmapType* bitmap = symbols(c);

                for (int32_t bi = 0, icnt = 0; bi < local_bitmap_size; bi += ValuesPerBranch, icnt++)
                {
                    int32_t limit = (bi + ValuesPerBranch <= local_bitmap_size) ? (bi + ValuesPerBranch) : local_bitmap_size;
                    size_t span_size = static_cast<size_t>(limit - bi);

                    index[icnt] = static_cast<IndexType>(span_size - PopCount(bitmap, bi, limit));
                }
            }
        }

        return VoidResult::of();
    }

    VoidResult set_bits(int32_t level, int32_t idx, int32_t size) noexcept
    {
        size_t start = idx;
        size_t stop  = idx + size;

        for (int32_t ll = level; ll >= 0; ll--)
        {
            BitmapType* bitmap = this->symbols(ll);
            FillOne(bitmap, start, stop);

            start *= 2;
            stop  *= 2;
        }

        start = idx;
        stop  = idx + size;

        for (int32_t ll = level + 1; ll < Indexes; ll++)
        {
            start = start / 2;
            stop  = divUp(stop, static_cast<size_t>(2));

            BitmapType* bitmap = this->symbols(ll);
            FillOne(bitmap, start, stop);
        }

        return VoidResult::of();
    }

    int32_t get_bit(int32_t level, int32_t idx) const noexcept
    {
        const BitmapType* bitmap = this->symbols(level);
        return GetBit(bitmap, idx);
    }


    VoidResult clear_bits(int32_t level, int32_t idx, int32_t size) noexcept
    {
        size_t start = idx;
        size_t stop  = idx + size;

        for (int32_t ll = level; ll >= 0; ll--)
        {
            BitmapType* bitmap = this->symbols(ll);
            FillZero(bitmap, start, stop);

            start *= 2;
            stop  *= 2;
        }

        rebuild_bitmaps(level);
        return reindex(false);
    }

    int32_t sum(int32_t level) const noexcept
    {
        size_t value = 0;

        int32_t bitmap_size = bitmap_level_size(size(), level);
        int32_t index_size = this->index_level_size(bitmap_size);

        if (MMA_LIKELY(index_size > 0))
        {
            const IndexType* idxs = this->index(level);
            for (int32_t c = 0; c < index_size; c++) {
                value += idxs[c];
            }
        }
        else {
            const BitmapType* bitmap = symbols(level);
            value = bitmap_size - PopCount(bitmap, 0, bitmap_size);
        }

        return static_cast<int32_t>(value);
    }


    struct SelectResult {
        int32_t pos;
        int32_t bm_size;
        int64_t rank;
    };


    SelectResult select0(int64_t rank, int32_t level) const noexcept
    {
        int32_t bm_size  = bitmap_level_size(size(), level);
        int32_t idx_size = index_level_size(bm_size);

        size_t bm_start = 0;
        int64_t sum{};

        int64_t rank_tmp = rank;

        if (idx_size > 0)
        {
            const IndexType* idxs = this->index(level);

            for (int32_t c = 0; c < idx_size; c++, bm_start += ValuesPerBranch)
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
            static_cast<int32_t>(result.local_pos()),
            bm_size,
            static_cast<int64_t>(result.rank()) + sum
        };
    }

    int32_t rank(int32_t pos, int32_t level) const noexcept
    {
        int32_t block_start = (pos / ValuesPerBranch) * ValuesPerBranch;

        const BitmapType* bitmap = symbols(level);

        size_t ones     = PopCount(bitmap, block_start, pos);
        int32_t zeroes  = (pos - block_start) - static_cast<int32_t>(ones);

        if (MMA_LIKELY(block_start > 0))
        {
            const IndexType* idxs = index(level);
            for (int32_t c  = 0; c < pos / ValuesPerBranch; c++)
            {
                zeroes += idxs[c];
            }
        }

        return zeroes;
    }


    auto selectFW(int64_t rank, int32_t level) const noexcept
    {
        auto res = select0(rank, level);
        return memoria::SelectResult{
            static_cast<size_t>(res.pos) << level,
            static_cast<size_t>(res.rank),
            res.pos < res.bm_size
        };
    }

    memoria::SelectResult selectFW(int32_t start, int64_t rank, int32_t level) const noexcept
    {
        int32_t startrank_ = this->rank(start, level);
        auto result = selectFW(startrank_ + rank, level);

        result.rank() -= startrank_;

        return result;
    }


    class CountResult {
        int32_t count_;
        int32_t symbol_;
    public:
        CountResult(int32_t count, int32_t symbol) noexcept:
            count_(count), symbol_(symbol)
        {}

        int32_t count() const noexcept {return count_;}
        int32_t symbol() const noexcept {return symbol_;}
    };

    CountResult countFW(int32_t start, int32_t level) const noexcept
    {
        int32_t bm_size     = bitmap_level_size(size(), level);
        int32_t block_end   = (start / ValuesPerBranch + 1) * ValuesPerBranch;
        int32_t block_limit = block_end > bm_size ? bm_size : block_end;

        const BitmapType* bitmap = symbols(level);
        int32_t zeroes = CountTrailingZeroes(bitmap, start, block_limit);

        if (zeroes < block_limit - start) {
            return CountResult{zeroes, level};
        }
        else {
            int32_t index_size = index_level_size(bm_size);
            const IndexType* idxs = index(level);

            int32_t index_start = start / ValuesPerBranch + 1;
            int32_t sum{};

            for (int32_t ii = index_start; ii < index_size; ii++)
            {
                int32_t index_block_start = ii * ValuesPerBranch;
                int32_t index_block_end = (ii + 1) * ValuesPerBranch;
                int32_t index_block_limit = index_block_end > bm_size ? bm_size : index_block_end;
                int32_t index_block_size  = index_block_limit - index_block_start;

                int32_t cell_value = idxs[ii];

                if (cell_value == index_block_size) {
                    sum += cell_value;
                }
                else {
                    int32_t blk_cnt = CountTrailingZeroes(bitmap, index_block_start, index_block_limit);
                    sum += blk_cnt;
                    break;
                }
            }

            return CountResult{zeroes + sum, level};
        }
    }

    static int32_t initial_size(int32_t available_space) noexcept
    {
        return find_block_size(available_space);
    }


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startGroup("PACKED_ALLOCATION_MAP");

        int32_t bm_size = this->size();
        handler->value("SIZE", &bm_size);

        handler->startGroup("BITMAPS", bm_size);

        for (int32_t ll = 0; ll < Indexes; ll++)
        {
            int32_t bm_level_size  = bitmap_level_size(bm_size, ll);
            int32_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                handler->value("INDEX", index(ll), idx_level_size);
            }

            handler->symbols("SYMBOLS", symbols(ll), bm_level_size, 1);
        }

        handler->endGroup();

        handler->endGroup();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        const Metadata* meta = this->metadata();

        int32_t bm_size = meta->size();
        FieldFactory<int32_t>::serialize(buf, bm_size);

        for (int32_t ll = 0; ll < Indexes; ll++)
        {
            int32_t bm_level_size  = bitmap_level_size(bm_size, ll);
            int32_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                FieldFactory<IndexType>::serialize(buf, index(ll), idx_level_size);
            }

            int32_t bm_atoms = divUp(bm_level_size, static_cast<int32_t>(sizeof(BitmapType) * 8));
            FieldFactory<BitmapType>::serialize(buf, symbols(ll), bm_atoms);
        }

        return VoidResult::of();
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());
        int32_t bm_size = meta->size();

        for (int32_t ll = 0; ll < Indexes; ll++)
        {
            int32_t bm_level_size  = bitmap_level_size(bm_size, ll);
            int32_t idx_level_size = index_level_size(bm_level_size);

            if (idx_level_size > 0)
            {
                FieldFactory<IndexType>::deserialize(buf, index(ll), idx_level_size);
            }

            int32_t bm_atoms = divUp(bm_level_size, static_cast<int32_t>(sizeof(BitmapType) * 8));
            FieldFactory<BitmapType>::deserialize(buf, symbols(ll), bm_atoms);
        }

        return VoidResult::of();
    }



private:

    constexpr static int32_t index_level_size(int32_t bitmap_size) noexcept {
        return bitmap_size > ValuesPerBranch ? divUp(bitmap_size, ValuesPerBranch) : 0;
    }

    constexpr static int32_t bitmap_level_size(int32_t l0_bitmap_size, int32_t level) noexcept {
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

    void rebuild_bitmaps(int32_t level_from) noexcept
    {
        int32_t bitmap_size = this->size();
        int32_t local_bitmap_size = bitmap_level_size(bitmap_size, level_from);
        for (int32_t c = level_from; c < Indexes - 1; c++, local_bitmap_size /= 2)
        {
            const BitmapType* src_bitmap = this->symbols(c);
            BitmapType* tgt_bitmap = this->symbols(c + 1);

            int32_t atom_bits_size = sizeof(BitmapType) * 8;
            int32_t bitmap_atoms_size = local_bitmap_size / atom_bits_size;

            for (int32_t bms = 0; bms < bitmap_atoms_size; bms += 2)
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
    static constexpr int32_t Indexes = PkdAllocationMap<Types>::Indexes;
};



}
