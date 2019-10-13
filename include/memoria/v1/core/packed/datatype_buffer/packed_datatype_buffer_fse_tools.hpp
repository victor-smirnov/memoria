
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

#include <memoria/v1/core/packed/datatype_buffer/packed_datatype_buffer_common_tools.hpp>

namespace memoria {
namespace v1 {
namespace pdtbuf_ {


template <typename T, typename PkdStruct, psize_t Block_>
class PDTDimension<const T*, PkdStruct, Block_> {
    PkdStruct* pkd_buf_;

    using TPtr = const T*;

public:
    static constexpr psize_t Block = Block_;
    static constexpr psize_t Width = 1;

    static constexpr psize_t DataBlock = Block_;
    static constexpr psize_t Dimension = Block_ - 1;

    constexpr PDTDimension(PkdStruct* pkd_buf):
        pkd_buf_(pkd_buf)
    {}

    T* data() {
        return pkd_buf_->template get<T>(Block + 1);
    }

    const T* data() const {
        return pkd_buf_->template get<T>(Block + 1);
    }

    static OpStatus allocateEmpty(PkdStruct* alloc) {
        return OpStatus::OK;
    }

    static constexpr psize_t empty_size_aligned() {
        return 0;
    }

    void set(TPtr& value, psize_t row) const
    {
        const T* dd = data();
        value = dd + row;
    }

    void lengths(psize_t& value, psize_t row, psize_t size) const {
        value = size;
    }

    psize_t compute_new_size(psize_t extra_size, psize_t extra_data_len)
    {
        return 0;
    }

    OpStatus insert_space(psize_t start, psize_t size, psize_t data_len)
    {
        return OpStatus::OK;
    }



    template <typename Metadata>
    static void init_metadata(Metadata& metadata) {
        metadata.data_size(Dimension) = 0;
    }

    template <typename SerializationData, typename Metadata>
    void serialize(const Metadata& meta, SerializationData& buf) const
    {
        FieldFactory<T>::serialize(buf, data(), meta.size());
    }

    template <typename DeserializationData, typename Metadata>
    void deserialize(Metadata& meta, DeserializationData& buf)
    {
        FieldFactory<T>::deserialize(buf, data(), meta.size());
    }
};

}}}
