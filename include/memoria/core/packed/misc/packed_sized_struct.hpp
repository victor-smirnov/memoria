
// Copyright 2016 Victor Smirnov
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
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/packed/misc/packed_sized_struct_so.hpp>

#include <limits>

namespace memoria {

template <typename DataType_ = int64_t, size_t Indexes_ = 0, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedSizedStruct {

public:
    static const uint32_t VERSION = 1;
    static constexpr size_t Indexes = Indexes_;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedSizedStruct;

    using DataType = std::remove_reference_t<DataType_>;

    using Value = typename DataTypeTraits<DataType>::ViewType;
    using IndexDataType = DataType;

    static constexpr size_t Blocks = Indexes;

    using Values = core::StaticVector<Value, Blocks>;

    using IndexValue = Value;
    using SizesT = core::StaticVector<size_t, Blocks>;
    using ReadState = SizesT;

    using ExtData = DTTTypeDimensionsTuple<DataType>;
    using SparseObject = PackedSizedStructSO<ExtData, MyType>;

    class AppendState {
        psize_t size_ = 0;
    public:
        psize_t& size() {return size_;}
        const psize_t& size() const {return size_;}
    };



private:
    PackedAllocatable header_;

    psize_t size_;

public:
    PackedSizedStruct() = default;

    psize_t& size() noexcept {return size_;}
    const psize_t& size() const noexcept {return size_;}


    size_t block_size() const noexcept
    {
        return sizeof(MyType);
    }

    size_t block_size_for(const MyType* other) const noexcept
    {
        return block_size(size_ + other->size_);
    }

    static constexpr size_t block_size(size_t array_size) noexcept
    {
        return sizeof(MyType);
    }

    static constexpr size_t block_size(SizesT array_size) noexcept
    {
        return sizeof(MyType);
    }

    static constexpr size_t packed_block_size(size_t array_size) noexcept
    {
        return sizeof(MyType);
    }

    static size_t elements_for(size_t block_size) noexcept
    {
        size_t bsize = block_size;
        return bsize >= sizeof(MyType) ? std::numeric_limits<size_t>::max() : 0;
    }


    VoidResult init(size_t block_size) noexcept
    {
        size_ = 0;
        return VoidResult::of();
    }

    VoidResult init(const SizesT& capacities) noexcept
    {
        size_ = 0;
        return VoidResult::of();
    }

    static constexpr size_t empty_size() noexcept
    {
        return sizeof(MyType);
    }

    static constexpr size_t default_size(size_t available_space) noexcept
    {
        return empty_size();
    }

    VoidResult init_default(size_t block_size) noexcept {
        return init();
    }

    VoidResult init() noexcept
    {
        size_ = 0;

        return VoidResult::of();
    }


    auto sum(size_t) const noexcept
    {
        return size_;
    }


    VoidResult copyTo(MyType* other) const noexcept
    {
        other->size_ = this->size_;
        return VoidResult::of();
    }


    VoidResult reindex() noexcept {return VoidResult::of();}

    void check() const
    {
        MEMORIA_ASSERT(size_, >=, 0);
    }

    VoidResult remove(size_t start, size_t end) noexcept
    {
        MEMORIA_V1_ASSERT_TRUE_RTN(start >= 0);
        MEMORIA_V1_ASSERT_TRUE_RTN(end >= 0);

        size_t room_length = end - start;
        size_ -= room_length;

        return VoidResult::of();
    }

    VoidResult removeSpace(size_t room_start, size_t room_end) noexcept {
        return remove(room_start, room_end);
    }

    VoidResult insertSpace(size_t idx, size_t room_length) noexcept
    {
        MEMORIA_ASSERT_RTN(idx, <=, this->size());
        MEMORIA_ASSERT_RTN(idx, >=, 0);
        MEMORIA_ASSERT_RTN(room_length, >=, 0);

        size_ += room_length;

        return VoidResult::of();
    }

    VoidResult reset() noexcept
    {
        size_ = 0;
        return VoidResult::of();
    }


    VoidResult splitTo(MyType* other, size_t idx) noexcept
    {
        MEMORIA_ASSERT_RTN(other->size(), ==, 0);

        size_t split_size = this->size() - idx;
        MEMORIA_TRY_VOID(other->insertSpace(0, split_size));

        return removeSpace(idx, this->size());
    }

    VoidResult mergeWith(MyType* other) const noexcept
    {
        size_t my_size     = this->size();
        size_t other_size  = other->size();

        return other->insertSpace(other_size, my_size);
    }


    Value value(size_t, size_t) const noexcept {
        return Value();
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("SIZED_STRUCT");

        handler->value("SIZE", &size_);

        handler->endGroup();
        handler->endStruct();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        header_.serialize(buf);
        FieldFactory<psize_t>::serialize(buf, size_);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        header_.deserialize(buf);
        FieldFactory<psize_t>::deserialize(buf, size_);
    }
};


using StreamSize = PackedSizedStruct<int64_t, 1, PkdSearchType::SUM>;

template <typename T, size_t V, PkdSearchType S>
struct PackedStructTraits<PackedSizedStruct<T, V, S>>
{
    using PkdSearchKeyType  = T;
    using SearchKeyDataType = T;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
    static constexpr PkdSearchType KeySearchType = S;
    static constexpr size_t Blocks = PackedSizedStruct<T, V, S>::Blocks;
    static constexpr size_t Indexes = PackedSizedStruct<T, V, S>::Indexes;
};


}
