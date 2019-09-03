
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
#include <memoria/v1/core/tools/assert.hpp>

#include <memoria/v1/profiles/common/block_operations.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/v1/core/packed/array/packed_vle_array_iterator.hpp>

#include <memoria/v1/core/iovector/io_substream_array_vlen_base.hpp>

#include <memoria/v1/core/tools/span.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

template <typename ExtData, typename PkdStruct>
class PackedVLenElementArraySO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedVLenElementArraySO;

public:

    using Value         = typename PkdStruct::Value;
    using DataType      = typename PkdStruct::DataType;
    using DataSizeType  = typename PkdStruct::DataSizeType;
    using DataAtomType  = typename PkdStruct::DataAtomType;
    using Accessor      = typename PkdStruct::Accessor;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    template <typename, typename, typename>
    friend struct PkdDataTypeAccessor;

    using PkdStructT = PkdStruct;

    static constexpr psize_t Columns = PkdStruct::Blocks;
    static constexpr int32_t Indexes = 0;


    PackedVLenElementArraySO(): ext_data_(), data_() {}
    PackedVLenElementArraySO(const ExtData* ext_data, PkdStruct* data):
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

    ConstIterator begin() const {
        return Iterator(Accessor(this), 0, data_->size());
    }

    ConstIterator end() const
    {
        psize_t size = data_->size();
        return Iterator(Accessor(this), size, size);
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const {return ext_data_;}
    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    psize_t length(psize_t column, psize_t row) const
    {
        return data_->length(column, row);
    }

    psize_t length(psize_t column, psize_t row, psize_t size) const
    {
        return data_->length(column, row + size) - data_->length(column, row);
    }

    DataAtomType* data(psize_t column, psize_t row) {
        return data_->data(column) + row;
    }

    const DataAtomType* data(psize_t column, psize_t row) const {
        return data_->data(column) + row;
    }

    const DataAtomType* data_for(psize_t column, psize_t row) const {
        return data_->data(column) + data_->offsets(column)[row];
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        const auto& meta = data_->metadata();

        handler->startStruct();
        handler->startGroup("FX_SIZE_EL_ARRAY");

        handler->value("SIZE",          &meta.size());
        handler->value("DATA_SIZE",     &meta.data_size(0));

        handler->startGroup("DATA", meta.size());

        for (int32_t c = 0; c < meta.size(); c++)
        {
            handler->value("VALUES", BlockValueProviderFactory::provider(Columns, [&](int32_t idx) {
                return access(idx, c);
            }));
        }

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

    void check() const {}

    psize_t size() const {
        return data_->metadata().size();
    }

    Value access(psize_t column, psize_t row) const
    {
        Accessor accessor(*this, column);
        return accessor.get(row);
    }

    /*********************** API *********************/

    OpStatus copyTo(MyType& other, psize_t copy_from, psize_t count, psize_t copy_to, Span<const psize_t> data_lengths) const
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            const DataSizeType* offsets = data_->offsets(c);
            DataSizeType* other_offsets = other.data()->offsets(c);

            psize_t other_data_start = other_offsets[copy_to];
            psize_t data_start = offsets[copy_from];

            MemCpyBuffer(
                data_->data(c) + data_start,
                other.data()->data(c) + other_data_start,
                data_lengths[c]
            );

            psize_t other_prefix_data_lengths[Columns];
            other.compute_data_lengths(0, copy_to, other_prefix_data_lengths);

            psize_t prefix_data_lengths[Columns];
            compute_data_lengths(0, copy_from, prefix_data_lengths);

            for (psize_t i = 0; i < count; i++)
            {
                other_offsets[copy_to + i] = offsets[copy_from + i] - prefix_data_lengths[c] + other_prefix_data_lengths[c];
            }
        }

        return OpStatus::OK;
    }


    OpStatus splitTo(MyType& other, psize_t idx)
    {
        auto& meta = data_->metadata();

        MEMORIA_V1_ASSERT(other.size(), ==, 0);

        psize_t data_lengths[Columns];
        compute_data_lengths(idx, meta.size(), data_lengths);

        psize_t split_size = meta.size() - idx;
        if(isFail(other.insertSpace(0, split_size, data_lengths))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, idx, split_size, 0, data_lengths))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(idx, meta.size()))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus mergeWith(MyType& other)
    {
        psize_t my_size     = this->size();
        psize_t other_size  = other.size();

        psize_t data_lengths[Columns];
        compute_data_lengths(0, other_size, data_lengths);

        if(isFail(other.insertSpace(other_size, my_size, data_lengths))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, 0, my_size, other_size, data_lengths))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(0, my_size))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus removeSpace(psize_t room_start, psize_t room_end)
    {
        auto& meta = data_->metadata();
        psize_t size = meta.size();
        psize_t room_length = room_end - room_start;

        MEMORIA_V1_ASSERT(room_start, <=, size);
        MEMORIA_V1_ASSERT(room_end, <=, size);
        MEMORIA_V1_ASSERT(room_start, <=, room_end);

        psize_t data_lengths[Columns];

        psize_t offsets_size = meta.offsets_size();

        for (psize_t c = 0; c < Columns; c++)
        {
            auto offsets = data_->offsets(c);
            auto data    = data_->data(c);

            psize_t data_start  = offsets[room_start];
            psize_t data_end    = offsets[room_end];
            data_lengths[c]     = data_end - data_start;

            shift_offsets_left(c, room_end, offsets_size, data_lengths[c]);

            MemMoveBuffer(offsets + room_end, offsets + room_start, offsets_size - room_length);
            MemMoveBuffer(data + data_end, data + data_start, meta.data_size(c) - data_end);
        }

        psize_t new_offsets_length = (meta.offsets_size() - room_length) * sizeof(DataSizeType);
        for (psize_t c = 0; c < Columns; c++)
        {
            if(isFail(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock, new_offsets_length))) {
                return OpStatus::FAIL;
            }

            psize_t column_data_length = meta.data_size(c) - data_lengths[c];

            if(isFail(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock + 1, column_data_length))) {
                return OpStatus::FAIL;
            }
        }

        meta.size() -= room_length;

        return OpStatus::OK;
    }

    Value get_values(psize_t idx) const {
        return get_values(0, idx);
    }

    Value get_values(psize_t idx, psize_t column) const {
        return Accessor::get(*this, column, idx);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& view = io::substream_cast<typename PkdStruct::IOSubstreamView>(substream);
        view.configure(*this);
    }

    OpStatusT<int32_t> insert_io_substream(psize_t at, const io::IOSubstream& substream, psize_t start, psize_t size)
    {
        const io::IOVLen1DArraySubstream<DataType>& buffer
                = io::substream_cast<io::IOVLen1DArraySubstream<DataType>>(substream);

        psize_t length = buffer.length(start, size);

        if(isFail(insertSpace(at, size, Span<const psize_t>(&length, 1)))) {
            return OpStatus::FAIL;
        }

        const auto* offsets = buffer.offsets(start);
        const auto* data_src = buffer.data(offsets[0]);

        auto* local_offsets = data_->offsets(0);

        psize_t local_offset_prefix  = local_offsets[at];
        psize_t buffer_offset_prefix = buffer.offsets(start)[0];

        MemCpyBuffer(data_src, data_->data(0) + local_offset_prefix, length);

        for (psize_t c = 0; c < size; c++)
        {
            local_offsets[c] = offsets[c] - buffer_offset_prefix + local_offset_prefix;
        }

        return OpStatusT<int32_t>(at + size);
    }



    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        if (isFail(removeSpace(pos, pos + 1))) {
            return OpStatus::FAIL;
        }

        return _insert_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        if (isFail(Accessor::insert(*this, pos, 1, [&](psize_t column, psize_t row){
                return val(column);
        }))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        return removeSpace(idx, idx + 1);
    }

