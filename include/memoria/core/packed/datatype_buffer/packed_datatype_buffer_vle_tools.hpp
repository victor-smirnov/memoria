
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer_common_tools.hpp>
#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <type_traits>


namespace memoria {
namespace pdtbuf_ {


template <typename T, typename PkdStruct, size_t Dimension, size_t Block, size_t Blocks, size_t DimensionsStart>
class PDTDimension<Span<const T>, PkdStruct, Dimension, Block, Blocks, DimensionsStart> {
    PkdStruct* pkd_buf_;
    size_t column_;

    using DataSizeType = psize_t;

    static constexpr size_t DataBlock       = Block + 0;
    static constexpr size_t OffsetsBlock    = Block + 1;

public:
    static constexpr size_t Width = 2;

    PDTDimension(PkdStruct* pkd_buf, size_t column):
        pkd_buf_(pkd_buf),
        column_(column)
    {}


    constexpr size_t data_block_num() const noexcept {
        return DimensionsStart + Blocks * column_ + DataBlock;
    }

    constexpr size_t offsets_block_num() const noexcept {
        return DimensionsStart + Blocks * column_ + OffsetsBlock;
    }



    T* data() {
        return pkd_buf_->template get<T>(data_block_num());
    }

    const T* data() const {
        return pkd_buf_->template get<T>(data_block_num());
    }

    DataSizeType* offsets() {
        return pkd_buf_->template get<DataSizeType>(offsets_block_num());
    }

    const DataSizeType* offsets() const {
        return pkd_buf_->template get<DataSizeType>(offsets_block_num());
    }

    static VoidResult allocate_empty(PkdStruct* alloc, size_t column)
    {
        MEMORIA_TRY_VOID(alloc->template allocate_array_by_size<DataSizeType>(DimensionsStart + Blocks * column + OffsetsBlock, 1));
        return VoidResult::of();
    }

    static size_t data_block_size(size_t capacity) {
        return 0;
    }

    template <typename Metadata>
    static void init_metadata(Metadata& metadata, size_t column) {
    }

    static constexpr size_t empty_size_aligned() {
        return PackedAllocatable::round_up_bytes_to_alignment_blocks(sizeof(psize_t));
    }

    template <typename Metadata>
    size_t data_size(const Metadata& meta) const {
        return offsets()[meta.size()];
    }

    template <typename Metadata>
    size_t joint_data_length(const Metadata& my_meta, const PkdStruct* other, const Metadata& other_meta) const
    {
        size_t my_data_size = data_size(my_meta);
        size_t other_data_size = other->template dimension<Dimension>(column_).data_size(other_meta);

        size_t data_size = PackedAllocatable::round_up_bytes_to_alignment_blocks(
             (my_data_size + other_data_size) * sizeof(T)
        );

        size_t offsets_size = PackedAllocatable::round_up_bytes_to_alignment_blocks(
             (my_meta.offsets_size() + other_meta.offsets_size() - 1) * sizeof (DataSizeType)
        );

        return data_size + offsets_size;
    }

    void set(Span<const T>& span, size_t row) const
    {
        const DataSizeType* offs = offsets();
        const T* dd = data();
        span = Span<const T>(dd + offs[row], offs[row + 1] - offs[row]);
    }

    void lengths(size_t& value, size_t row, size_t size) const
    {
        const DataSizeType* offs = offsets();
        value = offs[row + size] - offs[row];
    }

    size_t length(size_t row, size_t size) const
    {
        const DataSizeType* offs = offsets();
        return offs[row + size] - offs[row];
    }

    size_t length(size_t row) const
    {
        const DataSizeType* offs = offsets();
        return offs[row + 1] - offs[row];
    }



    size_t compute_new_size(size_t extra_size, size_t extra_data_len)
    {
        auto meta = pkd_buf_->metadata();

        size_t data_size = this->data_size();

        size_t offsets_length_aligned = PackedAllocatable::round_up_bytes_to_alignment_blocks(
                    (meta.offsets_size() + extra_size) * sizeof(DataSizeType)
        );

        size_t data_length_aligned = PackedAllocatable::round_up_bytes_to_alignment_blocks(
                    (extra_data_len + data_size) * sizeof(T)
        );

        return data_length_aligned + offsets_length_aligned;
    }

