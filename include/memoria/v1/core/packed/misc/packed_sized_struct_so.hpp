
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

    OpStatus splitTo(MyType& other, int32_t idx)
    {
        return data_->splitTo(other.data(), idx);
    }

    OpStatus mergeWith(MyType& other) {
        return data_->mergeWith(other.data());
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return data_->removeSpace(room_start, room_end);
    }




    void generateDataEvents(IBlockDataEventHandler* handler) const {
        return data_->generateDataEvents(handler);
    }

    int32_t size() const {
        return data_->size();
    }

    void check() const {
        return data_->check();
    }

    template <int32_t Offset, typename... Args>
    auto max(Args&&... args) const {
        return data_->template max<Offset>(std::forward<Args>(args)...);
    }

    void configure_io_substream(io::IOSubstream& substream) const {
        return data_->configure_io_substream(substream);
    }

    template <int32_t Offset, typename... Args>
    OpStatus _insert_b(Args&&... args) {
        return data_->template _insert_b<Offset>(std::forward<Args>(args)...);
    }

    template <int32_t Offset, typename... Args>
    OpStatus _remove(Args&&... args) {
        return data_->template _remove<Offset>(std::forward<Args>(args)...);
    }

    OpStatus insertSpace(int32_t idx, int32_t room_length) {
        return data_->insertSpace(idx, room_length);
    }
};


}
}