
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/tools/bitmap_select.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedSSRLESeqSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedSSRLESeqSO;

public:
    using SeqSizeT = typename PkdStruct::SeqSizeT;
    using SymbolT = typename PkdStruct::SymbolT;

    using PkdStructT = PkdStruct;

    PackedSSRLESeqSO() noexcept: ext_data_(), data_() {}
    PackedSSRLESeqSO(ExtData* ext_data, PkdStruct* data) noexcept:
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

    VoidResult splitTo(MyType& other, int32_t idx) noexcept
    {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) const noexcept {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) noexcept {
        return data_->removeSpace(room_start, room_end);
    }

    size_t size() const noexcept {
        return (size_t)data_->size();
    }

    SymbolT get_symbol(SeqSizeT idx) const noexcept {
        return data_->access(idx);
    }

    auto rank(SeqSizeT pos, SymbolT symbol) const noexcept {
        return (size_t)data_->rank_eq(pos, symbol);
    }



    template <typename... Args>
    auto selectGEFW(size_t rank, size_t symbol) const {
        auto res = data_->select_fw_ge(rank, symbol);
        return SelectResult{(size_t)res.idx, (size_t)res.rank, res.idx < data_->size()};
    }

    template <typename... Args>
    auto selectGEFW(size_t start, size_t rank, size_t symbol) const {
        auto res = data_->select_fw_ge(start, rank, symbol);
        return SelectResult{(size_t)res.idx, (size_t)res.rank, res.idx < data_->size()};
    }

    template <typename... Args>
    auto selectFW(size_t rank, size_t symbol) const {
        auto res = data_->select_fw_eq(rank, symbol);
        return SelectResult{(size_t)res.idx, (size_t)res.rank, res.idx < data_->size()};
    }

    template <typename... Args>
    auto selectFW(size_t start, size_t rank, size_t symbol) const {
        auto res = data_->select_fw_eq(start, rank, symbol);
        return SelectResult{(size_t)res.idx, (size_t)res.rank, res.idx < data_->size()};
    }


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept {
        return data_->generateDataEvents(handler);
    }

    VoidResult check() const noexcept {
        return data_->check();
    }

    auto sum(int32_t symbol) const noexcept {
        return (uint64_t)data_->rank_eq(symbol);
    }

    void configure_io_substream(io::IOSubstream& substream) const {
        return data_->configure_io_substream(substream);
    }

    Int32Result insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size) noexcept
    {
        MEMORIA_TRY_VOID(data_->insert_io_substream(at, substream, start, size));
        return Int32Result::of(at + size);
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_TRY_VOID(data_->insertSpace(row_at, size));

        for (psize_t c = 0; c < size; c++)
        {
            int32_t symbol = elements(c);
            data_->insert(row_at, symbol, 1);
        }

        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_TRY_VOID(data_->removeSpace(row_at, row_at + size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    template <typename AccessorFn>
    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        return data_->removeSpace(row_at, row_at + size);
    }

private:
};



}
