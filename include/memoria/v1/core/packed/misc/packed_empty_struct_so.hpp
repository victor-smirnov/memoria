
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/profiles/common/block_operations.hpp>

namespace memoria {
namespace v1 {

template <typename ExtData, typename PkdStruct>
class PackedEmptyStructSO {
    const ExtData* ext_data_;
    PkdStruct* data_;

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

    OpStatus splitTo(PkdStruct* other, int32_t idx)
    {
        return data_->splitTo(other, idx);
    }

    OpStatus mergeWith(PkdStruct* other) {
        return data_->mergeWith(other);
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return data_->removeSpace(room_start, room_end);
    }

    template <typename T>
    OpStatus insert(int32_t idx, const core::StaticVector<T, Blocks>& values) {
        return data_->insert(idx, values);
    }

    template <typename Fn>
    OpStatus insert(int32_t idx, int32_t length, Fn&& fn) {
        return data_->insert(idx, length, std::forward<Fn>(fn));
    }



    OpStatus reindex() {
        return data_->reindex();
    }

    Values get_values(int32_t idx) const {
        return data_->get_values(idx);
    }

    template <typename T>
    OpStatus setValues(int32_t idx, const core::StaticVector<T, Blocks>& values) {
        return data_->setValues(idx, values);
    }

    void generateDataEvents(IBlockDataEventHandler* handler) const {
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
};


}
}
