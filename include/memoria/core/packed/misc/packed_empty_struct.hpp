
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

#include <memoria/core/packed/misc/packed_empty_struct_so.hpp>

#include <limits>

namespace memoria {


template <typename Value_ = int64_t, PkdSearchType SearchType_ = PkdSearchType::SUM>
class PackedEmptyStruct {

public:
    static const uint32_t VERSION = 1;
    static constexpr size_t Indexes = 0;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedEmptyStruct<Value_, SearchType_>;

    using DataType = typename std::remove_reference<Value_>::type;

    using Value = typename DataTypeTraits<DataType>::ViewType;

    using AccumValue = Value;

    static constexpr size_t Blocks = Indexes;

    using Values = core::StaticVector<Value, Blocks>;

    using IndexValue = Value;
    using SizesT = core::StaticVector<size_t, Blocks>;
    using ReadState = SizesT;

    using ExtData = DTTTypeDimensionsTuple<DataType>;
    using SparseObject = PackedEmptyStructSO<ExtData, MyType>;

private:
    PackedAllocatable header_;
public:
    PackedEmptyStruct() = default;

    Value access(size_t column, size_t row) const noexcept {
        return Value{};
    }

    size_t size() const noexcept {return 0;}

    size_t block_size() const noexcept
    {
        return sizeof(MyType);
    }

    size_t block_size_for(const MyType* other) const noexcept
    {
        return block_size();
    }

    static constexpr size_t block_size(size_t array_size) noexcept
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


    void init(size_t block_size) noexcept {}

    void init(const SizesT& capacities) noexcept {}

    static constexpr size_t default_size(size_t available_space) noexcept
    {
        return empty_size();
    }

    void init_default(size_t block_size) noexcept {
        return init();
    }

    static constexpr size_t empty_size() noexcept
    {
        return sizeof(MyType);
    }


    void init() noexcept
    {}


    void reindex() noexcept {}


    void remove(size_t start, size_t end) noexcept
    {}

    void reset() noexcept
    {}


    void split_to(MyType* other, size_t idx) noexcept
    {}

    void commit_merge_with(MyType* other) const noexcept
    {}


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("EMPTY_STRUCT");
        handler->endGroup();
        handler->endStruct();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        return header_.serialize(buf);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        return header_.deserialize(buf);
    }
};

template <typename T, PkdSearchType S>
struct PackedStructTraits<PackedEmptyStruct<T, S>>
{
    using SearchKeyDataType = T;

    using AccumType = typename DataTypeTraits<SearchKeyDataType>::ViewType;
    using SearchKeyType = typename DataTypeTraits<SearchKeyDataType>::ViewType;

    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::FIXED;
    static constexpr PkdSearchType KeySearchType = S;
    static constexpr size_t Blocks = PackedEmptyStruct<T, S>::Blocks;
    static constexpr size_t Indexes = PackedEmptyStruct<T, S>::Indexes;
};

}
