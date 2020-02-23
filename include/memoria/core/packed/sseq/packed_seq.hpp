
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

#include <memoria/core/container/block_traits.hpp>
#include <memoria/core/types.hpp>

#include <memoria/core/types/traits.hpp>
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>
#include <memoria/core/tools/md5.hpp>

#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/packed/tree_walkers.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <functional>
#include <algorithm>

namespace memoria {


namespace intrnl1 {

struct EmptyMainWalker {
    void adjust(int64_t value) {}
};

template <typename T>
struct ValueHelper {
    static void setup(IBlockDataEventHandler* handler, const T& value)
    {
        handler->value("VALUE", &value);
    }
};

template <typename T>
struct ValueHelper<BlockID<T> > {
    typedef BlockID<T>                                                   Type;

    static void setup(IBlockDataEventHandler* handler, const Type& value)
    {
        IDValue id(&value);
        handler->value("VALUE", &id);
    }
};

template <>
struct ValueHelper<EmptyValue> {
    typedef EmptyValue Type;

    static void setup(IBlockDataEventHandler* handler, const Type& value)
    {
        int64_t val = 0;
        handler->value("VALUE", &val);
    }
};


}

template <
    typename IK = uint32_t,
    typename V  = uint64_t,
    int32_t Bits_ = 1,
    int32_t BF = PackedSeqBranchingFactor,
    int32_t VPB = PackedSeqValuesPerBranch
>
struct PackedSeqTypes {
    typedef IK              IndexKey;
    typedef V               Value;

    static const int32_t Bits                   = Bits_;
    static const int32_t BranchingFactor        = BF;
    static const int32_t ValuesPerBranch        = VPB;
};

template <typename Types>
class PackedSeq {

    static const uint32_t VERSION               = 1;

    typedef PackedSeq<Types>               MyType;

public:

    typedef typename Types::IndexKey        IndexKey;
    typedef typename Types::Value           Value;
    typedef typename Types::Value           Symbol;

    static const int32_t Bits                   = Types::Bits;
    static const int32_t Blocks                 = 1<<Bits;
    static const int32_t Symbols                = Blocks;
    static const int32_t BranchingFactor        = Types::BranchingFactor;
    static const int32_t ValuesPerBranch        = Types::ValuesPerBranch;

    template <typename T> friend class PackedTree;

private:

    static const int32_t LEVELS_MAX             = 32;

    int32_t     size_;
    int32_t     max_size_;
    int32_t     index_size_;
    int8_t    memory_block_[];


public:

    typedef TypeList<
            ConstValue<uint32_t, VERSION>,
            decltype(size_),
            decltype(max_size_),
            decltype(index_size_),
            IndexKey,
            Value
    >                                                                           FieldsList;

    PackedSeq() {}


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startGroup("PACKED_TREE");

        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);
        handler->value("INDEX_SIZE",    &index_size_);

        handler->startGroup("INDEXES", index_size_);

        for (int32_t idx = 0; idx < index_size_; idx++)
        {
            IndexKey indexes[Blocks];
            for (int32_t block = 0; block < Blocks; block++)
            {
                indexes[block] = index(block, idx);
            }

            handler->value("INDEX", indexes, Blocks);
        }

        handler->endGroup();

        handler->startGroup("DATA", size_);

        handler->symbols("BITMAP", valuesBlock(), size_, Bits);

        handler->endGroup();

        handler->endGroup();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        FieldFactory<int32_t>::serialize(buf, size());
        FieldFactory<int32_t>::serialize(buf, max_size_);
        FieldFactory<int32_t>::serialize(buf, index_size_);

        FieldFactory<IndexKey>::serialize(buf, indexBlock(), Blocks * indexSize());

        const Value* values = valuesBlock();

        FieldFactory<Value>::serialize(buf, values, getUsedValueCells());

