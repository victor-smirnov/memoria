
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>

#include <memoria/v1/core/iovector/io_substream_row_array_fixed_size.hpp>
#include <memoria/v1/core/iovector/io_substream_row_array_fixed_size_view.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array_so.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {

template <
    typename V,
    int32_t Blocks_ = 1
>
struct PackedFSEArrayTypes {
    using Value = V;
    static const int32_t Blocks = Blocks_;
};

template <typename Types> class PackedFSEArray;

template <typename V, int32_t Blocks = 1>
using PkdFSQArrayT = PackedFSEArray<PackedFSEArrayTypes<V, Blocks>>;


namespace _ {

    template <bool Selector>
    struct GenerateDataEventsHelper {
        template <int32_t Blocks, typename Value, typename H, typename S, typename B>
        static void process(H* handler, S size_, const B* buffer_)
        {
            handler->value("DATA_ITEMS", BlockValueProviderFactory::provider(true, size_ * Blocks, [&](int32_t idx) -> const Value& {
                return *(buffer_ + idx);
            }));
        }
    };

    template <>
    struct GenerateDataEventsHelper<true> {
        template <int32_t Blocks, typename Value, typename H, typename S, typename B>
        static void process(H* handler, S size_, const B* buffer_)
        {
            ValueHelper<Value>::setup(handler, "DATA_ITEMS", buffer_, size_ * Blocks, IBlockDataEventHandler::BYTE_ARRAY);
        }
    };

}


template <typename Types_>
class PackedFSEArray {

public:
    static const uint32_t VERSION                                                   = 1;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    typedef Types_                                                              Types;
    typedef PackedFSEArray<Types>                                               MyType;

    typedef PackedAllocator                                                     Allocator;
    typedef typename Types::Value                                               Value;

    static constexpr int32_t Indexes                                                    = 0;
    static constexpr int32_t Blocks                                                     = Types::Blocks;

    static constexpr int32_t SafetyMargin                                               = 0;

    using Values = core::StaticVector<Value, Blocks>;
    using SizesT = core::StaticVector<int32_t, Blocks>;

    using ConstPtrsT = core::StaticVector<const Value*, Blocks>;

    class ReadState {
        int32_t idx_ = 0;
    public:
        int32_t& local_pos() {return idx_;}
        const int32_t& local_pos() const {return idx_;}
    };

    using GrowableIOSubstream = io::IORowwiseFixedSizeArraySubstreamImpl<Value, Blocks>;
    using IOSubstreamView     = io::IORowwiseFixedSizeArraySubstreamViewImpl<Value, Blocks>;

    using ExtData = EmptyType;
    using SparseObject = PackedFSEArraySO<ExtData, MyType>;

private:
    PackedAllocatable header_;

    int32_t size_;
    int32_t max_size_;

    Value buffer_[];

public:
    PackedFSEArray() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

    int32_t capacity() const {return max_size_ - size_;}

    int32_t total_capacity() const
    {
        int32_t my_size     = header_.allocator()->element_size(this);
        int32_t free_space  = header_.allocator()->free_space();
        int32_t data_size   = sizeof(Value) * size_ * Blocks;

        return (my_size + free_space - data_size) / (sizeof(Value) * Blocks);
    }

    int32_t block_size() const
    {
        return sizeof(MyType) + max_size_ * sizeof(Value) * Blocks;
    }

    int32_t block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

public:



