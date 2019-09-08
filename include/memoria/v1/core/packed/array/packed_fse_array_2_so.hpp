
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

#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size.hpp>
#include <memoria/v1/core/iovector/io_substream_col_array_fixed_size_view.hpp>

namespace memoria {
namespace v1 {

template <typename ExtData, typename PkdStruct>
class PackedFixedSizeElementArraySO {
    const ExtData* ext_data_;
    PkdStruct* data_;

    using MyType = PackedFixedSizeElementArraySO;

    using Value = typename PkdStruct::Value;
    using ViewType = Value;

    static constexpr psize_t VALUES = PkdStruct::VALUES;

public:
    using PkdStructT = PkdStruct;

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



    void generateDataEvents(IBlockDataEventHandler* handler) const
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
    }

    void check() const {}

    psize_t size() const {
        return data_->metadata().size();
    }

    /*********************** API *********************/

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
//        auto end = this->end(column);
//        auto ii = std::upper_bound(begin(column), end, val);

//        if (ii != end)
//        {
//            return ii.pos();
//        }

        return FindResult(data_->size());
    }

    FindResult findGEForward(psize_t column, const ViewType& val) const
    {
//        auto end = this->end(column);
//        auto ii = std::lower_bound(begin(column), end, val);

//        if (ii != end)
//        {
//            return ii.pos();
//        }

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

    auto findForward(SearchType search_type, psize_t column, const OptionalT<ViewType>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(column, val.value());
        }
        else {
            return findGEForward(column, val.value());
        }
    }

    OpStatus insertSpace(psize_t idx, psize_t room_length)
    {
        auto& meta = data_->metadata();

        MEMORIA_V1_ASSERT(idx, <=, meta.size());

        psize_t total_data_length{};

        for (psize_t c = 0; c < Columns; c++)
        {
            total_data_length += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (meta.size() + room_length) * sizeof(Value)
            );
        }

        if (isFail(data_->resize(PkdStruct::empty_size() + total_data_length))) {
            return OpStatus::FAIL;
        }

        for (psize_t c = 0; c < Columns; c++)
        {
            psize_t new_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(
                (meta.size() + room_length) * sizeof(Value)
            );

            if (isFail(data_->resizeBlock(c + VALUES, new_block_size))) {
                return OpStatus::FAIL;
            }
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

        return OpStatus::OK;
    }



    OpStatus copyTo(MyType& other, psize_t copy_from, psize_t count, psize_t copy_to) const
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            MemMoveBuffer(
                data_->values(c) + copy_from,
                other.data()->values(c) + copy_to,
                count
            );
        }

        return OpStatus::OK;
    }


    OpStatus splitTo(MyType& other, psize_t idx)
    {
        MEMORIA_V1_ASSERT(other.size(), ==, 0);

        psize_t split_size = this->size() - idx;
        if(isFail(other.insertSpace(0, split_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, idx, split_size, 0))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(idx, this->size()))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    OpStatus mergeWith(MyType& other)
    {
        psize_t my_size     = this->size();
        psize_t other_size  = other.size();

        if(isFail(other.insertSpace(other_size, my_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, 0, my_size, other_size))) {
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

        MEMORIA_V1_ASSERT(room_end, <=, meta.size());
        MEMORIA_V1_ASSERT(room_start, <=, meta.size());
        MEMORIA_V1_ASSERT(room_start, <=, room_end);

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

            if(isFail(data_->resizeBlock(c + VALUES, data_length))) {
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

        io::FixedSizeArrayColumnMetadata<Value> columns[Columns];

        auto& meta = data_->metadata();

        for (psize_t c = 0; c < Columns; c++)
        {
            columns[c].capacity = data_->element_size(c) / sizeof(Value);
            columns[c].data_buffer = data_->values(c);
            columns[c].size = meta.size();
        }

        view.configure(columns);
    }

    OpStatusT<int32_t> insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size)
    {
        const io::IOColumnwiseFixedSizeArraySubstream<Value>& buffer
                = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<Value>>(substream);

        if(isFail(insertSpace(at, size))) {
            return OpStatus::FAIL;
        }

        for (psize_t c = 0; c < Columns; c++)
        {
            auto buffer_values = T2T<const Value*>(buffer.select(c, start));
            int32_t end = start + size;
            MemCpyBuffer(buffer_values, data_->values(c) + at, end - start);
        }

        return OpStatusT<int32_t>(at + size);
    }



    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        for (psize_t c = 0; c < Columns; c++)
        {
            data_->values(c)[pos] = val(c);
        }

        return OpStatus::OK;
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(psize_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        if (isFail(insertSpace(pos, 1)))
        {
            return OpStatus::FAIL;
        }

        return _update_b<Offset>(pos, accum, std::forward<AccessorFn>(val));
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(psize_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        return removeSpace(idx, idx + 1);
    }
};


}
}
