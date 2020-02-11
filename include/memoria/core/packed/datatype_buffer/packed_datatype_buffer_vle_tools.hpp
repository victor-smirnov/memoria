
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
#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <type_traits>


namespace memoria {
namespace pdtbuf_ {


template <typename T, typename PkdStruct, psize_t Block_>
class PDTDimension<Span<const T>, PkdStruct, Block_> {
    PkdStruct* pkd_buf_;

    using DataSizeType = psize_t;

public:
    static constexpr psize_t Block = Block_;
    static constexpr psize_t Width = 2;

    static constexpr psize_t DataBlock = Block_;
    static constexpr psize_t OffsetsBlock = Block_ + 1;
    static constexpr psize_t Dimension = Block_ - 1;


    PDTDimension(PkdStruct* pkd_buf):
        pkd_buf_(pkd_buf)
    {}

    T* data() {
        return pkd_buf_->template get<T>(DataBlock);
    }

    const T* data() const {
        return pkd_buf_->template get<T>(DataBlock);
    }

    psize_t* offsets() {
        return pkd_buf_->template get<psize_t>(OffsetsBlock);
    }

    const psize_t* offsets() const {
        return pkd_buf_->template get<psize_t>(OffsetsBlock);
    }

    static VoidResult allocateEmpty(PkdStruct* alloc) noexcept
    {
        MEMORIA_TRY(offsets, alloc->template allocateArrayBySize<psize_t>(OffsetsBlock, 1));

        offsets[0] = 0;

        return VoidResult::of();
    }

    static psize_t data_block_size(psize_t capacity)
    {
        return 0;
    }

    template <typename Metadata>
    static void init_metadata(Metadata& metadata) {
        metadata.data_size(Dimension) = 0;
    }

    static constexpr psize_t empty_size_aligned() {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(psize_t));
    }

    template <typename Metadata>
    psize_t joint_data_length(const Metadata& my_meta, const PkdStruct* other, const Metadata& other_meta) const
    {
        psize_t data_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
             (my_meta.data_size(Dimension) + other_meta.data_size(Dimension)) * sizeof(T)
        );

        psize_t offsets_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
             (my_meta.offsets_size() + other_meta.offsets_size() - 1) * sizeof (DataSizeType)
        );

