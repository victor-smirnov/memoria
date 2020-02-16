
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
    ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedSizedStructSO;

public:
    using PkdStructT = PkdStruct;

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

    VoidResult splitTo(MyType& other, int32_t idx) noexcept {
        return data_->splitTo(other.data(), idx);
    }

    VoidResult mergeWith(MyType& other) const noexcept {
        return data_->mergeWith(other.data());
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) noexcept {
        return data_->removeSpace(room_start, room_end);
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept {
        return data_->generateDataEvents(handler);
    }

    int32_t size() const noexcept {
        return data_->size();
    }

    VoidResult check() const noexcept {
        return data_->check();
    }

    auto sum(int32_t column) const noexcept {
        return data_->sum(column);
    }



    void configure_io_substream(io::IOSubstream& substream) const {
        return data_->configure_io_substream(substream);
    }



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

