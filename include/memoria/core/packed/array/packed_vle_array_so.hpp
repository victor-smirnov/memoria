
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
#include <memoria/core/tools/assert.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <memoria/core/packed/array/packed_array_iterator.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/datatypes/buffer/buffer_generic.hpp>

#include <algorithm>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedVLenElementArraySO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedVLenElementArraySO;

public:

    using ViewType      = typename PkdStruct::ViewType;
    using DataType      = typename PkdStruct::DataType;
    using DataSizeType  = typename PkdStruct::DataSizeType;
    using DataAtomType  = typename PkdStruct::DataAtomType;
    using Accessor      = typename PkdStruct::Accessor;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    template <typename, typename, typename, typename>
    friend struct PkdDataTypeAccessor;

    using PkdStructT = PkdStruct;

    static constexpr psize_t Columns = PkdStruct::Blocks;
    static constexpr int32_t Indexes = PkdStruct::Indexes;


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

    ConstIterator begin(psize_t column) const {
        return ConstIterator(Accessor(*this, column), 0, data_->size());
    }

    ConstIterator end(psize_t column) const
    {
        psize_t size = data_->size();
        return ConstIterator(Accessor(*this, column), size, size);
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const ExtData* ext_data() const {return ext_data_;}
    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    VoidResult reindex() noexcept {
        return VoidResult::of();
    }

    psize_t length(psize_t column, psize_t row) const
    {
        return data_->length(column, row);
    }

    psize_t length(psize_t column, psize_t row, psize_t size) const
    {
        const auto* offsets = data_->offsets(column);
        return offsets[row + size] - offsets[row];
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


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        const auto& meta = data_->metadata();

        handler->startStruct();
        handler->startGroup("VLE_ARRAY");

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

        return VoidResult::of();
    }

    void check() const {}

    psize_t size() const {
        return data_->metadata().size();
    }

    ViewType access(psize_t column, psize_t row) const
    {
        Accessor accessor(*this, column);
        return accessor.get(row);
    }

    /*********************** API *********************/

    VoidResult copyTo(MyType& other, psize_t copy_from, psize_t count, psize_t copy_to, Span<const psize_t> data_lengths) const noexcept
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

        return VoidResult::of();
    }


    VoidResult splitTo(MyType& other, psize_t idx) noexcept
    {
        auto& meta = data_->metadata();

        MEMORIA_V1_ASSERT(other.size(), ==, 0);

        psize_t data_lengths[Columns];
        compute_data_lengths(idx, meta.size(), data_lengths);

        psize_t split_size = meta.size() - idx;
        MEMORIA_TRY_VOID(other.insertSpace(0, split_size, data_lengths));

        MEMORIA_TRY_VOID(copyTo(other, idx, split_size, 0, data_lengths));

        MEMORIA_TRY_VOID(removeSpace(idx, meta.size()));

        return VoidResult::of();
    }

    VoidResult mergeWith(MyType& other) noexcept
    {
        psize_t my_size     = this->size();
        psize_t other_size  = other.size();

        psize_t data_lengths[Columns];
        compute_data_lengths(0, other_size, data_lengths);

        MEMORIA_TRY_VOID(other.insertSpace(other_size, my_size, data_lengths));

        MEMORIA_TRY_VOID(copyTo(other, 0, my_size, other_size, data_lengths));

        MEMORIA_TRY_VOID(removeSpace(0, my_size));

        return VoidResult::of();
    }

    VoidResult removeSpace(psize_t room_start, psize_t room_end) noexcept
    {
        auto& meta = data_->metadata();
        psize_t size = meta.size();
        psize_t room_length = room_end - room_start;

        MEMORIA_V1_ASSERT_RTN(room_start, <=, size);
        MEMORIA_V1_ASSERT_RTN(room_end, <=, size);
        MEMORIA_V1_ASSERT_RTN(room_start, <=, room_end);

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

            meta.data_size(c) -= data_lengths[c];
        }

        psize_t new_offsets_length = (meta.offsets_size() - room_length) * sizeof(DataSizeType);
        for (psize_t c = 0; c < Columns; c++)
        {
            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock, new_offsets_length));

            psize_t column_data_length = meta.data_size(c);

            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock + 1, column_data_length));
        }

        meta.size() -= room_length;

        return VoidResult::of();
    }

    ViewType get_values(psize_t idx) const {
        return get_values(idx, 0);
    }

    ViewType get_values(psize_t idx, psize_t column) const {
        return Accessor::get(*this, column, idx);
    }


    template <typename T>
    VoidResult setValues(psize_t pos, const core::StaticVector<T, Columns>& values) noexcept
    {
        MEMORIA_TRY_VOID(removeSpace(pos, pos + 1));

        return insert(pos, values);
    }

    template <typename T>
    VoidResult insert(psize_t pos, const core::StaticVector<T, Columns>& values) noexcept
    {
        MEMORIA_TRY_VOID(Accessor::insert(*this, pos, Columns, [&](psize_t column, psize_t row){
                return values[column];
        }));

        return VoidResult::of();
    }

    ViewType max(psize_t column) const
    {
        auto size = this->size();

        if (size > 0)
        {
            return access(column, size - 1);
        }
        else {
            MMA_THROW(RuntimeException()) << format_ex("Column {} is empty", column);
        }
    }

    template <typename T>
    void max(core::StaticVector<T, Columns>& accum) const
    {
        psize_t size = data_->size();

        if (size > 0)
        {
            for (psize_t column = 0; column < Columns; column++)
            {
                accum[column] = access(column, size - 1);
            }
        }
        else {
            for (psize_t column = 0; column < Columns; column++)
            {
                accum[column] = T{};
            }
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t column = 0; column < Columns; column++)
        {
            accum[column] = this->max(column);
        }
    }


    void configure_io_substream(io::IOSubstream& substream) const
    {
        auto& view = io::substream_cast<typename PkdStruct::IOSubstreamView>(substream);
        view.configure(*this);
    }

    Int32Result insert_io_substream(psize_t at, const io::IOSubstream& substream, psize_t start, psize_t size) noexcept
    {
        static_assert(Columns == 1, "");

        using IOBuffer = typename PkdStruct::GrowableIOSubstream;

        const IOBuffer& buffer
                = io::substream_cast<IOBuffer>(substream);

        psize_t length = buffer.template data_length<0>(start, size);

        MEMORIA_TRY_VOID(insertSpace(at, size, Span<const psize_t>(&length, 1)));

        const auto* offsets = buffer.template offsets<0>(start);
        const auto* data_src = buffer.template data<0>(start);

        auto* local_offsets = data_->offsets(0);

        psize_t local_offset_prefix  = local_offsets[at];
        psize_t buffer_offset_prefix = offsets[0];

        MemCpyBuffer(data_src, data_->data(0) + local_offset_prefix, length);

        for (psize_t c = 0; c < size; c++)
        {
            local_offsets[c + at] = offsets[c] - buffer_offset_prefix + local_offset_prefix;
        }

        return Int32Result::of(at + size);
    }



    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    VoidResult _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val) noexcept
    {
        MEMORIA_TRY_VOID(removeSpace(pos, pos + 1));

        return _insert_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    VoidResult _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val) noexcept
    {
        MEMORIA_TRY_VOID(Accessor::insert(*this, pos, 1, [&](psize_t column, psize_t row){
                return val(column);
        }));

        return VoidResult::of();
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    VoidResult _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum) noexcept
    {
        return removeSpace(idx, idx + 1);
    }





    class FindResult {
        int32_t idx_;
    public:
        FindResult(int32_t idx): idx_(idx)
        {}

        FindResult(): idx_()
        {}

        ViewType prefix() {return ViewType{};}
        int32_t local_pos() const {return idx_;}
    };

    FindResult findGTForward(psize_t column, const ViewType& val) const
    {
        auto end = this->end(column);
        auto ii = std::upper_bound(begin(column), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
    }

    FindResult findGEForward(psize_t column, const ViewType& val) const
    {
        auto end = this->end(column);
        auto ii = std::lower_bound(begin(column), end, val);

        if (ii != end)
        {
            return ii.pos();
        }

        return FindResult(data_->size());
    }

    auto findForward(SearchType search_type, psize_t column, const ViewType& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val);
        }
        else {
            return findGEForward(column, val);
        }
    }

    auto findForward(SearchType search_type, psize_t column, const Optional<ViewType>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val.get());
        }
        else {
            return findGEForward(column, val.get());
        }
    }

    psize_t estimate_insert_upsize(const ViewType& view) const
    {
        static_assert(Columns == 1, "");

        const auto& meta = data_->metadata();

        psize_t column_data_size = meta.data_size(0);
        psize_t view_length = Accessor::length(view);

        psize_t offsets_size = meta.offsets_size();

        psize_t offsets_block_size_aligned0 = data_->element_size(PkdStruct::DATA + 1);

        psize_t offsets_block_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            (offsets_size + 1) * sizeof(psize_t)
        );

        psize_t column_data_size_aligned0 = data_->element_size(PkdStruct::DATA);

        psize_t column_data_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            view_length + column_data_size
        );

        return column_data_size_aligned1 - column_data_size_aligned0
                + offsets_block_size_aligned1 - offsets_block_size_aligned0;
    }

    psize_t estimate_replace_upsize(psize_t idx, const ViewType& view) const
    {
        static_assert(Columns == 1, "");

        psize_t existing_value_length = data_->length(0, idx);
        psize_t new_value_length      = Accessor::length(view);

        if (existing_value_length < new_value_length)
        {
            const auto& meta = data_->metadata();

            psize_t column_data_size = meta.data_size(0);
            psize_t column_data_size_aligned0 = data_->element_size(PkdStruct::DATA);

            psize_t size_delta = new_value_length - existing_value_length;

            psize_t column_data_size_aligned1 = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                size_delta + column_data_size
            );

            return column_data_size_aligned1 - column_data_size_aligned0;
        }

        return 0;
    }

    VoidResult replace(psize_t column, psize_t idx, const ViewType& view) noexcept
    {
        psize_t length = Accessor::length(view);

        MEMORIA_TRY_VOID(resize(column, idx, length));

        psize_t offset = data_->offsets(column)[idx];
        auto* data = data_->data(column) + offset;

        MemCpyBuffer(Accessor::data(view), data, length);

        return VoidResult::of();
    }

    VoidResult insert(psize_t idx, const ViewType& view) noexcept
    {
        static_assert(Columns == 1, "");

        core::StaticVector<ViewType, 1> vv;
        vv[0] = view;

        return insert(idx, vv);
    }

    VoidResult remove(psize_t idx)
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


    VoidResult insertSpace(psize_t idx, psize_t room_length, Span<const psize_t> data_lengths) noexcept
    {
        auto& meta = data_->metadata();

        psize_t size = meta.size();

        MEMORIA_V1_ASSERT_RTN(idx, <=, size);
        MEMORIA_V1_ASSERT_RTN(idx, >=, 0);
        MEMORIA_V1_ASSERT_RTN(room_length, >=, 0);


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

        MEMORIA_TRY_VOID(data_->resize(PkdStruct::empty_size() + data_length_aligned + offsets_length_aligned * Columns));

        for (psize_t c = 0; c < Columns; c++)
        {
            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock, offsets_length));

            psize_t column_data_length = data_lengths[c] + meta.data_size(c);

            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + c * PkdStruct::SegmentsPerBlock + 1, column_data_length));
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

        return VoidResult::of();
    }


    VoidResult resize(psize_t column, psize_t idx, psize_t new_value_size) noexcept
    {
        auto& meta = data_->metadata();

        psize_t size = meta.size();

        MEMORIA_V1_ASSERT_RTN(idx, <=, size);
        MEMORIA_V1_ASSERT_RTN(idx, >=, 0);

        psize_t offsets_length_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(
            meta.offsets_size() * sizeof(DataSizeType)
        );

        psize_t data_length_aligned{};

        psize_t current_value_size = data_->length(column, idx);

        if (current_value_size < new_value_size)
        {
            psize_t size_delta = new_value_size - current_value_size;

            psize_t data_lengths[Columns] = {};
            data_lengths[column] = size_delta;

            for (psize_t c = 0; c < Columns; c++)
            {
                data_length_aligned += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    data_lengths[c] + meta.data_size(c)
                );
            }

            MEMORIA_TRY_VOID(data_->resize(PkdStruct::empty_size() + data_length_aligned + offsets_length_aligned * Columns));

            psize_t column_data_length = size_delta + meta.data_size(column);

            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + column * PkdStruct::SegmentsPerBlock + 1, column_data_length));

            auto offsets = data_->offsets(column);
            auto data    = data_->data(column);

            psize_t data_start  = offsets[idx + 1];
            psize_t data_end    = data_start + size_delta;

            MemMoveBuffer(data + data_start, data + data_end, meta.data_size(column) - data_start);

            psize_t offsets_size = meta.offsets_size();
            shift_offsets_right(column, idx + 1, offsets_size, size_delta);

            meta.data_size(column) += size_delta;
        }
        else if (current_value_size > new_value_size)
        {
            psize_t size_delta = current_value_size - new_value_size;

            auto offsets = data_->offsets(column);
            auto data    = data_->data(column);

            psize_t data_start  = offsets[idx + 1];
            psize_t data_end    = data_start - size_delta;

            MemMoveBuffer(data + data_start, data + data_end, meta.data_size(column) - data_start);

            psize_t offsets_size = meta.offsets_size();
            shift_offsets_left(column, idx + 1, offsets_size, size_delta);

            psize_t data_lengths[Columns] = {};
            data_lengths[column] = size_delta;

            for (psize_t c = 0; c < Columns; c++)
            {
                data_length_aligned += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    meta.data_size(c) - data_lengths[c]
                );
            }

            MEMORIA_TRY_VOID(data_->resize(PkdStruct::empty_size() + data_length_aligned + offsets_length_aligned * Columns));

            psize_t column_data_length = meta.data_size(column) - size_delta;

            MEMORIA_TRY_VOID(data_->resizeBlock(PkdStruct::DATA + column * PkdStruct::SegmentsPerBlock + 1, column_data_length));

            meta.data_size(column) -= size_delta;
        }

        return VoidResult::of();
    }
};