        return data_size + offsets_size;
    }

    void set(Span<const T>& span, psize_t row) const
    {
        const psize_t* offs = offsets();
        const T* dd = data();
        span = Span<const T>(dd + offs[row], offs[row + 1] - offs[row]);
    }

    void lengths(psize_t& value, psize_t row, psize_t size) const
    {
        const psize_t* offs = offsets();
        value = offs[row + size] - offs[row];
    }

    psize_t length(psize_t row, psize_t size) const
    {
        const psize_t* offs = offsets();
        return offs[row + size] - offs[row];
    }

    psize_t length(psize_t row) const
    {
        const psize_t* offs = offsets();
        return offs[row + 1] - offs[row];
    }



    psize_t compute_new_size(psize_t extra_size, psize_t extra_data_len)
    {
        auto meta = pkd_buf_->metadata();

        psize_t offsets_length_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    (meta.offsets_size() + extra_size) * sizeof(DataSizeType)
        );

        psize_t data_length_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    (extra_data_len + meta.data_size(Dimension)) * sizeof(T)
        );

        return data_length_aligned + offsets_length_aligned;
    }

    VoidResult insert_space(psize_t start, psize_t extra_size, psize_t extra_data_length) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        using DataSizeType = psize_t;

        psize_t offsets_length = (meta.offsets_size() + extra_size) * sizeof(DataSizeType);

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(OffsetsBlock, offsets_length));

        psize_t column_data_length = extra_data_length + meta.data_size(Dimension);

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length * sizeof(T)));

        psize_t offsets_size = meta.offsets_size();


        auto offsets = this->offsets();
        auto data = this->data();

        psize_t data_start  = offsets[start];
        psize_t data_end    = data_start + extra_data_length;

        MemMoveBuffer(offsets + start, offsets + (start + extra_size), offsets_size - start);
        MemMoveBuffer(data + data_start, data + data_end, meta.data_size(Dimension) - data_start);

        shift_offsets_right(start + extra_size, offsets_size + extra_size, extra_data_length);

        meta.data_size(Dimension) += extra_data_length;

        return VoidResult::of();
    }

    VoidResult remove_space(psize_t start, psize_t size) noexcept
    {
        auto& meta = pkd_buf_->metadata();

        psize_t room_start = start;
        psize_t room_end = start + size;

        psize_t offsets_size = meta.offsets_size();

        auto offsets = this->offsets();
        auto data    = this->data();

        psize_t data_start  = offsets[room_start];
        psize_t data_end    = offsets[room_end];
        psize_t data_length = data_end - data_start;

        shift_offsets_left(room_end, offsets_size, data_length);

        MemMoveBuffer(offsets + room_end, offsets + room_start, offsets_size - room_end);
        MemMoveBuffer(data + data_end, data + data_start, meta.data_size(Dimension) - data_end);

        meta.data_size(Dimension) -= data_length;


        psize_t new_offsets_length = (meta.offsets_size() - size) * sizeof(DataSizeType);

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(OffsetsBlock, new_offsets_length));

        psize_t column_data_length = meta.data_size(Dimension) * sizeof(T);

        MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length));

        return VoidResult::of();
    }

    VoidResult resize_row(psize_t idx, const Span<const T>& value)
    {
        auto& meta = pkd_buf_->metadata();

        psize_t current_value_size = this->length(idx);
        psize_t new_value_size = value.length();

        if (current_value_size < new_value_size)
        {
            psize_t size_delta = new_value_size - current_value_size;

            psize_t column_data_length = size_delta + meta.data_size(Dimension);

            MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length));

            auto offsets = this->offsets();
            auto data    = this->data();

            psize_t data_start  = offsets[idx + 1];
            psize_t data_end    = data_start + size_delta;

            MemMoveBuffer(data + data_start, data + data_end, meta.data_size(Dimension) - data_start);

            psize_t offsets_size = meta.offsets_size();
            shift_offsets_right(idx + 1, offsets_size, size_delta);

            meta.data_size(Dimension) += size_delta;
        }
        else if (current_value_size > new_value_size)
        {
            psize_t size_delta = current_value_size - new_value_size;

            auto offsets = this->offsets();
            auto data    = this->data();

            psize_t data_start  = offsets[idx + 1];
            psize_t data_end    = data_start - size_delta;

            MemMoveBuffer(data + data_start, data + data_end, meta.data_size(Dimension) - data_start);

            psize_t offsets_size = meta.offsets_size();
            shift_offsets_left(idx + 1, offsets_size, size_delta);

            psize_t column_data_length = meta.data_size(Dimension) - size_delta;

            MEMORIA_TRY_VOID(pkd_buf_->resizeBlock(DataBlock, column_data_length));

            meta.data_size(Dimension) -= size_delta;
        }

        return VoidResult::of();
    }

    VoidResult replace_row(psize_t idx, const Span<const T>& value) noexcept
    {
        auto* data = this->data();
        psize_t offset = this->offsets()[idx];

        MemCpyBuffer(value.data(), data + offset, value.length());
        return VoidResult::of();
    }

    void copy_to(PkdStruct* other, psize_t copy_from, psize_t count, psize_t copy_to, psize_t data_length) const
    {
        auto other_dim = other->template dimension<Dimension>();

        const DataSizeType* offsets = this->offsets();
        DataSizeType* other_offsets = other_dim.offsets();

        psize_t other_data_start = other_offsets[copy_to];
        psize_t data_start = offsets[copy_from];

        MemCpyBuffer(
            this->data() + data_start,
            other_dim.data() + other_data_start,
            data_length
        );

        psize_t other_prefix_data_length = other_offsets[copy_to];
        psize_t prefix_data_length = offsets[copy_from];

        for (psize_t i = 0; i < count; i++)
        {
            other_offsets[copy_to + i] = offsets[copy_from + i] - prefix_data_length + other_prefix_data_length;
        }
    }

    template <typename Buffer>
    void copy_from_databuffer(psize_t idx, psize_t start, psize_t size, psize_t data_length, const Buffer& buffer)
    {
        const auto* offsets = buffer.template offsets<Dimension>(start);
        const auto* data_src = buffer.template data<Dimension>(start);

        auto* local_offsets = this->offsets();

        psize_t local_offset_prefix  = local_offsets[idx];
        psize_t buffer_offset_prefix = offsets[0];

        MemCpyBuffer(data_src, this->data() + local_offset_prefix, data_length);

        for (psize_t c = 0; c < size; c++)
        {
            local_offsets[c + idx] = offsets[c] - buffer_offset_prefix + local_offset_prefix;
        }
    }


    void copy_from(psize_t idx, const Span<const T>& span)
    {
        auto* offsets = this->offsets();

        psize_t local_offset_prefix = offsets[idx];

        MemCpyBuffer(span.data(), this->data() + local_offset_prefix, span.length());

        offsets[idx + 1] = local_offset_prefix + span.length();
    }

    template <typename Metadata>
    psize_t estimate_insert_upsize(const Span<const T>& span, const Metadata& meta) const
    {
        psize_t column_data_size = meta.data_size(Dimension);
        psize_t view_length = span.length();

        psize_t offsets_size = meta.offsets_size();

        psize_t offsets_block_size_aligned0 = pkd_buf_->element_size(OffsetsBlock);

        psize_t offsets_block_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            (offsets_size + 1) * sizeof(DataSizeType)
        );

        psize_t column_data_size_aligned0 = pkd_buf_->element_size(DataBlock);

        psize_t column_data_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            (view_length + column_data_size) * sizeof(T)
        );

        return column_data_size_aligned1 - column_data_size_aligned0
                + offsets_block_size_aligned1 - offsets_block_size_aligned0;
    }


    template <typename Metadata>
    psize_t estimate_replace_upsize(psize_t idx, const Span<const T>& span, const Metadata& meta) const
    {
        psize_t existing_value_length = this->length(idx);
        psize_t new_value_length      = span.length();

        if (existing_value_length < new_value_length)
        {
            psize_t column_data_size = meta.data_size(Dimension);
            psize_t column_data_size_aligned0 = pkd_buf_->element_size(OffsetsBlock);

            psize_t size_delta = new_value_length - existing_value_length;

            psize_t column_data_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                        size_delta + column_data_size
            );

            return column_data_size_aligned1 - column_data_size_aligned0;
        }

        return 0;
    }

    template <typename SerializationData, typename Metadata>
    VoidResult serialize(const Metadata& meta, SerializationData& buf) const noexcept
    {
        FieldFactory<psize_t>::serialize(buf, offsets(), meta.offsets_size());
        FieldFactory<T>::serialize(buf, data(), meta.data_size(Dimension));

        return VoidResult::of();
    }

    template <typename DeserializationData, typename Metadata>
    VoidResult deserialize(const Metadata& meta, DeserializationData& buf) noexcept
    {
        FieldFactory<psize_t>::deserialize(buf, offsets(), meta.offsets_size());
        FieldFactory<T>::deserialize(buf, data(), meta.data_size(Dimension));

        return VoidResult::of();
    }


private:
    void shift_offsets_right(psize_t start, psize_t end, psize_t shift_length)
    {
        psize_t* offsets = this->offsets();
        for (psize_t c = start; c < end; c++){
            offsets[c] += shift_length;
        }
    }

    void shift_offsets_left(psize_t start, psize_t end, psize_t shift_length)
    {
        psize_t* offsets = this->offsets();
        for (psize_t c = start; c < end; c++){
            offsets[c] -= shift_length;
        }
    }
};


}}