    VoidResult insert_space(size_t start, size_t extra_size, size_t extra_data_length)
    {
        auto& meta = pkd_buf_->metadata();

        using DataSizeType = size_t;

        size_t offsets_length = (meta.offsets_size() + extra_size) * sizeof(DataSizeType);

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(offsets_block_num(), offsets_length));

        size_t data_size = this->data_size(meta);
        size_t column_data_length = extra_data_length + data_size;

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(data_block_num(), column_data_length * sizeof(T)));

        size_t offsets_size = meta.offsets_size();
        auto offsets = this->offsets();
        auto data = this->data();

        size_t data_start  = offsets[start];
        size_t data_end    = data_start + extra_data_length;

        MemMoveBuffer(offsets + start, offsets + (start + extra_size), offsets_size - start);
        MemMoveBuffer(data + data_start, data + data_end, data_size - data_start);

        shift_offsets_right(start + extra_size, offsets_size + extra_size, extra_data_length);

        return VoidResult::of();
    }

    VoidResult remove_space(size_t start, size_t size)
    {
        auto& meta = pkd_buf_->metadata();

        size_t room_start = start;
        size_t room_end = start + size;

        size_t data_size = this->data_size(meta);
        size_t offsets_size = meta.offsets_size();

        auto offsets = this->offsets();
        auto data    = this->data();

        size_t data_start  = offsets[room_start];
        size_t data_end    = offsets[room_end];
        size_t data_length = data_end - data_start;

        shift_offsets_left(room_end, offsets_size, data_length);

        MemMoveBuffer(offsets + room_end, offsets + room_start, offsets_size - room_end);
        MemMoveBuffer(data + data_end, data + data_start, data_size - data_end);

        data_size -= data_length;

        size_t new_offsets_length = (meta.offsets_size() - size) * sizeof(DataSizeType);

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(offsets_block_num(), new_offsets_length));
        size_t column_data_length = data_size * sizeof(T);

        MEMORIA_TRY_VOID(pkd_buf_->resize_block(data_block_num(), column_data_length));

