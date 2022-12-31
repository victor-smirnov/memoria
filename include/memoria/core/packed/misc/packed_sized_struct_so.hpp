
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
#include <memoria/core/tools/result.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/api/common/ctr_batch_input.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedSizedStructSO {
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedSizedStructSO;

public:
    using PkdStructT = PkdStruct;

    using UpdateState = PkdStructNoOpUpdate<PackedSizedStructSO>;

    PackedSizedStructSO() noexcept: ext_data_(), data_() {}
    PackedSizedStructSO(ExtData* ext_data, PkdStruct* data) noexcept:
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

    void split_to(MyType& other, size_t idx) noexcept {
        return data_->split_to(other.data(), idx);
    }

    PkdUpdateStatus prepare_merge_with(const MyType& other, UpdateState& update_state) const {
        return PkdUpdateStatus::SUCCESS;
    }

    void commit_merge_with(MyType& other, UpdateState&) const noexcept {
        return data_->commit_merge_with(other.data());
    }

    PkdUpdateStatus prepare_remove(size_t room_start, size_t room_end, UpdateState& update_state) const {
        return PkdUpdateStatus::SUCCESS;
    }

    void commit_remove(size_t room_start, size_t room_end, UpdateState&) noexcept {
        return data_->remove(room_start, room_end);
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const {
        return data_->generateDataEvents(handler);
    }

    size_t size() const noexcept {
        return data_->size();
    }

    void check() const {
        return data_->check();
    }

    auto sum(size_t column) const noexcept {
        return data_->sum(column);
    }

    template <typename DT>
    PkdUpdateStatus prepare_insert_io_substream(size_t at, const HermesDTBuffer<DT>& substream, size_t start, size_t size, UpdateState&) {
        return PkdUpdateStatus::SUCCESS;
    }


    // FIXME: Adapt to multicolumn!
    template <typename DT>
    size_t commit_insert_io_substream(size_t at, const HermesDTBuffer<DT>& substream, size_t start, size_t size, UpdateState&)
    {
        insert_space(0, size);
        return size;
    }


    void insert_space(size_t idx, size_t room_length) noexcept {
        return data_->insert_space(idx, room_length);
    }

    template <typename AccessorFn>
    PkdUpdateStatus prepare_insert(psize_t row_at, psize_t size, UpdateState&, AccessorFn&&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    template <typename AccessorFn>
    void commit_insert(psize_t row_at, psize_t size, UpdateState&, AccessorFn&& elements) noexcept {
        MEMORIA_ASSERT(row_at, <=, this->size());
        return data_->insert_space(row_at, size);
    }

    template <typename AccessorFn>
    PkdUpdateStatus prepare_update(psize_t row_at, psize_t size, UpdateState&, AccessorFn&&) const {
        return PkdUpdateStatus::SUCCESS;
    }

    template <typename AccessorFn>
    void commit_update(psize_t row_at, psize_t size, UpdateState&, AccessorFn&& elements) noexcept
    {
        MEMORIA_ASSERT(row_at, <=, this->size());
    }

    MMA_MAKE_UPDATE_STATE_METHOD
};


}