    static constexpr int32_t block_size(int32_t array_size)
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr int32_t packed_block_size(int32_t array_size)
    {
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr int32_t elements_for(int32_t block_size)
    {
        return max_size_for(block_size);
    }

    int32_t allocated_block_size() const
    {
        if (header_.allocator_offset() != 0)
        {
            return header_.allocator()->element_size(this);
        }
        else {
            return block_size();
        }
    }

    OpStatus init(int32_t block_size)
    {
        size_ = 0;
        max_size_ = max_size_for(block_size);
        return OpStatus::OK;
    }

    OpStatus init(const SizesT& capacities)
    {
        size_ = 0;
        max_size_ = capacities[0];

        return OpStatus::OK;
    }

    static constexpr int32_t max_size_for(int32_t block_size) {
        return (block_size - empty_size()) / (sizeof(Value) * Blocks);
    }

    static constexpr int32_t empty_size()
    {
        return sizeof(MyType);
    }

    OpStatus initEmpty()
    {
        size_ = 0;
        max_size_   = 0;

        return OpStatus::OK;
    }

    OpStatus init()
    {
        size_ = 0;
        max_size_   = 0;

        return OpStatus::OK;
    }

    int32_t object_size() const
    {
        int32_t object_size = sizeof(MyType) + sizeof(Value) * size_ * Blocks;
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(object_size);
    }

    Value& value(int32_t block, int32_t idx) {
        return buffer_[idx * Blocks + block];
    }

    const Value& value(int32_t block, int32_t idx) const {
        return buffer_[idx * Blocks + block];
    }


    Value get_values(int32_t idx) const {
        return value(0, idx);
    }

    Value get_values(int32_t idx, int32_t index) const {
        return value(index, idx);
    }

    Value* data() {
        return buffer_;
    }

    const Value* data() const {
        return buffer_;
    }

    Value* values() {
        return buffer_;
    }

    const Value* values() const {
        return buffer_;
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t start, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    // =================================== Update ========================================== //

    OpStatus reindex() {return OpStatus::OK;}
    void check() const {}

    bool ensureCapacity(int32_t size)
    {
        int32_t capacity = this->capacity();
        if (capacity < size)
        {
            (void)enlarge(size - capacity);
            return true;
        }
        else {
            return false;
        }
    }

    OpStatus enlarge(int32_t items_num)
    {
        Allocator* alloc = header_.allocator();

        int32_t requested_block_size    = (max_size_ + items_num) * sizeof(Value) * Blocks + empty_size();
        int32_t new_size                = alloc->resizeBlock(this, requested_block_size);

        if (isFail(new_size)) {
            return OpStatus::FAIL;
        }

        max_size_ = max_size_for(new_size);

        return OpStatus::OK;
    }

    OpStatus shrink(int32_t items_num)
    {
        MEMORIA_V1_ASSERT(max_size_ - items_num, >=, size_);

        return enlarge(-items_num);
    }

    OpStatus remove(int32_t start, int32_t end)
    {
        MEMORIA_V1_ASSERT_TRUE(start >= 0);
        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        int32_t room_length = end - start;
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(room_length, <= , size - start);

        Value* values = this->values();

        CopyBuffer(
                values + end * Blocks,
                values + start * Blocks,
                (size_ - end) * Blocks
        );

        size_ -= room_length;

        return shrink(room_length);
    }

    OpStatus removeSpace(int32_t room_start, int32_t room_end) {
        return remove(room_start, room_end);
    }

    OpStatus insertSpace(int32_t idx, int32_t room_length)
    {
        MEMORIA_V1_ASSERT(idx, <=, this->size());
        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(room_length, >=, 0);

        int32_t capacity = this->capacity();

        if (capacity < room_length)
        {
            if(isFail(enlarge(room_length - capacity))) {
                return OpStatus::FAIL;
            }
        }

        auto values = this->values();

        CopyBuffer(
                values + idx * Blocks,
                values + (idx + room_length) * Blocks,
                (size_ - idx) * Blocks
        );

        size_ += room_length;

        clear(idx, idx + room_length);

        return OpStatus::OK;
    }

    void clearValues(int32_t idx) {
        buffer_[idx] = 0;
    }

    void clear(int32_t start, int32_t end)
    {
        auto values = this->values();

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                values[c * Blocks + block] = Value();
            }
        }
    }

    void reset()
    {
        size_ = 0;
    }


    OpStatus splitTo(MyType* other, int32_t idx)
    {
        MEMORIA_V1_ASSERT(other->size(), ==, 0);

        int32_t split_size = this->size() - idx;
        if(isFail(other->insertSpace(0, split_size))) {
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

    OpStatus mergeWith(MyType* other)
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        if(isFail(other->insertSpace(other_size, my_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(copyTo(other, 0, my_size, other_size))) {
            return OpStatus::FAIL;
        }

        if(isFail(removeSpace(0, my_size))) {
            return OpStatus::FAIL;
        }

        return reindex();
    }


    OpStatus copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
    {
        MEMORIA_V1_ASSERT_TRUE(copy_from >= 0);
        MEMORIA_V1_ASSERT_TRUE(count >= 0);

        CopyBuffer(
                this->values() + copy_from * Blocks,
                other->values() + copy_to * Blocks,
                count * Blocks
        );

        return OpStatus::OK;
    }

    template <typename TreeType>
    OpStatus transferDataTo(TreeType* other) const
    {
        const auto* my_values   = values();
        auto* other_values      = other->values();

        int32_t size = this->size() * Blocks;

        for (int32_t c = 0; c < size; c++)
        {
            other_values[c] = my_values[c];
        }

        other->size() = size;

        return OpStatus::OK;
    }

    OpStatus resize(int32_t delta)
    {
        if (delta > 0)
        {
            return insertSpace(size_, delta);
        }
        else {
            return removeSpace(size_, -delta);
        }

        return OpStatus::OK;
    }

    // ===================================== IO ============================================ //

    OpStatus insert(int32_t pos, const Value& val)
    {
        if (isFail(insertSpace(pos, 1))) {
            return OpStatus::FAIL;
        }

        for (int32_t block = 0;  block < Blocks; block++)
        {
            value(block, pos) = val;
        }

        return OpStatus::OK;
    }

    OpStatus insert(int32_t block, int32_t pos, const Value& val)
    {
        if (isFail(insertSpace(pos, 1))) {
            return OpStatus::FAIL;
        }

        value(block, pos) = val;

        return OpStatus::OK;
    }


    template <typename Adaptor>
    OpStatus insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        if (isFail(insertSpace(pos, size))) {
            return OpStatus::FAIL;
        }

        auto values = this->values();

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                const auto& val = adaptor(block, c);
                values[c * Blocks + block] = val;
            }
        }

        return OpStatus::OK;
    }