        return VoidResult::of();
    }

    VoidResult resize_row(size_t idx, const Span<const T>& value)
    {
        auto& meta = pkd_buf_->metadata();

        size_t current_value_size = this->length(idx);
        size_t new_value_size = value.length();

        if (current_value_size < new_value_size)
        {
            size_t size_delta = new_value_size - current_value_size;

            size_t data_size = this->data_size(meta);
            size_t column_data_length = size_delta + data_size;

            MEMORIA_TRY_VOID(pkd_buf_->resize_block(data_block_num(), column_data_length));

            auto offsets = this->offsets();
            auto data    = this->data();

            size_t data_start  = offsets[idx + 1];
            size_t data_end    = data_start + size_delta;


            MemMoveBuffer(data + data_start, data + data_end, data_size - data_start);

            size_t offsets_size = meta.offsets_size();
            shift_offsets_right(idx + 1, offsets_size, size_delta);
        }
        else if (current_value_size > new_value_size)
        {
            size_t size_delta = current_value_size - new_value_size;

            auto offsets = this->offsets();
            auto data    = this->data();

            size_t data_start  = offsets[idx + 1];
            size_t data_end    = data_start - size_delta;

            size_t data_size = this->data_size(meta);
            MemMoveBuffer(data + data_start, data + data_end, data_size - data_start);

            size_t offsets_size = meta.offsets_size();
            shift_offsets_left(idx + 1, offsets_size, size_delta);

            size_t column_data_length = data_size - size_delta;

            MEMORIA_TRY_VOID(pkd_buf_->resize_block(data_block_num(), column_data_length));
        }

        return VoidResult::of();
    }

    VoidResult replace_row(size_t idx, const Span<const T>& value)
    {
        auto* data = this->data();
        size_t offset = this->offsets()[idx];

        MemCpyBuffer(value.data(), data + offset, value.length());
        return VoidResult::of();
    }

    void copy_to(PkdStruct* other, size_t copy_from, size_t count, size_t copy_to, size_t data_length) const
    {
        auto other_dim = other->template dimension<Dimension>(column_);

        const DataSizeType* offsets = this->offsets();
        DataSizeType* other_offsets = other_dim.offsets();

        size_t other_data_start = other_offsets[copy_to];
        size_t data_start = offsets[copy_from];

        MemCpyBuffer(
            this->data() + data_start,
            other_dim.data() + other_data_start,
            data_length
        );

        size_t other_prefix_data_length = other_offsets[copy_to];
        size_t prefix_data_length = offsets[copy_from];

        for (size_t i = 0; i < count; i++)
        {
            other_offsets[copy_to + i] = offsets[copy_from + i] - prefix_data_length + other_prefix_data_length;
        }
    }

    template <typename Buffer>
    void copy_from_databuffer(size_t idx, size_t start, size_t size, size_t data_length, const Buffer& buffer)
    {
        const auto* offsets = buffer.template offsets<Dimension>(start);
        const auto* data_src = buffer.template data<Dimension>(start);

        auto* local_offsets = this->offsets();

        size_t local_offset_prefix  = local_offsets[idx];
        size_t buffer_offset_prefix = offsets[0];

        MemCpyBuffer(data_src, this->data() + local_offset_prefix, data_length);

        for (size_t c = 0; c < size; c++) {
            local_offsets[c + idx] = offsets[c] - buffer_offset_prefix + local_offset_prefix;
        }
    }


    void copy_from(size_t idx, const Span<const T>& span)
    {
        auto* offsets = this->offsets();

        size_t local_offset_prefix = offsets[idx];

        MemCpyBuffer(span.data(), this->data() + local_offset_prefix, span.length());

        offsets[idx + 1] = local_offset_prefix + span.length();
    }

    template <typename Metadata>
    size_t estimate_insert_upsize(const Span<const T>& span, const Metadata& meta) const
    {
        size_t column_data_size = this->data_size(meta);
        size_t view_length = span.length();

        size_t offsets_size = meta.offsets_size();

        size_t offsets_block_size_aligned0 = pkd_buf_->element_size(offsets_block_num());

        size_t offsets_block_size_aligned1 = PackedAllocatable::round_up_bytes_to_alignment_blocks(
            (offsets_size + 1) * sizeof(DataSizeType)
        );

        size_t column_data_size_aligned0 = pkd_buf_->element_size(data_block_num());

        size_t column_data_size_aligned1 = PackedAllocatable::round_up_bytes_to_alignment_blocks(
            (view_length + column_data_size) * sizeof(T)
        );

        return column_data_size_aligned1 - column_data_size_aligned0
                + offsets_block_size_aligned1 - offsets_block_size_aligned0;
    }


    template <typename Metadata>
    size_t estimate_replace_upsize(size_t idx, const Span<const T>& span, const Metadata& meta) const
    {
        size_t existing_value_length = this->length(idx);
        size_t new_value_length      = span.length();

        if (existing_value_length < new_value_length)
        {
            size_t column_data_size = this->data_size(meta);
            size_t column_data_size_aligned0 = pkd_buf_->element_size(offsets_block_num());

            size_t size_delta = new_value_length - existing_value_length;

            size_t column_data_size_aligned1 = PackedAllocatable::round_up_bytes_to_alignment_blocks(
                        size_delta + column_data_size
            );

            return column_data_size_aligned1 - column_data_size_aligned0;
        }

        return 0;
    }

    template <typename SerializationData, typename Metadata>
    void serialize(const Metadata& meta, SerializationData& buf) const
    {
        FieldFactory<DataSizeType>::serialize(buf, offsets(), meta.offsets_size());
        FieldFactory<T>::serialize(buf, data(), this->data_size(meta));
    }

    template <typename DeserializationData, typename Metadata>
    void deserialize(const Metadata& meta, DeserializationData& buf)
    {
        FieldFactory<DataSizeType>::deserialize(buf, offsets(), meta.offsets_size());
        FieldFactory<T>::deserialize(buf, data(), this->data_size(meta));
    }


private:
    void shift_offsets_right(size_t start, size_t end, size_t shift_length) {
        DataSizeType* offsets = this->offsets();
        for (size_t c = start; c < end; c++){
            offsets[c] += shift_length;
        }
    }

    void shift_offsets_left(size_t start, size_t end, size_t shift_length) {
        DataSizeType* offsets = this->offsets();
        for (size_t c = start; c < end; c++){
            offsets[c] -= shift_length;
        }
    }
};


}}