        return VoidResult::of();
    }


    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        FieldFactory<int32_t>::deserialize(buf, size());
        FieldFactory<int32_t>::deserialize(buf, max_size_);
        FieldFactory<int32_t>::deserialize(buf, index_size_);

        FieldFactory<IndexKey>::deserialize(buf, indexBlock(), Blocks * indexSize());

        Value* values = valuesBlock();

        FieldFactory<Value>::deserialize(buf, values, getUsedValueCells());

        return VoidResult:;of();
    }

    void initByBlock(int32_t block_size)
    {
        size_ = 0;

        max_size_   = getMaxSize(block_size);
        index_size_ = getIndexSize(max_size_);

        MEMORIA_ASSERT(getDataSize(), <=, block_size);
    }


    void initSizes(int32_t max)
    {
        size_       = 0;
        max_size_   = max;
        index_size_ = getIndexSize(max_size_);
    }

    int32_t getObjectSize() const
    {
        return sizeof(MyType) + getBlockSize();
    }

    static size_t getObjectSzie(size_t capacity)
    {
        MyType seq;
        seq.initSizes(capacity);

        return seq.getObjectSize();
    }

    int32_t getObjectDataSize() const
    {
        return sizeof(size_) + sizeof(max_size_) + sizeof(index_size_) + getBlockSize();
    }

    int32_t getBlockSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(max_size_);
    }

    static int32_t getValueCellsCount(int32_t values_count)
    {
        int32_t total_bits  = values_count * Bits;
        size_t mask     = TypeBitmask<Value>();
        size_t bitsize  = TypeBitsize<Value>();

        size_t suffix   = total_bits & mask;

        return total_bits / bitsize + (suffix > 0);
    }

    static int32_t getValueBlockSize(int32_t values_count)
    {
        return getValueCellsCount(values_count) * sizeof(Value);
    }

    int32_t getDataSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(size_);
    }

    int32_t getTotalDataSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(max_size_);
    }

    int32_t getUsedValueCells() const
    {
        return getValueCellsCount(size_);
    }

    int32_t getTotalValueCells() const
    {
        return getValueCellsCount(max_size_);
    }

    int32_t getValueCellsCapacity() const
    {
        return getValueCellsCount(max_size_ - size_);
    }

    int32_t& size() {
        return size_;
    }

    const int32_t& size() const
    {
        return size_;
    }

    int32_t capacity() const {
        return max_size_ - size_;
    }

    int32_t indexSize() const
    {
        return index_size_;
    }

    int32_t maxSize() const
    {
        return max_size_;
    }

    static int32_t maxSizeFor(int32_t block_size)
    {
        return getMaxSize(block_size);
    }

    static int32_t getMemoryBlockSizeFor(int32_t max)
    {
        int32_t indexSize = getIndexSize(max);
        return (indexSize * sizeof(IndexKey)) * Blocks + getValueBlockSize(max);
    }

    int8_t* memoryBlock()
    {
        return memory_block_;
    }

    const int8_t* memoryBlock() const
    {
        return memory_block_;
    }

    Value* valuesBlock()
    {
        return ptr_cast<Value>(memory_block_ + getValueBlockOffset());
    }

    const Value* valuesBlock() const
    {
        return ptr_cast<const Value>(memory_block_ + getValueBlockOffset());
    }

    IndexKey* indexBlock()
    {
        return ptr_cast<IndexKey>(memory_block_);
    }

    const IndexKey* indexBlock() const
    {
        return ptr_cast<const IndexKey>(memory_block_);
    }

    IndexKey* indexes(int32_t block) {
        return ptr_cast<IndexKey>(memory_block_ + getIndexKeyBlockOffset(block));
    }

    const IndexKey* indexes(int32_t block) const {
        return ptr_cast<const IndexKey>(memory_block_ + getIndexKeyBlockOffset(block));
    }


    int32_t getIndexKeyBlockOffset(int32_t block_num) const
    {
        return sizeof(IndexKey) * index_size_ * block_num;
    }

    int32_t getValueBlockOffset() const
    {
        return getIndexKeyBlockOffset(Blocks);
    }

    IndexKey& indexb(int32_t block_offset, int32_t key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return *ptr_cast<IndexKey>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    const IndexKey& indexb(int32_t block_offset, int32_t key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return *ptr_cast<const IndexKey>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    IndexKey& index(int32_t block_num, int32_t key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        int32_t block_offset = getIndexKeyBlockOffset(block_num);

        return *ptr_cast<IndexKey>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    const IndexKey& index(int32_t block_num, int32_t key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        int32_t block_offset = getIndexKeyBlockOffset(block_num);

        return *ptr_cast<IndexKey>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    IndexKey& maxIndex(int32_t block_num)
    {
        return index(block_num, 0);
    }

    const IndexKey& maxIndex(int32_t block_num) const
    {
        return index(block_num, 0);
    }

    const IndexKey& maxIndexb(int32_t block_offset) const
    {
        return indexb(block_offset, 0);
    }


    int32_t rank(int32_t from, int32_t to, Value symbol) const
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(to, >=, from);
        MEMORIA_ASSERT(to, <, size());

        RankWalker<MyType, Bits> walker(*this, symbol);

        walkRange(from, to + 1, walker);

        return walker.sum();
    }

    int32_t rank(int32_t to, Value symbol) const
    {
        MEMORIA_ASSERT(to, <, size());

        RankWalker<MyType, Bits> walker(*this, symbol);

        walkRange(to + 1, walker);

        return walker.sum();
    }


    int32_t rank1(int32_t from, int32_t to, Value symbol) const
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(to, >=, from);
        MEMORIA_ASSERT(to, <=, size());

        RankWalker<MyType, Bits> walker(*this, symbol);

        walkRange(from, to, walker);

        return walker.sum();
    }

    int32_t rank1(int32_t to, Value symbol) const
    {
        MEMORIA_ASSERT(to, <=, size());

        RankWalker<MyType, Bits> walker(*this, symbol);

        walkRange(to, walker);

        return walker.sum();
    }



    SelectResult selectFW(int32_t from, Value symbol, int32_t rank) const
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(from, <, size());

        intrnl1::EmptyMainWalker mw;
        btree::EmptyExtenderState state;

        sequence::SelectForwardWalker<
            MyType, intrnl1::EmptyMainWalker, btree::EmptyExtender, btree::EmptyExtenderState
        >
        walker(mw, *this, rank, symbol, state);

        size_t idx = findFw(from, walker);

        return SelectResult(idx, walker.sum(), walker.is_found());
    }

    SelectResult selectFW(Value symbol, int32_t rank) const
    {
        intrnl1::EmptyMainWalker mw;
        bt::EmptyExtenderState state;

        sequence::SelectForwardWalker<
             MyType, intrnl1::EmptyMainWalker, btree::EmptyExtender, btree::EmptyExtenderState
        >
        walker(mw, *this, rank, symbol, state);

        size_t idx = findFw(walker);

        return SelectResult(idx, walker.sum(), walker.is_found());
    }


    SelectResult selectBW(int32_t from, Value symbol, int32_t rank) const
    {
        intrnl1::EmptyMainWalker mw;
        bt::EmptyExtenderState state;

        sequence::SelectBackwardWalker<
            MyType, intrnl1::EmptyMainWalker, btree::EmptyExtender, btree::EmptyExtenderState
        >
        walker(mw, *this, rank, symbol, state);

        size_t idx = findBw(from, walker);

        return SelectResult(idx, walker.sum(), walker.is_found());
    }

    IndexKey countFW(int32_t from, Value symbol) const
    {
        intrnl1::EmptyMainWalker mw;
        bt::EmptyExtenderState state;

        sequence::PackedSequenceCountForwardWalker<
            MyType, intrnl1::EmptyMainWalker, btree::EmptyExtender, btree::EmptyExtenderState
        >
        walker(mw, *this, 0, symbol, state);

        findFw(from, walker);

        return walker.sum();
    }

    IndexKey countBW(int32_t from, Value symbol) const
    {
//      CountBWWalker<MyType, Bits> walker(*this, symbol);
//
//      findBw(from, walker);
//
//      return walker.rank();

        intrnl1::EmptyMainWalker mw;
        bt::EmptyExtenderState state;

        sequence::PackedSequenceCountBackwardWalker<
            MyType, intrnl1::EmptyMainWalker, btree::EmptyExtender, btree::EmptyExtenderState
        >
        walker(mw, *this, 0, symbol, state);

        findBw(from, walker);

        return walker.sum();
    }

    void dump(ostream& out_ = cout) const
    {
        out_<<"size_ = "<<size_<<endl;
        out_<<"max_size_ = "<<max_size_<<endl;
        out_<<"index_size_ = "<<index_size_<<endl;

        Expand(out_, 5);
        for (int32_t d = 0; d < Blocks; d++)
        {
            out_.width(5);
            out_<<d;
        }

        out_<<endl<<endl;

        for (int32_t c = 0; c < index_size_; c++)
        {
            out_.width(4);
            out_<<c<<": ";
            for (int32_t d = 0; d < Blocks; d++)
            {
                out_.width(4);
                out_<<this->indexb(this->getIndexKeyBlockOffset(d), c)<<" ";
            }
            out_<<endl;
        }


        int32_t columns;

        switch (Bits) {
        case 1: columns = 100; break;
        case 2: columns = 100; break;
        case 4: columns = 100; break;
        default: columns = 50;
        }

        int32_t width = Bits <= 4 ? 1 : 3;

        int32_t c = 0;

        do
        {
            out_<<endl;
            Expand(out_, 31 - width*5 - (Bits <= 4 ? 2 : 0));
            for (int c = 0; c < columns; c += 5)
            {
                out_.width(width*5);
                out_<<dec<<c;
            }
            out_<<endl;

            int32_t rows = 0;
            for (; c < size() && rows < 10; c += columns, rows++)
            {
                Expand(out_, 12);
                out_<<" ";
                out_.width(6);
                out_<<dec<<c<<" "<<hex;
                out_.width(6);
                out_<<c<<": ";

                for (int32_t d = 0; d < columns && c + d < size(); d++)
                {
                    out_<<hex;
                    out_.width(width);
                    out_<<value(c + d);
                }

                out_<<dec<<endl;
            }
        } while (c < size());
    }

private:

    class ValueSetter {
        MyType& me_;
        int32_t     block_offset_;
        int32_t     idx_;

    public:
        ValueSetter(MyType& me, int32_t block_offset, int32_t idx):
            me_(me),
            block_offset_(block_offset),
            idx_(idx)
        {}

        operator Value() const
        {
            return me_.getValueItem(block_offset_, idx_);
        }

        Value operator=(const Value& v)
        {
            me_.setValueItem(block_offset_, idx_, v);
            return v;
        }
    };



public:
    ValueSetter value(int32_t value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    Value value(int32_t value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    ValueSetter valueb(int32_t block_offset, int32_t value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return ValueSetter(*this, block_offset, value_num);
    }

    Value valueb(int32_t block_offset, int32_t value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return this->getValueItem(block_offset, value_num);
    }

    Value getValueItem(int32_t block_offset, int32_t item_idx) const
    {
        if (Bits == 1 || Bits == 2 || Bits == 4)
        {
            const Value* buffer = ptr_cast<const Value>(memory_block_ + block_offset);
            return GetBits0(buffer, item_idx * Bits, Bits);
        }
        else if (Bits == 8)
        {
            const uint8_t* buffer = ptr_cast<const uint8_t>(memory_block_ + block_offset);
            return buffer[item_idx];
        }
        else
        {
            const Value* buffer = ptr_cast<const Value>(memory_block_ + block_offset);
            return GetBits(buffer, item_idx * Bits, Bits);
        }
    }

    void setValueItem(int32_t block_offset, int32_t item_idx, const Value& v)
    {
        if (Bits == 1 || Bits == 2 || Bits == 4)
        {
            Value* buffer = ptr_cast<Value>(memory_block_ + block_offset);
            SetBits0(buffer, item_idx * Bits, v, Bits);
        }
        else if (Bits == 8)
        {
            uint8_t* buffer = ptr_cast<uint8_t>(memory_block_ + block_offset);
            buffer[item_idx] = v;
        }
        else
        {
            Value* buffer = ptr_cast<Value>(memory_block_ + block_offset);
            SetBits(buffer, item_idx * Bits, v, Bits);
        }
    }

    bool testb(int32_t block_offset, int32_t item_idx, Value value) const
    {
        if (Bits == 1 || Bits == 2 || Bits == 4)
        {
            const Value* buffer = valuesBlock();
            return TestBits(buffer, item_idx * Bits, value, Bits);
        }
        else {
            return valueb(block_offset, item_idx) == value;
        }
    }

    bool test(int32_t item_idx, Value value) const
    {
        if (Bits == 1 || Bits == 2 || Bits == 4)
        {
            const Value* buffer = valuesBlock();
            return TestBits(buffer, item_idx * Bits, value, Bits);
        }
        else {
            int32_t block_offset = getValueBlockOffset();
            return valueb(block_offset, item_idx) == value;
        }
    }

    const Value* cellAddr(int32_t idx) const
    {
        return ptr_cast<const Value>(valuesBlock() + idx);
    }

    Value* cellAddr(int32_t idx)
    {
        return ptr_cast<Value>(valuesBlock() + idx);
    }

    void copyTo(MyType* other, int32_t copy_from, int32_t count, int32_t copy_to) const
    {
        MEMORIA_ASSERT(copy_from, >=, 0);
        MEMORIA_ASSERT(copy_from + count, <=, max_size_);

        MEMORIA_ASSERT(copy_to, >=, 0);

        MEMORIA_ASSERT(copy_to + count, <=, other->max_size_);

        const Value* src    = valuesBlock();
        Value* dst          = other->valuesBlock();

        MoveBits(src, dst, copy_from * Bits, copy_to * Bits, count * Bits);
    }

    void clearValues(int32_t from, int32_t to)
    {
        int32_t block_offset = this->getValueBlockOffset();

        for (int32_t idx = from; idx < to; idx++)
        {
            valueb(block_offset, idx) = 0;
        }
    }

    void clear(int32_t from, int32_t to)
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(to, <=, max_size_);
        MEMORIA_ASSERT(from, <=, to);

        clearValues(from, to);
    }

    void clearIndex(int32_t block)
    {
        for (int32_t idx = 0; idx < indexSize(); idx++)
        {
            index(block, idx) = 0;
        }
    }

    void clearIndexes()
    {
        for (int32_t b = 0; b < Blocks; b++)
        {
            clearIndex(b);
        }
    }

    void clearUnusedData()
    {
        clearValues(size(), maxSize());
    }

    void clearUnused()
    {
        clearUnusedData();
        clearIndexes();
    }

    void enlargeBlock(int32_t block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        transferTo(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void shrinkBlock(int32_t block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        transferTo(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void transferTo(MyType* other, int8_t* memory_block = nullptr) const
    {
        MEMORIA_ASSERT(size(), <=, other->maxSize());
#ifndef __clang__
        MEMORIA_ASSERT(Blocks,  ==, other->Blocks);
#endif
        if (memory_block == nullptr)
        {
            memory_block = other->memory_block_;
        }

        copyValuesBlock(other, memory_block);
    }

    void insertSpace(int32_t room_start, int32_t room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, max_size_);
        MEMORIA_ASSERT(room_start,  <=, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(size_ + room_length, <=, max_size_);

        copyTo(this, room_start, size() - room_start, room_start + room_length);

        size_ += room_length;
    }

    void removeSpace(int32_t room_start, int32_t room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(room_start + room_length, <=, size_);

        int32_t copy_from = room_start + room_length;

        copyTo(this, copy_from, size() - copy_from, room_start);

        size_ -= room_length;
    }

    void add(int32_t block_num, int32_t idx, const IndexKey& value)
    {
        int32_t index_block_offset  = getIndexKeyBlockOffset(block_num);

        int32_t index_level_size    = getIndexCellsNumberFor(max_size_);
        int32_t index_level_start   = index_size_ - index_level_size;

        int32_t level = 0;
        while (index_level_start >= 0)
        {
            idx /= BranchingFactor;

            indexb(index_block_offset, idx + index_level_start) += value;

            int32_t index_parent_size   = getIndexCellsNumberFor(level, index_level_size);

            index_level_size        = index_parent_size;
            index_level_start       -= index_parent_size;

            level++;
        }
    }


    void insert(const Value& val, int32_t at)
    {
        if (at < size_ - 1)
        {
            insertSpace(at, 1);
        }

        value(at) = val;
    }


    void updateUp(int32_t block_num, int32_t idx, IndexKey key_value)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size());

        int32_t level_size      = maxSize();
        int32_t level_start     = indexSize();

        int32_t block_offset    = getIndexKeyBlockOffset(block_num);

        int32_t level           = 0;

        do {
            level_size      = getIndexCellsNumberFor(level, level_size);
            level_start     -= level_size;

            if (level > 0) {
                idx /= BranchingFactor;
            }
            else {
                idx /= ValuesPerBranch;
            }

            indexb(block_offset, idx + level_start) += key_value;
        }
        while (level_start > 0);
    }

    void reindex(int32_t start, int32_t end)
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, <=, size());
        MEMORIA_ASSERT(start, <=, end);

        int32_t block_start = getBlockStartV(start);
        int32_t block_end   = getBlockEndV(end);

        int32_t value_block_offset  = getValueBlockOffset();

        int32_t index_level_size    = getIndexCellsNumberFor(0, maxSize());
        int32_t index_level_start   = indexSize() - index_level_size;

        int32_t level_max           = size();

        if (Bits < 3)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                int32_t index_block_offset  = getIndexKeyBlockOffset(block);

                for (int32_t c = block_start; c < block_end; c += ValuesPerBranch)
                {
                    int32_t max      = c + ValuesPerBranch <= level_max ? c + ValuesPerBranch : level_max;

                    IndexKey sum = popCount(value_block_offset, c, max, block);

                    int32_t idx = c / ValuesPerBranch + index_level_start;
                    indexb(index_block_offset, idx) = sum;
                }
            }
        }
        else {
            for (int32_t c = block_start; c < block_end; c += ValuesPerBranch)
            {
                int32_t max      = c + ValuesPerBranch <= level_max ? c + ValuesPerBranch : level_max;

                IndexKey sums[Blocks];
                for (int32_t block = 0; block < Blocks; block++) sums[block] = 0;

                for (int32_t d = c; d < max; d++)
                {
                    Value symbol = valueb(value_block_offset, d);
                    sums[symbol]++;
                }

                int32_t idx = c / ValuesPerBranch + index_level_start;

                for (int32_t block = 0; block < Blocks; block++)
                {
                    index(block, idx) = sums[block];
                }
            }
        }


        for (int32_t block = 0; block < Blocks; block ++)
        {
            int32_t block_start0        = block_start;
            int32_t block_end0          = block_end;
            int32_t level_max0          = level_max;
            int32_t index_level_start0  = index_level_start;
            int32_t index_level_size0   = index_level_size;


            int32_t level = 0;
            int32_t index_block_offset = getIndexKeyBlockOffset(block);

            int32_t divider = ValuesPerBranch;

            while (index_level_start0 > 0)
            {
                level_max0      = getIndexCellsNumberFor(level, level_max0);
                block_start0    = getBlockStart(block_start0 / divider);
                block_end0      = getBlockEnd(block_end0 / divider);

                int32_t index_parent_size   = getIndexCellsNumberFor(level + 1, index_level_size0);
                int32_t index_parent_start  = index_level_start0 - index_parent_size;

                for (int32_t c = block_start0; c < block_end0; c += BranchingFactor)
                {
                    IndexKey sum = 0;
                    int32_t max      = (c + BranchingFactor <= level_max0 ? c + BranchingFactor : level_max0)
                                    + index_level_start0;

                    for (int32_t d = c + index_level_start0; d < max; d++)
                    {
                        sum += indexb(index_block_offset, d);
                    }

                    int32_t idx = c / BranchingFactor + index_parent_start;
                    indexb(index_block_offset, idx) = sum;
                }

                index_level_size0    = index_parent_size;
                index_level_start0   -= index_parent_size;

                level++;

                divider = BranchingFactor;
            }
        }
    }

    void reindex()
    {
        clearIndexes();
        reindex(0, size());
    }


    IndexKey popCount(int32_t start, int32_t end, Value symbol) const
    {
        int32_t block_offset = getValueBlockOffset();
        return popCount(block_offset, start, end, symbol);
    }

    IndexKey popCount(int32_t block_offset, int32_t start, int32_t end, Value symbol) const
    {
        if (Bits == 1)
        {
            const Value* buffer = this->valuesBlock();
            size_t rank = PopCount(buffer, start, end);
            if (symbol) {
                return rank;
            }
            else {
                return end - start - rank;
            }
        }
        else {
            IndexKey total = 0;

            for (int32_t c = start; c < end; c++)
            {
                total += testb(block_offset, c, symbol);
            }

            return total;
        }
    }