private:

    void compute_data_lengths(psize_t start, psize_t end, psize_t* lengths) const
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            auto offsets = data_->offsets(c);
            lengths[c] = offsets[end] - offsets[start];
        }
    }

    void shift_offsets_right(psize_t column, psize_t start, psize_t end, psize_t shift_length)
    {
        DataSizeType* offsets = data_->offsets(column);
        for (psize_t c = start; c < end; c++){
            offsets[c] += shift_length;
        }
    }

    void shift_offsets_left(psize_t column, psize_t start, psize_t end, psize_t shift_length)
    {
        DataSizeType* offsets = data_->offsets(column);
        for (psize_t c = start; c < end; c++){
            offsets[c] -= shift_length;
        }
    }


    OpStatus insertSpace(psize_t idx, psize_t room_length, Span<const psize_t> data_lengths)
    {
        auto& meta = data_->metadata();

        psize_t size = meta.size();


        MEMORIA_V1_ASSERT(idx, <=, size);
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(room_length, >=, 0);


        psize_t offsets_length = (meta.offsets_size() + room_length) * sizeof(DataSizeType);

        psize_t offsets_length_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            (meta.offsets_size() + room_length) * sizeof(DataSizeType)
        );

        psize_t data_length_aligned{};

        for (psize_t c = 0; c < Columns; c++)
        {
            data_length_aligned += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                data_lengths[c] + meta.data_size(c)
            );
        }

        if(isFail(data_->resize(PkdStruct::empty_size() + data_length_aligned + offsets_length_aligned * Columns))) {
            return OpStatus::FAIL;
        }

        for (psize_t c = 0; c < Columns; c++)
        {
            if(isFail(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock, offsets_length))) {
                return OpStatus::FAIL;
            }

            psize_t column_data_length = data_lengths[c] + meta.data_size(c);

            if(isFail(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock + 1, column_data_length))) {
                return OpStatus::FAIL;
            }
        }

        psize_t offsets_size = meta.offsets_size();

        for (psize_t c = 0; c < Columns; c++)
        {
            auto offsets = data_->offsets(c);
            auto data = data_->data(c);

            psize_t data_start  = offsets[idx];
            psize_t data_end    = data_start + data_lengths[c];

            MemMoveBuffer(offsets + idx, offsets + (idx + room_length), offsets_size - idx);
            MemMoveBuffer(data + data_start, data + data_end, meta.data_size(c) - data_start);

            shift_offsets_right(c, idx + room_length, offsets_size + room_length, data_lengths[c]);

            meta.data_size(c) += data_lengths[c];
        }

        meta.size() += room_length;

        return OpStatus::OK;
    }

};


}
}
