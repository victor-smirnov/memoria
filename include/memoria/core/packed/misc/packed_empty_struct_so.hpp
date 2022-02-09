
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
#include <memoria/core/tools/result.hpp>


namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedEmptyStructSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedEmptyStructSO;

    using Values = typename PkdStruct::Values;

public:
    using PkdStructT = PkdStruct;
    static constexpr size_t Blocks = PkdStruct::Blocks;


    PackedEmptyStructSO() noexcept: ext_data_(), data_() {}
    PackedEmptyStructSO(ExtData* ext_data, PkdStruct* data) noexcept:
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

    psize_t max_element_idx() const noexcept {
        return 0;
    }

    auto access(size_t column, size_t row) const noexcept {
        return data_->access(column, row);
    }

    VoidResult splitTo(MyType& other, size_t idx) noexcept
    {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) const noexcept {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(size_t room_start, size_t room_end) {
        return data_->removeSpace(room_start, room_end);
    }

    VoidResult reindex() noexcept {
        return data_->reindex();
    }

    Values get_values(size_t idx) const {
        return data_->get_values(idx);
    }

    template <typename T>
    VoidResult setValues(size_t idx, const core::StaticVector<T, Blocks>& values) noexcept {
        return data_->setValues(idx, values);
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const {
        return data_->generateDataEvents(handler);
    }

    void check() const {}

    size_t size() const {
        return data_->size();
    }

    template <typename... Args>
    auto max(Args&&... args) const {
        return data_->max(std::forward<Args>(args)...);
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        return VoidResult::of();
    }

    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        return VoidResult::of();
    }
};


}