private:
    template <typename Walker>
    class FinishHandler {
        Walker& walker_;
    public:
        FinishHandler(Walker& walker): walker_(walker) {}

        ~FinishHandler()
        {
            walker_.finish();
        }
    };

public:
    template <typename Functor>
    void walkRange(int32_t start, int32_t end, Functor& walker) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end,   <=,  size());
        MEMORIA_ASSERT(start, <=,  end);

        FinishHandler<Functor> finish_handler(walker);

        if (end - start <= ValuesPerBranch * 2)
        {
            walker.walkValues(start, end);
        }
        else {
            int32_t block_start_end     = getBlockStartEndV(start);
            int32_t block_end_start     = getBlockStartV(end);

            walker.walkValues(start, block_start_end);

            if (block_start_end < block_end_start)
            {
                int32_t level_size = getIndexCellsNumberFor(0, max_size_);
                walker.prepareIndex();
                walkIndexRange(
                        start / ValuesPerBranch + 1,
                        end / ValuesPerBranch,
                        walker,
                        index_size_ - level_size,
                        level_size,
                        ValuesPerBranch
                );
            }

            walker.walkValues(block_end_start, end);
        }
    }

    template <typename Walker>
    void walkRange(int32_t target, Walker& walker) const
    {
        MEMORIA_ASSERT(target,   <=,  size());

        FinishHandler<Walker> finish_handler(walker);

        int32_t levels = 0;
        int32_t level_sizes[LEVELS_MAX];

        int32_t level_size = max_size_;
        int32_t cell_size = 1;

        do
        {
            level_size = getIndexCellsNumberFor(levels, level_size);
            level_sizes[levels++] = level_size;
        }
        while (level_size > 1);

        cell_size = ValuesPerBranch;
        for (int32_t c = 0; c < levels - 2; c++)
        {
            cell_size *= BranchingFactor;
        }

        int32_t base = 1, start = 0, target_idx = target;

        for (int32_t level = levels - 2; level >= 0; level--)
        {
            int32_t end = target_idx / cell_size;

            walker.walkIndex(start + base, end + base);

            start       = level > 0 ? end * BranchingFactor : end * ValuesPerBranch;
            base        += level_sizes[level];
            cell_size   /= BranchingFactor;
        }

        return walker.walkValues(start, target);
    }


    template <typename Walker>
    int32_t findFw(Walker &walker) const
    {
        FinishHandler<Walker> finish_handler(walker);

        int32_t levels = 0;
        int32_t level_sizes[LEVELS_MAX];

        int32_t level_size = max_size_;

        do
        {
            level_size = getIndexCellsNumberFor(levels, level_size);
            level_sizes[levels++] = level_size;
        }
        while (level_size > 1);

        int32_t base = 1, start = 0;

        for (int32_t level = levels - 2; level >= 0; level--)
        {
            int32_t level_size  = level_sizes[level];
            int32_t end         = (start + BranchingFactor < level_size) ? (start + BranchingFactor) : level_size;

            int32_t idx = walker.walkIndex(start + base, end + base, 0) - base;
            if (idx < end)
            {
                start = level > 0 ? idx * BranchingFactor : idx * ValuesPerBranch;
            }
            else {
                return size_;
            }

            base += level_size;
        }

        int32_t end = (start + ValuesPerBranch) > size_ ? size_ : start + ValuesPerBranch;

        return walker.walkValues(start, end);
    }



    template <typename Walker>
    int32_t findFw(int32_t start, Walker& walker) const
    {
        MEMORIA_ASSERT(start, <=, size());

        FinishHandler<Walker> finish_handler(walker);

        int32_t block_limit     = getBlockStartEndV(start);

        if (block_limit >= size())
        {
            return walker.walkValues(start, size());
        }
        else
        {
            int32_t limit = walker.walkValues(start, block_limit);
            if (limit < block_limit)
            {
                return limit;
            }
            else {
                walker.prepareIndex();

                int32_t level_size      = getIndexCellsNumberFor(0, max_size_);
                int32_t level_limit     = getIndexCellsNumberFor(0, size_);
                int32_t last_start      = walkIndexFw(
                        block_limit/ValuesPerBranch,
                        walker,
                        index_size_ - level_size,
                        level_size,
                        level_limit,
                        ValuesPerBranch,
                        ValuesPerBranch
                );

                if (last_start < size())
                {
                    int32_t last_start_end  = getBlockStartEndV(last_start);

                    int32_t last_end = last_start_end <= size()? last_start_end : size();

                    return walker.walkValues(last_start, last_end);
                }
                else {
                    return size();
                }
            }
        }
    }

    template <typename Walker>
    size_t findBw(int32_t start, Walker& walker) const
    {
        MEMORIA_ASSERT(start, >=, 0);

        FinishHandler<Walker> finish_handler(walker);

        int32_t block_end   = getBlockStartEndBwV(start);

        if (block_end == 0)
        {
            return walker.walkValues(start, 0);
        }
        else
        {
            int32_t limit = walker.walkValues(start, block_end);
            if (walker.is_found())
            {
                return limit;
            }
            else {
                walker.prepareIndex();

                int32_t level_size = getIndexCellsNumberFor(0, max_size_);
                int32_t last_start = walkIndexBw(
                                    block_end/ValuesPerBranch - 1,
                                    walker,
                                    index_size_ - level_size,
                                    level_size,
                                    ValuesPerBranch,
                                    ValuesPerBranch
                                 );

                if (last_start > 0)
                {
                    return walker.walkValues(last_start, last_start - ValuesPerBranch);
                }
                else {
                    return 0;
                }
            }
        }
    }

