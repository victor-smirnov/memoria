
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

#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_common_tools.hpp>

namespace memoria {
namespace pdtbuf_ {


template <typename T, typename PkdStruct, size_t Block_>
class PDTDimension<const T*, PkdStruct, Block_> {
    PkdStruct* pkd_buf_;

    using TPtr = const T*;

public:
    static constexpr size_t Block = Block_;
    static constexpr size_t Width = 1;

    static constexpr size_t DataBlock = Block_;
    static constexpr size_t Dimension = Block_ - 1;

    constexpr PDTDimension(PkdStruct* pkd_buf):
        pkd_buf_(pkd_buf)
    {}

    T* data() {
        return pkd_buf_->template get<T>(DataBlock);
    }

    const T* data() const {
        return pkd_buf_->template get<const T>(DataBlock);
    }

    static VoidResult allocate_empty(PkdStruct* alloc) noexcept {
        return VoidResult::of();
    }

    static constexpr size_t empty_size_aligned() {
        return 0;
    }

    static size_t data_block_size(size_t capacity)
    {
        size_t size = PackedAllocatable::round_up_bytes_to_alignment_blocks(
            capacity * sizeof(T)
        );

        return size;
    }

    void set(TPtr& value, size_t row) const
    {
        const T* dd = data();
        value = dd + row;
    }

    void lengths(size_t& value, size_t row, size_t size) const {
        value = size;
    }

    size_t compute_new_size(size_t extra_size, size_t extra_data_len)
    {
        auto meta = pkd_buf_->metadata();

        size_t data_length_aligned = PackedAllocatable::round_up_bytes_to_alignment_blocks(
                    (extra_data_len + meta.size()) * sizeof(T)
        );

        return data_length_aligned;
    }

    template <typename Metadata>
    size_t joint_data_length(const Metadata& my_meta, const PkdStruct* other, const Metadata& other_meta) const
    {
        size_t data_size = PackedAllocatable::round_up_bytes_to_alignment_blocks(
             (my_meta.size() + other_meta.size()) * sizeof(T)
        );

        return data_size;
    }

    VoidResult insert_space(size_t start, size_t size, size_t data_len) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        size_t column_data_length = size + meta.size();

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(DataBlock, column_data_length * sizeof(T)));

        auto data = this->data();

        size_t data_start  = start;
        size_t data_end    = data_start + size;

        MemMoveBuffer(data + data_start, data + data_end, meta.size() - start);

        return VoidResult::of();
    }

    VoidResult remove_space(size_t start, size_t size) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        auto data = this->data();

        MemMoveBuffer(data + start + size, data + start, meta.size() - start - size);

        size_t column_data_length = (meta.size() - size) * sizeof(T);

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(DataBlock, column_data_length));

        return VoidResult::of();
    }

    VoidResult resize_row(size_t idx, const T* value) noexcept
    {
        return VoidResult::of();
    }

    VoidResult replace_row(size_t idx, const T* value) noexcept
    {
        *(this->data() + idx) = *value;
        return VoidResult::of();
    }

    void copy_to(PkdStruct* other, size_t copy_from, size_t count, size_t copy_to, size_t data_length) const
    {
        auto other_dim = other->template dimension<Dimension>();

        MemCpyBuffer(
            this->data() + copy_from,
            other_dim.data() + copy_to,
            count
        );
    }

    void copy_from(size_t idx, const T* data)
    {
        *(this->data() + idx) = *data;
    }

    template <typename Buffer>
    void copy_from_databuffer(size_t idx, size_t start, size_t size, size_t data_length, const Buffer& buffer)
    {
        const auto* data_src = buffer.template data<Dimension>(start);
        MemCpyBuffer(data_src, this->data() + idx, size);
    }

    template <typename Metadata>
    size_t estimate_insert_upsize(const T* data, const Metadata& meta) const
    {
        size_t column_data_size = meta.size();
        size_t view_length = 1;

        size_t column_data_size_aligned0 = pkd_buf_->element_size(DataBlock);

        size_t column_data_size_aligned1 = PackedAllocatable::round_up_bytes_to_alignment_blocks(
            (view_length + column_data_size) * sizeof(T)
        );

        return column_data_size_aligned1 - column_data_size_aligned0;
    }

    template <typename Metadata>
    size_t estimate_replace_upsize(size_t idx, const T* span, const Metadata& meta) const
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

    template <typename SerializationData, typename Metadata, typename IDResolver>
    void cow_serialize(const Metadata& meta, SerializationData& buf, const IDResolver* id_resolver) const
    {
        auto size = meta.size();
        auto data = this->data();
        for (size_t c = 0; c < size; c++)
        {
            auto new_id = id_resolver->resolve_id(data[c]);
            FieldFactory<T>::serialize(buf, new_id);
        }
    }

    template <typename DeserializationData, typename Metadata>
    void deserialize(Metadata& meta, DeserializationData& buf)
    {
        FieldFactory<T>::deserialize(buf, data(), meta.size());
    }
};

}}
