
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
        return pkd_buf_->template get<T>(DataBlock);
    }

    const T* data() const {
        return pkd_buf_->template get<T>(DataBlock);
    }

    static OpStatus allocateEmpty(PkdStruct* alloc) {
        return OpStatus::OK;
    }

    static constexpr psize_t empty_size_aligned() {
        return 0;
    }

    static psize_t data_block_size(psize_t capacity)
    {
        psize_t size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            capacity * sizeof(T)
        );

        return size;
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
        auto meta = pkd_buf_->metadata();

        psize_t data_length_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    (extra_data_len + meta.size()) * sizeof(T)
        );

        return data_length_aligned;
    }

    template <typename Metadata>
    psize_t joint_data_length(const Metadata& my_meta, const PkdStruct* other, const Metadata& other_meta) const
    {
        psize_t data_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
             (my_meta.size() + other_meta.size()) * sizeof(T)
        );

        return data_size;
    }

    OpStatus insert_space(psize_t start, psize_t size, psize_t data_len)
    {
        auto& meta = pkd_buf_->metadata();

        psize_t column_data_length = size + meta.size();

        if(isFail(pkd_buf_->resizeBlock(DataBlock, column_data_length * sizeof(T)))) {
            return OpStatus::FAIL;
        }

        auto data = this->data();

        psize_t data_start  = start;
        psize_t data_end    = data_start + size;

        MemMoveBuffer(data + data_start, data + data_end, meta.size() - start);

        return OpStatus::OK;
    }

    OpStatus remove_space(psize_t start, psize_t size)
    {
        auto& meta = pkd_buf_->metadata();

        auto data = this->data();

        MemMoveBuffer(data + start, data + start + size, meta.size() - start - size);

        psize_t column_data_length = (meta.size() - size) * sizeof(T);

        if(isFail(pkd_buf_->resizeBlock(DataBlock, column_data_length))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus resize_row(psize_t idx, const T* value)
    {
        return OpStatus::OK;
    }

    void replace_row(psize_t idx, const T* value)
    {
        *(this->data() + idx) = *value;
    }

    void copy_to(PkdStruct* other, psize_t copy_from, psize_t count, psize_t copy_to, psize_t data_length) const
    {
        auto other_dim = other->template dimension<Dimension>();

        MemCpyBuffer(
            this->data() + copy_from,
            other_dim.data() + copy_to,
            count
        );
    }

    void copy_from(psize_t idx, const T* data)
    {
        *(this->data() + idx) = *data;
    }

    template <typename Buffer>
    void copy_from_databuffer(psize_t idx, psize_t start, psize_t size, psize_t data_length, const Buffer& buffer)
    {
        const auto* data_src = buffer.template data<Dimension>(start);
        MemCpyBuffer(data_src, this->data() + idx, size);
    }

    template <typename Metadata>
    psize_t estimate_insert_upsize(const T* data, const Metadata& meta) const
    {
        psize_t column_data_size = meta.size();
        psize_t view_length = 1;

        psize_t column_data_size_aligned0 = pkd_buf_->element_size(DataBlock);

        psize_t column_data_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            (view_length + column_data_size) * sizeof(T)
        );

        return column_data_size_aligned1 - column_data_size_aligned0;
    }

    template <typename Metadata>
    psize_t estimate_replace_upsize(psize_t idx, const T* span, const Metadata& meta) const
    {
        return 0;
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