protected:
    static int32_t getBlockStart(int32_t i)
    {
        return (i / BranchingFactor) * BranchingFactor;
    }

    static int32_t getBlockStartEnd(int32_t i)
    {
        return (i / BranchingFactor + 1) * BranchingFactor;
    }

    static int32_t getBlockStartV(int32_t i)
    {
        return (i / ValuesPerBranch) * ValuesPerBranch;
    }

    static int32_t getBlockStartEndV(int32_t i)
    {
        return (i / ValuesPerBranch + 1) * ValuesPerBranch;
    }

    static int32_t getBlockStartEndBw(int32_t i)
    {
        return (i / BranchingFactor) * BranchingFactor - 1;
    }

    static int32_t getBlockStartEndBwV(int32_t i)
    {
        return (i / ValuesPerBranch) * ValuesPerBranch;
    }

    static int32_t getBlockEnd(int32_t i)
    {
        return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
    }

    static int32_t getBlockEndV(int32_t i)
    {
        return (i / ValuesPerBranch + ((i % ValuesPerBranch) ? 1 : 0)) * ValuesPerBranch;
    }

    static int32_t getIndexCellsNumberFor(int32_t i)
    {
        return getIndexCellsNumberFor(1, i);
    }

    static int32_t getIndexCellsNumberFor(int32_t level, int32_t i)
    {
        if (level > 0)
        {
            return i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0);
        }
        else {
            return i / ValuesPerBranch + ((i % ValuesPerBranch) ? 1 : 0);
        }
    }

