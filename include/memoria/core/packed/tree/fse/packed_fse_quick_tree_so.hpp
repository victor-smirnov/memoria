
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
#include <memoria/core/tools/static_array.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedFSEQuickTreeSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using Values = typename PkdStruct::Values;
    using Value = typename PkdStruct::Value;
    using IndexValue = typename PkdStruct::IndexValue;

    using MyType = PackedFSEQuickTreeSO;


public:
    using PkdStructT = PkdStruct;

    static constexpr int32_t Blocks = PkdStruct::Blocks;

    PackedFSEQuickTreeSO() noexcept: ext_data_(), data_() {}
    PackedFSEQuickTreeSO(ExtData* ext_data, PkdStruct* data) noexcept:
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


    VoidResult reindex() noexcept {
        return data_->reindex();
    }


    const Value& access(int32_t column, int32_t row) const noexcept {
        return data_->value(column, row);
    }

    template <typename T>
    VoidResult setValues(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept {
        return data_->setValues(idx, values);
    }

    int32_t size() const {
        return data_->size();
    }

    auto findForward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const {
        return data_->findForward(search_type, block, start, val);
    }

    auto findBackward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const {
        return data_->findBackward(search_type, block, start, val);
    }

    const Value& value(int32_t block, int32_t idx) const {
        return data_->value(block, idx);
    }

    template <typename T>
    void _add(int32_t block, int32_t start, int32_t end, T& value) const
    {
        value += data_->sum(block, start, end);
    }



    template <typename T>
    void _sub(int32_t block, int32_t start, int32_t end, T& value) const
    {
        value -= data_->sum(block, start, end);
    }

    template <typename... Args>
    auto sum(Args&&... args) const {
        return data_->sum(std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto findNZLT(Args&&... args) const {
        return data_->findNZLT(std::forward<Args>(args)...);
    }


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept {
        return data_->generateDataEvents(handler);
    }

    VoidResult check() const noexcept {
        return data_->check();
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        return data_->insert_entries(row_at, size, std::forward<AccessorFn>(elements));
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
        MEMORIA_TRY_VOID(data_->removeSpace(row_at, row_at + size));
        return data_->reindex();
    }
};




}
