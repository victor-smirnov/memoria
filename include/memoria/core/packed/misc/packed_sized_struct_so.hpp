
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
#include <memoria/core/tools/result.hpp>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedSizedStructSO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedSizedStructSO;

public:
    using PkdStructT = PkdStruct;

    PackedSizedStructSO(): ext_data_(), data_() {}
    PackedSizedStructSO(const ExtData* ext_data, PkdStruct* data):
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

    VoidResult splitTo(MyType& other, int32_t idx) noexcept
    {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) noexcept {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) noexcept {
        return data_->removeSpace(room_start, room_end);
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept {
        return data_->generateDataEvents(handler);
    }

    int32_t size() const {
        return data_->size();
    }

    void check() const {
        return data_->check();
    }

    auto sum(int32_t column) const noexcept {
        return data_->sum(column);
    }



    void configure_io_substream(io::IOSubstream& substream) const {
        return data_->configure_io_substream(substream);
    }

//    template <int32_t Offset, typename... Args>
//    VoidResult _insert_b(Args&&... args) {
//        return data_->template _insert_b<Offset>(std::forward<Args>(args)...);
//    }

//    template <int32_t Offset, typename... Args>
//    VoidResult _remove(Args&&... args) noexcept {
//        return data_->template _remove<Offset>(std::forward<Args>(args)...);
//    }

    VoidResult insertSpace(int32_t idx, int32_t room_length) noexcept {
        return data_->insertSpace(idx, room_length);
    }


    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());
        return data_->insertSpace(row_at, size);
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());
        return VoidResult::of();
    }

    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        return removeSpace(row_at, row_at + size);
    }

};


}