private:

    template <typename Functor>
    void walkIndexRange(int32_t start, int32_t end, Functor& walker, int32_t level_offet, int32_t level_size, int32_t cell_size) const
    {
        if (end - start <= BranchingFactor * 2)
        {
            walker.walkIndex(start + level_offet, end + level_offet);
        }
        else {
            int32_t block_start_end     = getBlockStartEnd(start);
            int32_t block_end_start     = getBlockStart(end);

            walker.walkIndex(start + level_offet, block_start_end + level_offet);

            if (block_start_end < block_end_start)
            {
                int32_t level_size0 = getIndexCellsNumberFor(level_size);
                walkIndexRange(
                        start / BranchingFactor + 1,
                        end / BranchingFactor,
                        walker,
                        level_offet - level_size0,
                        level_size0,
                        BranchingFactor
                );
            }

            walker.walkIndex(block_end_start + level_offet, end + level_offet);
        }
    }

    template <typename Walker>
    int32_t walkIndexFw(
            int32_t start,
            Walker& walker,
            int32_t level_offet,
            int32_t level_size,
            int32_t level_limit,
            int32_t cells_number_on_lower_level,
            int32_t cell_size
    ) const
    {
        int32_t block_start_end     = getBlockStartEnd(start);

        if (block_start_end >= level_limit)
        {
            return (walker.walkIndex(
                                start + level_offet,
                                level_limit + level_offet,
                                cell_size
                           )
                           - level_offet) * cells_number_on_lower_level;
        }
        else
        {
            int32_t limit = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
            if (limit < block_start_end)
            {
                return limit * cells_number_on_lower_level;
            }
            else {
                int32_t level_size0     = getIndexCellsNumberFor(level_size);
                int32_t level_limit0    = getIndexCellsNumberFor(level_limit);

                int32_t last_start      = walkIndexFw(
                                        block_start_end / BranchingFactor,
                                        walker,
                                        level_offet - level_size0,
                                        level_size0,
                                        level_limit0,
                                        BranchingFactor,
                                        cell_size * BranchingFactor
                                      );

                int32_t last_start_end  = getBlockStartEnd(last_start);

                int32_t last_end = last_start_end <= level_limit ? last_start_end : level_limit;

                return (walker.walkIndex(
                                    last_start + level_offet,
                                    last_end + level_offet,
                                    cell_size
                               )
                               - level_offet) * cells_number_on_lower_level;
            }
        }
    }

    template <typename Walker>
    int32_t walkIndexBw(
            int32_t start,
            Walker& walker,
            int32_t level_offet,
            int32_t level_size,
            int32_t cells_number_on_lower_level,
            int32_t cell_size
    ) const
    {
        int32_t block_start_end     = getBlockStartEndBw(start);

        if (block_start_end == -1)
        {
            return (walker.walkIndex(
                                start + level_offet,
                                level_offet - 1,
                                cell_size
                           )
                           - level_offet + 1) * cells_number_on_lower_level;
        }
        else
        {
            int32_t idx = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
            if (idx > block_start_end)
            {
                return (idx + 1) * cells_number_on_lower_level;
            }
            else {
                int32_t level_size0 = getIndexCellsNumberFor(level_size);
                int32_t last_start  = walkIndexBw(
                                    block_start_end / BranchingFactor,
                                    walker,
                                    level_offet - level_size0,
                                    level_size0,
                                    BranchingFactor,
                                    cell_size * BranchingFactor
                                  ) - 1;

                int32_t last_start_end = getBlockStartEndBw(last_start);

                return (walker.walkIndex(
                                    last_start + level_offet,
                                    last_start_end + level_offet,
                                    cell_size
                               )
                               - level_offet + 1) * cells_number_on_lower_level;
            }
        }
    }


    void copyValuesBlock(MyType* other, int8_t* target_memory_block) const
    {
        Value* tgt = ptr_cast<Value>(target_memory_block + other->getValueBlockOffset());

        CopyBuffer(valuesBlock(), tgt, getValueCellsCount(size()));
    }

    static int32_t getBlockSize(int32_t item_count)
    {
        return getIndexSize(item_count) * sizeof(IndexKey) * Blocks + getValueBlockSize(item_count);
    }

    static int32_t getIndexSize(int32_t csize)
    {
        if (csize == 1)
        {
            return 1;
        }
        else {
            int32_t sum = 0;
            for (int32_t nlevels=0; csize > 1; nlevels++)
            {
                if (nlevels > 0) {
                    csize = ((csize % BranchingFactor) == 0) ? (csize / BranchingFactor) : (csize / BranchingFactor) + 1;
                }
                else {
                    csize = ((csize % ValuesPerBranch) == 0) ? (csize / ValuesPerBranch) : (csize / ValuesPerBranch) + 1;
                }
                sum += csize;
            }
            return sum;
        }
    }

    static int32_t getMaxSize(int32_t block_size)
    {
        int32_t first       = 1;
        int32_t last        = block_size * 8 / Bits;

        while (first < last - 1)
        {
            int32_t middle = (first + last) / 2;

            int32_t size = getBlockSize(middle);
            if (size < block_size)
            {
                first = middle;
            }
            else if (size > block_size)
            {
                last = middle;
            }
            else {
                break;
            }
        }

        int32_t max_size;

        if (getBlockSize(last) <= block_size)
        {
            max_size = last;
        }
        else if (getBlockSize((first + last) / 2) <= block_size)
        {
            max_size = (first + last) / 2;
        }
        else {
            max_size = first;
        }

        int32_t cells = getValueCellsCount(max_size);
        int32_t max = cells * TypeBitsize<Value>() / Bits;

        if (getIndexSize(max) <= getIndexSize(max_size))
        {
            return max;
        }

        return max_size;
    }
};

}
