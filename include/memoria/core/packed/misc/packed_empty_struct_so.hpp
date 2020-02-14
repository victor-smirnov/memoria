
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
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedEmptyStructSO;

    using Values = typename PkdStruct::Values;

public:
    using PkdStructT = PkdStruct;
    static constexpr int32_t Blocks = PkdStruct::Blocks;


    PackedEmptyStructSO(): ext_data_(), data_() {}
    PackedEmptyStructSO(const ExtData* ext_data, PkdStruct* data):
        ext_data_(ext_data), data_(data)
    {}

    void setup() {
        ext_data_ = nullptr;
        data_ = nullptr;
    }

    void setup(const ExtData* ext_data, PkdStruct* data) {
        ext_data_ = ext_data;
        data_ = data;
    }

    void setup(const ExtData* ext_data) {
        ext_data_ = ext_data;
    }

    void setup(PkdStruct* data) {
        data_ = data;
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const {return ext_data_;}
    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    psize_t max_element_idx() const noexcept {
        return 0;
    }

    auto access(int32_t column, int32_t row) const noexcept {
        return data_->access(column, row);
    }

    VoidResult splitTo(MyType& other, int32_t idx) noexcept
    {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) noexcept {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) {
        return data_->removeSpace(room_start, room_end);
    }

//    template <typename T>
//    VoidResult insert(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept {
//        return data_->insert(idx, values);
//    }

//    template <typename Fn>
//    VoidResult insert(int32_t idx, int32_t length, Fn&& fn) noexcept {
//        return data_->insert(idx, length, std::forward<Fn>(fn));
//    }



    VoidResult reindex() noexcept {
        return data_->reindex();
    }

    Values get_values(int32_t idx) const {
        return data_->get_values(idx);
    }

    template <typename T>
    VoidResult setValues(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept {
        return data_->setValues(idx, values);
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept {
        return data_->generateDataEvents(handler);
    }

    void check() const {
        return data_->check();
    }

    int32_t size() const {
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
