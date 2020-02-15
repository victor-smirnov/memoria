
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
    static constexpr int32_t Indexes = 0;
    static constexpr PkdSearchType KeySearchType = SearchType_;


    using MyType = PackedEmptyStruct<Value_, SearchType_>;

    using DataType = typename std::remove_reference<Value_>::type;

    using Value = typename DataTypeTraits<DataType>::ViewType;

    using AccumValue = Value;

    static constexpr int32_t Blocks = Indexes;

    using Values = core::StaticVector<Value, Blocks>;

    using IndexValue = Value;
    using SizesT = core::StaticVector<int32_t, Blocks>;
    using ReadState = SizesT;

    using ExtData = DTTTypeDimensionsTuple<DataType>;
    using SparseObject = PackedEmptyStructSO<ExtData, MyType>;

private:
    PackedAllocatable header_;
public:
    PackedEmptyStruct() = default;

    Value access(int32_t column, int32_t row) const noexcept {
        return Value{};
    }

    int32_t size() const {return 0;}

    int32_t block_size() const
    {
        return sizeof(MyType);
    }

    int32_t block_size(const MyType* other) const
    {
        return block_size();
    }

    static constexpr int32_t block_size(int32_t array_size)
    {
        return sizeof(MyType);
    }

    static constexpr int32_t packed_block_size(int32_t array_size)
    {
        return sizeof(MyType);
    }

    static int32_t elements_for(int32_t block_size)
    {
        size_t bsize = block_size;

        return bsize >= sizeof(MyType) ? std::numeric_limits<int32_t>::max() : 0;
    }


    VoidResult init(int32_t block_size) noexcept {return VoidResult::of();}

    VoidResult init(const SizesT& capacities) noexcept {return VoidResult::of();}

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }


    VoidResult init() noexcept
    {
        return VoidResult::of();
    }

    template <typename T>
    void max(T& accum) const
    {
    }

    template <typename T>
    VoidResult setValues(int32_t idx, T&&) noexcept {return VoidResult::of();}


    template <typename T>
    int32_t append(int32_t size, T&&)
    {
        return size;
    }

    // =================================== Update ========================================== //

    VoidResult reindex() {return VoidResult::of();}


    VoidResult remove(int32_t start, int32_t end) noexcept
    {
        return VoidResult::of();
    }

    VoidResult removeSpace(int32_t room_start, int32_t room_end) noexcept {
        return remove(room_start, room_end);
    }



    VoidResult reset() noexcept
    {
        return VoidResult::of();
    }


    VoidResult splitTo(MyType* other, int32_t idx) noexcept
    {
        return VoidResult::of();
    }

    VoidResult mergeWith(MyType* other) const noexcept
    {
        return VoidResult::of();
    }

    // ===================================== IO ============================================ //

    VoidResult insert(int32_t pos, Value val) noexcept
    {
        return VoidResult::of();
    }

    VoidResult insert(int32_t block, int32_t pos, Value val) noexcept
    {
        return VoidResult::of();
    }


    template <typename Adaptor>
    VoidResult insert(int32_t pos, int32_t size, Adaptor&& adaptor) noexcept
    {
        return VoidResult::of();
    }


    Values get_values(int32_t) const {
        return Values();
    }

    ReadState positions(int32_t idx) const {
        return ReadState(idx);
    }


    template <typename Adaptor>
    VoidResult _insert(int32_t pos, int32_t size, Adaptor&& adaptor) noexcept
    {
        return VoidResult::of();
    }


    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    VoidResult _update(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum) noexcept
    {
        return VoidResult::of();
    }

    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    VoidResult _insert(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum) noexcept
    {
        return VoidResult::of();
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    VoidResult _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum) noexcept
    {
        return VoidResult::of();
    }



    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
    }

    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {

    }



    template <typename Fn>
    SizesT scan(int32_t start, int32_t end, Fn&& fn) const
    {
        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {}


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startStruct();
        handler->startGroup("EMPTY_STRUCT");
        handler->endGroup();
        handler->endStruct();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        return header_.serialize(buf);
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
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
    static constexpr int32_t Blocks = PackedEmptyStruct<T, S>::Blocks;
    static constexpr int32_t Indexes = PackedEmptyStruct<T, S>::Indexes;
};

}
