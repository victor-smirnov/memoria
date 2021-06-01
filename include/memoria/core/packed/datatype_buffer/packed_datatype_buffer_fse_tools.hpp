
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
        return pkd_buf_->template get<const T>(DataBlock);
    }

    static VoidResult allocateEmpty(PkdStruct* alloc) noexcept {
        return VoidResult::of();
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

    VoidResult insert_space(psize_t start, psize_t size, psize_t data_len) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        psize_t column_data_length = size + meta.size();

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length * sizeof(T)));

        auto data = this->data();

        psize_t data_start  = start;
        psize_t data_end    = data_start + size;

        MemMoveBuffer(data + data_start, data + data_end, meta.size() - start);

        return VoidResult::of();
    }

    VoidResult remove_space(psize_t start, psize_t size) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        auto data = this->data();

        MemMoveBuffer(data + start + size, data + start, meta.size() - start - size);

        psize_t column_data_length = (meta.size() - size) * sizeof(T);

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length));

        return VoidResult::of();
    }

    VoidResult resize_row(psize_t idx, const T* value) noexcept
    {
        return VoidResult::of();
    }

    VoidResult replace_row(psize_t idx, const T* value) noexcept
    {
        *(this->data() + idx) = *value;
        return VoidResult::of();
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
    VoidResult serialize(const Metadata& meta, SerializationData& buf) const noexcept
    {
        return wrap_throwing([&] {
            FieldFactory<T>::serialize(buf, data(), meta.size());
        });
    }

    template <typename SerializationData, typename Metadata, typename IDResolver>
    VoidResult cow_serialize(const Metadata& meta, SerializationData& buf, const IDResolver* id_resolver) const noexcept
    {
        return wrap_throwing([&] {
            auto size = meta.size();
            auto data = this->data();
            for (psize_t c = 0; c < size; c++)
            {
                auto new_id = id_resolver->resolve_id(data[c]);
                FieldFactory<T>::serialize(buf, new_id);
            }
        });
    }

    template <typename DeserializationData, typename Metadata>
    VoidResult deserialize(Metadata& meta, DeserializationData& buf) noexcept
    {
        return wrap_throwing([&] {
            FieldFactory<T>::deserialize(buf, data(), meta.size());
        });
    }
};

}}
