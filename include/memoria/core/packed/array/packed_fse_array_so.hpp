
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

#include <memoria/core/packed/datatypes/fixed_size.hpp>
#include <memoria/core/packed/array/packed_array_iterator.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/datatypes/buffer/buffer_0_fse.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <algorithm>

namespace memoria {

template <typename ExtData, typename PkdStruct>
class PackedFixedSizeElementArraySO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedFixedSizeElementArraySO;

    using Value = typename PkdStruct::Value;
    using Values = typename PkdStruct::Values;
    using ViewType = typename PkdStruct::ViewType;

    using DataType = typename PkdStruct::DataType;

    static constexpr psize_t VALUES = PkdStruct::VALUES;

public:

    using Accessor = PkdDataTypeAccessor<
        DataType, std::tuple<>, MyType, FixedSizeDataTypeTag
    >;

    using PkdStructT = PkdStruct;

    using ConstIterator = PkdRandomAccessIterator<Accessor>;

    static constexpr psize_t Columns = PkdStruct::Blocks;
    static constexpr int32_t Indexes = 0;

    PackedFixedSizeElementArraySO(): ext_data_(), data_() {}
    PackedFixedSizeElementArraySO(const ExtData* ext_data, PkdStruct* data):
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



    Span<Value> values(psize_t block)
    {
        const auto& meta = data_->metadata();
        return Span<Value>(data_->values(block), meta->size());
    }

    Span<const Value> values(psize_t block) const
    {
        const auto& meta = data_->metadata();
        return Span<const Value>(data_->values(block), meta.size());
    }

    const Value& access(psize_t column, psize_t idx) const {
        return data_->values(column)[idx];
    }

