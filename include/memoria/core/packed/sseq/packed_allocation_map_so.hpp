
// Copyright 2019 Victor Smirnov
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
#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/api/allocation_map/allocation_map_api.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedAllocationMapSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedAllocationMapSO;

public:
    using PkdStructT = PkdStruct;
    static constexpr size_t LEVELS = PkdStruct::Indexes;
    static constexpr size_t Indexes = PkdStruct::Indexes;

    using UpdateState = PkdStructUpdate<MyType>;

    PackedAllocationMapSO() noexcept: ext_data_(), data_() {}
    PackedAllocationMapSO(ExtData* ext_data, PkdStruct* data) noexcept:
        ext_data_(ext_data), data_(data)
    {}

    void setup() noexcept {
        ext_data_ = nullptr;
        data_ = nullptr;
    }

    void setup(ExtData* ext_data, PkdStruct* data) noexcept {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(ExtData* ext_data) noexcept {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data) noexcept {
        data_ = data;
    }


    operator bool() const noexcept {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const noexcept {return ext_data_;}
    ExtData* ext_data() noexcept {return ext_data_;}

    const PkdStruct* data() const noexcept {return data_;}
    PkdStruct* data() noexcept {return data_;}

    void set_bits(size_t level, size_t idx, size_t size) noexcept
    {
        return data_->set_bits(level, idx, size);
    }

    void clear_bits(size_t level, size_t idx, size_t size) noexcept
    {
        return data_->clear_bits(level, idx, size);
    }


    void clear_bits_opt(size_t level, size_t idx, size_t size) noexcept
    {
        return data_->clear_bits_opt(level, idx, size);
    }

    void rebuild_bitmaps(size_t level) noexcept {
        data_->rebuild_bitmaps(level);
    }

    void reindex(bool recompute_bitmaps) noexcept {
        return data_->reindex(recompute_bitmaps);
    }

    void split_to(MyType& other, size_t idx) noexcept
    {
        MEMORIA_MAKE_GENERIC_ERROR("Splitting PackedAllocationMap is not supported").do_throw();
    }

    void commit_merge_with(MyType& other, UpdateState&) const noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("Merging PackedAllocationMap is not supported").do_throw();
    }

    PkdUpdateStatus prepare_remove(size_t start, size_t end, UpdateState& update_state) const {
        return PkdUpdateStatus::SUCCESS;
    }

    void commit_remove(size_t start, size_t end, UpdateState& ) noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("Removing space from PackedAllocationMap is not supported").do_throw();
    }

    size_t size() const noexcept {
        return data_->size();
    }


    auto countFW(size_t start, size_t level) const noexcept {
        return data_->countFW(start, level);
    }


    auto selectFW(int64_t rank, size_t level) const noexcept {
        return data_->selectFW(rank, level);
    }

    auto select_fw(size_t start, int64_t rank, size_t level, SeqOpType) const noexcept {
        return data_->selectFW(start, rank, level);
    }

    auto select_fw_out(size_t start, int64_t rank, size_t level, SeqOpType) const noexcept {
        return data_->selectFW(start, rank, level);
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const {
        return data_->generateDataEvents(handler);
    }

    void check() const {
        return data_->check();
    }

    auto sum(size_t level) const noexcept {
        return data_->sum(level);
    }

    void configure_io_substream(io::IOSubstream& substream) const {
        //return data_->configure_io_substream(substream);
    }

    SizeTResult insert_io_substream(size_t at, const io::IOSubstream& substream, size_t start, size_t size) noexcept
    {
        //MEMORIA_TRY_VOID(data_->insert_io_substream(at, substream, start, size));
        return SizeTResult::of(at + size);
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
//        MEMORIA_TRY_VOID(data_->insertSpace(row_at, size));

//        for (psize_t c = 0; c < size; c++)
//        {
//            size_t symbol = elements(c);
//            data_->insert(row_at, symbol, 1);
//        }

        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements, UpdateState&) noexcept
    {
//        MEMORIA_TRY_VOID(data_->remove(row_at, row_at + size));
//        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));

        return VoidResult::of();
    }


    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        //return data_->remove(row_at, row_at + size);
        return VoidResult::of();
    }


    template <typename AllocationPool>
    BoolResult populate_allocation_pool(int64_t base, AllocationPool& pool) noexcept
    {
        return data_->populate_allocation_pool(base, pool);
    }

    MMA_MAKE_UPDATE_STATE_METHOD
};



}