template <typename DataType, typename ExtData, typename PkdStruct>
class IO1DArraySubstreamViewImpl<
        DataType,
        PackedVLenElementArraySO<ExtData, PkdStruct>
>: public io::IO1DArraySubstreamView<DataType> {

    using Base = io::IO1DArraySubstreamView<DataType>;
    using ArraySO = PackedVLenElementArraySO<ExtData, PkdStruct>;

    using typename Base::ViewType;

    static_assert(ArraySO::Columns == 1, "");

    ArraySO array_{};

public:
    void configure(ArraySO array)
    {
        array_ = array;
    }

    size_t size() const {
        return array_.size();
    }

    void read_to(size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const
    {
        auto ii = array_.begin(0);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append_value(*ii);
        }
    }

    void read_to(size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const
    {
        auto ii = array_.begin(0);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer.append(*ii);
        }
    }

    void read_to(size_t row, size_t size, Span<ViewType> buffer) const
    {
        auto ii = array_.begin(0);
        ii += row;

        for (size_t c = 0; c < size; c++, ++ii) {
            buffer[c] = (*ii);
        }
    }

    virtual Span<const ViewType> span(size_t row, size_t size) const {
        MMA_THROW(UnsupportedOperationException());
    }

    ViewType get(size_t row) const {
        return array_.access(0, row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};

}