    Value& access(psize_t column, psize_t idx) {
        return data_->values(column)[idx];
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        const auto& meta = data_->metadata();

        handler->startStruct();
        handler->startGroup("FX_SIZE_EL_ARRAY");

        handler->value("SIZE", &meta.size());

        handler->startGroup("DATA", meta.size());

        std::vector<Span<const Value>> columns;

        for (psize_t c = 0; c < Columns; c++) {
            columns.emplace_back(values(c));
        }

        for (int32_t c = 0; c < meta.size(); c++)
        {
            handler->value("VALUES", BlockValueProviderFactory::provider(Columns, [&](int32_t idx) {
                return columns[idx][c];
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

    ConstIterator begin(psize_t column) const {
        return ConstIterator(Accessor(*this, column), 0, size());
    }

    ConstIterator end(psize_t column) const
    {
        psize_t size = this->size();
        return ConstIterator(Accessor(*this, column), size, size);
    }

    /*********************** API *********************/

    template <typename T>
    VoidResult setValues(psize_t pos, const core::StaticVector<T, Columns>& values) noexcept
    {
        for (psize_t c = 0; c < Columns; c++) {
            access(c, pos) = values[c];
        }

        return VoidResult::of();
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

    VoidResult insertSpace(psize_t idx, psize_t room_length) noexcept
    {
        auto& meta = data_->metadata();

        MEMORIA_V1_ASSERT_RTN(idx, <=, meta.size());

        psize_t total_data_length{};

        for (psize_t c = 0; c < Columns; c++)
        {
            total_data_length += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (meta.size() + room_length) * sizeof(Value)
            );
        }

        MEMORIA_TRY_VOID(data_->resize(PkdStruct::empty_size() + total_data_length));

        for (psize_t c = 0; c < Columns; c++)
        {
            psize_t new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (meta.size() + room_length) * sizeof(Value)
            );

            MEMORIA_TRY_VOID(data_->resizeBlock(c + VALUES, new_block_size));
        }


        for (psize_t c = 0; c < Columns; c++)
        {
            auto values = data_->values(c);

            MemCpyBuffer(
                values + idx,
                values + (idx + room_length),
                size() - idx
            );
        }

        data_->size() += room_length;

        return VoidResult::of();
    }



    VoidResult copyTo(MyType& other, psize_t copy_from, psize_t count, psize_t copy_to) const noexcept
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            MemMoveBuffer(
                data_->values(c) + copy_from,
                other.data()->values(c) + copy_to,
                count
            );
        }

        return VoidResult::of();
    }


    VoidResult splitTo(MyType& other, psize_t idx) noexcept
    {
        MEMORIA_V1_ASSERT_RTN(other.size(), ==, 0);

        psize_t split_size = this->size() - idx;
        MEMORIA_TRY_VOID(other.insertSpace(0, split_size));

        MEMORIA_TRY_VOID(copyTo(other, idx, split_size, 0));

        MEMORIA_TRY_VOID(removeSpace(idx, this->size()));

        return VoidResult::of();
    }

    VoidResult mergeWith(MyType& other) noexcept
    {
        psize_t my_size     = this->size();
        psize_t other_size  = other.size();

        MEMORIA_TRY_VOID(other.insertSpace(other_size, my_size));

        MEMORIA_TRY_VOID(copyTo(other, 0, my_size, other_size));

        MEMORIA_TRY_VOID(removeSpace(0, my_size));

        return VoidResult::of();
    }

    VoidResult removeSpace(psize_t room_start, psize_t room_end) noexcept
    {
        auto& meta = data_->metadata();

        MEMORIA_V1_ASSERT_RTN(room_end, <=, meta.size());
        MEMORIA_V1_ASSERT_RTN(room_start, <=, meta.size());
        MEMORIA_V1_ASSERT_RTN(room_start, <=, room_end);

        psize_t room_length = room_end - room_start;


        for (psize_t c = 0; c < Columns; c++)
        {
            auto values = data_->values(c);

            MemMoveBuffer(
                values + room_end,
                values + room_start,
                meta.size() - room_end
            );
        }

        for (psize_t c = 0; c < Columns; c++)
        {
            psize_t data_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (meta.size() - room_length) * sizeof(Value)
            );

            MEMORIA_TRY_VOID(data_->resizeBlock(c + VALUES, data_length));
        }

        meta.size() -= room_length;

        return VoidResult::of();
    }

    Value get_values(psize_t idx) const {
        return get_values(0, idx);
    }

    Value get_values(psize_t idx, psize_t column) const {
        return data_->values(column)[idx];
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

    Int32Result insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size) noexcept
    {
        static_assert(Columns == 1, "");

        using IOBuffer = typename PkdStruct::GrowableIOSubstream;

        const IOBuffer& buffer = io::substream_cast<IOBuffer>(substream);

        MEMORIA_TRY_VOID(insertSpace(at, size));

        for (psize_t c = 0; c < Columns; c++)
        {
            auto buffer_values = ptr_cast<const Value>(buffer.span(start, size).data());
            MemCpyBuffer(buffer_values, data_->values(c) + at, size);
        }

        return Int32Result::of(at + size);
    }



    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    VoidResult _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val) noexcept
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            data_->values(c)[pos] = val(c);
        }

        return VoidResult::of();
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    VoidResult _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val) noexcept
    {
        MEMORIA_TRY_VOID(insertSpace(pos, 1));
        return _update_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    VoidResult _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum) noexcept
    {
        return removeSpace(idx, idx + 1);
    }

    psize_t estimate_insert_upsize(const ViewType& view) const noexcept
    {
        static_assert(Columns == 1, "");


        psize_t current_block_size = data_->element_size(VALUES);
        psize_t requested_data_size = (data_->size() + 1) * sizeof(Value);

        psize_t requested_data_size_aligned = PackedAllocatable::roundUpBytesToAlignmentBlocks(requested_data_size);

        return requested_data_size_aligned - current_block_size;
    }

    psize_t estimate_replace_upsize(psize_t idx, const ViewType& view) const noexcept
    {
        return 0;
    }

    VoidResult replace(psize_t column, psize_t idx, const ViewType& view) noexcept
    {
        access(column, idx) = view;

        return VoidResult::of();
    }

    VoidResult insert(psize_t idx, const ViewType& view) noexcept
    {
        static_assert(Columns == 1, "");

        MEMORIA_TRY_VOID(insertSpace(idx, 1));

        access(0, idx) = view;

        return VoidResult::of();
    }

    template <typename T>
    VoidResult insert(psize_t pos, const core::StaticVector<T, Columns>& values) noexcept
    {
        MEMORIA_TRY_VOID(insertSpace(pos, 1));

        for (psize_t c = 0; c < Columns; c++)
        {
            access(c, pos) = values[c];
        }

        return VoidResult::of();
    }

    VoidResult insert(int32_t idx, int32_t size, std::function<const Values& (int32_t)> provider) noexcept
    {
        MEMORIA_TRY_VOID(insertSpace(idx, size));

        Value* values[Columns];
        for (int32_t block  = 0; block < Columns; block++)
        {
            values[block] = data_->values(block);
        }

        for (int32_t c = idx; c < idx + size; c++)
        {
            const Values& vals = provider(c - idx);

            for (int32_t block = 0; block < Columns; block++)
            {
                values[block][c] = vals[block];
            }
        }

        return VoidResult::of();
    }



    VoidResult remove(psize_t idx) noexcept {
        return removeSpace(idx, idx + 1);
    }

    VoidResult reindex() noexcept {
        return VoidResult::of();
    }
};


template <typename DataType, typename ExtData, typename PkdStruct>
class IO1DArraySubstreamViewImpl<
        DataType,
        PackedFixedSizeElementArraySO<ExtData, PkdStruct>
>: public io::IO1DArraySubstreamView<DataType> {

    using Base = io::IO1DArraySubstreamView<DataType>;
    using ArraySO = PackedFixedSizeElementArraySO<ExtData, PkdStruct>;

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
        return Span<const ViewType>(
            array_.data()->values(0) + row, size
        );
    }

    ViewType get(size_t row) const {
        return array_.access(0, row);
    }

    U8String describe() const {
        return TypeNameFactory<Base>::name().to_u8();
    }
};

}