    OpStatusT<int32_t> insert_io_substream(int32_t at, const io::IOSubstream& substream, int32_t start, int32_t size)
    {
        const io::IORowwiseFixedSizeArraySubstream<Value>& buffer
                = io::substream_cast<io::IORowwiseFixedSizeArraySubstream<Value>>(substream);

        if(isFail(insertSpace(at, size))) {
            return OpStatus::FAIL;
        }

        auto buffer_values = T2T<const Value*>(buffer.select(start));

        int32_t end = start + size;

        CopyBuffer(buffer_values, this->values() + at * Blocks, (end - start) * Blocks);

        return OpStatusT<int32_t>(at + size);
    }

    void configure_io_substream(io::IOSubstream& substream) const
    {        
        auto& view = io::substream_cast<IOSubstreamView>(substream);
        view.configure(this->values(), this->size());
    }

    ReadState positions(int32_t idx) const
    {
        ReadState state;
        state.local_pos() = idx;
        return state;
    }


    template <typename Adaptor>
    OpStatus _insert(int32_t pos, int32_t size, Adaptor&& adaptor)
    {
        if(isFail(insertSpace(pos, size))) {
            return OpStatus::FAIL;
        }

        auto values = this->values();

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t b = 0; b < Blocks; b++) {
                values[(c + pos) * Blocks + b] = adaptor(b, c);
            }
        }

        return OpStatus::OK;
    }


    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _update(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        value(0, pos) = val;

        return OpStatus::OK;
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _update_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        value(0, pos) = val(0);

        return OpStatus::OK;
    }

    template <int32_t Offset, typename Value, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _insert(int32_t pos, Value&& val, BranchNodeEntryItem<T, Size>& accum)
    {
        return _insert(pos, 1, [&](int block, int idx) -> const auto& {
            return val[block];
        });
    }

    template <int32_t Offset, typename T, int32_t Size, template <typename, int32_t> class BranchNodeEntryItem, typename AccessorFn>
    OpStatus _insert_b(int32_t pos, BranchNodeEntryItem<T, Size>& accum, AccessorFn&& val)
    {
        return _insert(pos, 1, [&](int block, int idx) -> const auto& {
            return val(block);
        });
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    OpStatus _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        return remove(idx, idx + 1);
    }






    template <typename Fn>
    void read(int32_t block, int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        auto values = this->values();

        for (int32_t c = start; c < end; c++)
        {
            fn(values[c * Blocks + block]);
            fn.next();
        }
    }


    template <typename Fn>
    void read(int32_t start, int32_t end, Fn&& fn) const
    {
        MEMORIA_V1_ASSERT(end, <=, size_);
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        auto values = this->values();

        for (int32_t c = start; c < end; c++)
        {
            for (int32_t b = 0; b < Blocks; b++) {
                fn(b, values[c * Blocks + b]);
            }

            fn.next();
        }
    }

    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {
        out << "size_       = " << size_ << std::endl;
        out << "max_size_   = " << max_size_ << std::endl;
        out << std::endl;

        out << "Data:" << std::endl;

        const Value* values_ = buffer_;

        if (Blocks == 1)
        {
            dumpArray<Value>(out, size_ * Blocks, [&](int32_t pos) -> Value {
                return values_[pos];
            });
        }
        else {
            for (int32_t c = 0; c < size_; c++)
            {
                out << c << ": ";
                for (int32_t b = 0; b < Blocks; b++)
                {
                    out << values_[c * Blocks + b] << ", ";
                }
                out << std::endl;
            }
        }
    }


    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("FSE_ARRAY");

        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);

        handler->startGroup("DATA", size_);

        _::GenerateDataEventsHelper<sizeof(Value) == 1>::template process<Blocks, Value>(handler, size_, buffer_);

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        header_.serialize(buf);

        FieldFactory<int32_t>::serialize(buf, size_);
        FieldFactory<int32_t>::serialize(buf, max_size_);

        FieldFactory<Value>::serialize(buf, buffer_, size_ * Blocks);
    }

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        header_.deserialize(buf);

        FieldFactory<int32_t>::deserialize(buf, size_);
        FieldFactory<int32_t>::deserialize(buf, max_size_);

        FieldFactory<Value>::deserialize(buf, buffer_, size_ * Blocks);
    }
};


template <typename Types>
struct PkdStructSizeType<PackedFSEArray<Types>> {
    static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T>
struct StructSizeProvider<PackedFSEArray<T>> {
    static const int32_t Value = 0;
};


template <typename T>
struct PkdSearchKeyTypeProvider<PackedFSEArray<T>> {
    using Type = typename PackedFSEArray<T>::Value;
};



}}
