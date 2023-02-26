
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

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

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

    using UpdateState = PkdStructNoOpUpdate<MyType>;

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

    void split_to(MyType& other, size_t idx) noexcept
    {
        return data_->split_to(other.data(), idx);
    }

    PkdUpdateStatus prepare_merge_with(const MyType&, UpdateState&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    void commit_merge_with(MyType& other, UpdateState&) const noexcept {
        return data_->commit_merge_with(other.data());
    }

    PkdUpdateStatus prepare_remove(size_t, size_t, UpdateState&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    void commit_remove(size_t room_start, size_t room_end, UpdateState&) {
        return data_->remove(room_start, room_end);
    }

    void reindex() noexcept {
        return data_->reindex();
    }

    Values get_values(size_t idx) const {
        return data_->get_values(idx);
    }

    template <typename T>
    void setValues(size_t idx, const core::StaticVector<T, Blocks>& values) noexcept {
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
    PkdUpdateStatus prepare_insert(psize_t row_at, psize_t size, UpdateState&, AccessorFn&&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    template <typename AccessorFn>
    void commit_insert(psize_t row_at, psize_t size, UpdateState&, AccessorFn&& elements) noexcept
    {}

    template <typename AccessorFn>
    PkdUpdateStatus prepare_update(psize_t row_at, psize_t size, UpdateState&, AccessorFn&&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    template <typename AccessorFn>
    void commit_update(psize_t row_at, psize_t size, UpdateState&, AccessorFn&& elements) noexcept
    {}

    MMA_MAKE_UPDATE_STATE_METHOD
};


}
