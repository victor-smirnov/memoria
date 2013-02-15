
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_TREE_HPP_
#define MEMORIA_CORE_PMAP_PACKED_TREE_HPP_

#include <memoria/core/container/page_traits.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/md5.hpp>

#include <memoria/core/tools/reflection.hpp>

#include <functional>
#include <algorithm>

namespace memoria {


namespace intrnl0 {

template <typename T>
struct ValueHelper {
    static void setup(IPageDataEventHandler* handler, const T& value)
    {
        handler->value("VALUE", &value);
    }
};

template <typename T>
struct ValueHelper<PageID<T> > {
    typedef PageID<T>                                                   Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        IDValue id(&value);
        handler->value("VALUE", &id);
    }
};

template <>
struct ValueHelper<EmptyValue> {
    typedef EmptyValue Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        BigInt val = 0;
        handler->value("VALUE", &val);
    }
};


}

template <typename K, typename IK, typename V, typename Acc, Int Blocks_ = 1, Int BF = PackedTreeBranchingFactor>
struct PackedTreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef Acc             Accumulator;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
};

template <typename Types>
class PackedTree {

    static const UInt VERSION               = 1;

    typedef PackedTree<Types>               MyType;

public:

    typedef typename Types::Key             Key;
    typedef typename Types::IndexKey        IndexKey;
    typedef typename Types::Value           Value;
    typedef typename Types::Accumulator     Accumulator;

    static const Int Blocks                 = Types::Blocks;
    static const Int BranchingFactor        = Types::BranchingFactor;

    template <typename T> friend class PackedTree;

private:

    static const Int LEVELS_MAX             = 32;

    Int     size_;
    Int     max_size_;
    Int     index_size_;
    Byte    memory_block_[];


public:

    typedef TypeList<
            ConstValue<UInt, VERSION>,
            decltype(size_),
            decltype(max_size_),
            decltype(index_size_),
            Key,
            IndexKey,
            Value
    >                                                                           FieldsList;

    PackedTree() {}


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_TREE");

        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);
        handler->value("INDEX_SIZE",    &index_size_);

        handler->startGroup("INDEXES", index_size_);

        for (Int idx = 0; idx < index_size_; idx++)
        {
            IndexKey indexes[Blocks];
            for (Int block = 0; block < Blocks; block++)
            {
                indexes[block] = index(block, idx);
            }

            handler->value("INDEX", indexes, Blocks);
        }

        handler->endGroup();

        handler->startGroup("DATA", size_);

        for (Int idx = 0; idx < size_; idx++)
        {
            handler->startLine("ENTRY");

            Key keys[Blocks];
            for (Int block = 0; block < Blocks; block++)
            {
                keys[block] = key(block, idx);
            }

            handler->value(Blocks == 1 ? "KEY" : "KEYS", keys, Blocks);

            if (getValueSize() > 0)
            {
                intrnl0::ValueHelper<Value>::setup(handler, value(idx));
            }

            handler->endLine();
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, size());
        FieldFactory<Int>::serialize(buf, max_size_);
        FieldFactory<Int>::serialize(buf, index_size_);

        FieldFactory<IndexKey>::serialize(buf, index(0, 0), Blocks * indexSize());

        for (Int c = 0; c < Blocks; c++)
        {
            FieldFactory<Key>::serialize(buf, key(c, 0), size());
        }

        if (getValueSize() > 0)
        {
            FieldFactory<Value>::serialize(buf, value(0), size());
        }
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, size());
        FieldFactory<Int>::deserialize(buf, max_size_);
        FieldFactory<Int>::deserialize(buf, index_size_);

        FieldFactory<IndexKey>::deserialize(buf, index(0, 0), Blocks * indexSize());

        for (Int c = 0; c < Blocks; c++)
        {
            FieldFactory<Key>::deserialize(buf, key(c, 0), size());
        }

        if (getValueSize() > 0)
        {
            FieldFactory<Value>::deserialize(buf, value(0), size());
        }
    }

    void initByBlock(Int block_size)
    {
        size_ = 0;

        max_size_   = getMaxSize(block_size);
        index_size_ = getIndexSize(max_size_);
    }


    void initSizes(Int max)
    {
        size_       = 0;
        max_size_   = max;
        index_size_ = getIndexSize(max_size_);
    }

    Int getObjectSize() const
    {
        return sizeof(MyType) + getBlockSize();
    }

    Int getObjectDataSize() const
    {
        return sizeof(size_) + sizeof(max_size_) + sizeof(index_size_) + getBlockSize();
    }


    Int getBlockSize() const
    {
        return (index_size_ * sizeof(IndexKey) + max_size_ * sizeof(Key)) * Blocks + max_size_ * getValueSize();
    }

    Int getDataSize() const
    {
        return (index_size_ * sizeof(IndexKey) + size_ * sizeof(Key)) * Blocks + size_ * getValueSize();
    }

    Int getTotalDataSize() const
    {
        return (index_size_ * sizeof(IndexKey) + max_size_ * sizeof(Key)) * Blocks + max_size_ * getValueSize();
    }

    Byte* getKeysPtr(Byte* memory_block) const {
        return memory_block + index_size_ * sizeof(IndexKey) * Blocks;
    }

    Byte* getKeysPtr() {
        return getKeysPtr(memory_block_);
    }

    const Byte* getKeysPtr() const {
        return memory_block_ + index_size_ * sizeof(IndexKey) * Blocks;
    }

    Key* keys(Int block) {
    	return T2T<Key*>(memory_block_ + getKeyBlockOffset(block));
    }

    const Key* keys(Int block) const {
    	return T2T<const Key*>(memory_block_ + getKeyBlockOffset(block));
    }

    IndexKey* indexes(Int block) {
    	return T2T<IndexKey*>(memory_block_ + getIndexKeyBlockOffset(block));
    }

    const IndexKey* indexes(Int block) const {
    	return T2T<const IndexKey*>(memory_block_ + getIndexKeyBlockOffset(block));
    }

    Accumulator keysAt(Int idx) const
    {
    	Accumulator acc;

    	for (Int c = 0; c < Blocks; c++)
    	{
    		acc[c] = key(c, idx);
    	}

    	return acc;
    }


    Int getKeysSize() const {
        return max_size_ * sizeof(Key) * Blocks;
    }

    Int& size() {
        return size_;
    }

    const Int& size() const
    {
        return size_;
    }

    Int indexSize() const
    {
        return index_size_;
    }

    Int maxSize() const
    {
        return max_size_;
    }

    static Int maxSizeFor(Int block_size)
    {
        return getMaxSize(block_size);
    }

    static Int getMemoryBlockSizeFor(Int max)
    {
        Int indexSize = getIndexSize(max);
        return (indexSize * sizeof(IndexKey) + max * sizeof(Key)) * Blocks + max * getValueSize();
    }

    Byte* memoryBlock() {
        return memory_block_;
    }

    const Byte* memoryBlock() const {
        return memory_block_;
    }

    Int getIndexKeyBlockOffset(Int block_num) const
    {
        return sizeof(IndexKey) * index_size_ * block_num;
    }

    Int getKeyBlockOffset(Int block_num) const
    {
        return getIndexKeyBlockOffset(Blocks) + sizeof(Key) * max_size_ * block_num;
    }

    Int getValueBlockOffset() const
    {
        return getKeyBlockOffset(Blocks);
    }

    template <typename T>
    T& blockItem(Int block_offset, Int item_idx, Int item_size = sizeof(T))
    {
        return *T2T<T*>(memory_block_ + block_offset + item_idx * item_size);
    }

    template <typename T>
    T& blockItem(Byte* memory_block, Int block_offset, Int item_idx, Int item_size = sizeof(T)) const
    {
        return *T2T<T*>(memory_block + block_offset + item_idx * item_size);
    }

    template <typename T>
    const T& blockItem(Int block_offset, Int item_idx, Int item_size = sizeof(T)) const
    {
        return *T2T<const T*>(memory_block_ + block_offset + item_idx * item_size);
    }

    IndexKey& indexb(Int block_offset, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return blockItem<IndexKey>(block_offset, key_num);
    }

    const IndexKey& indexb(Int block_offset, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return blockItem<IndexKey>(block_offset, key_num);
    }

    IndexKey& index(Int block_num, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return blockItem<IndexKey>(getIndexKeyBlockOffset(block_num), key_num);
    }

    const IndexKey& index(Int block_num, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return blockItem<IndexKey>(getIndexKeyBlockOffset(block_num), key_num);
    }


    Key& keyb(Int block_offset, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, max_size_);

        return blockItem<Key>(block_offset, key_num);
    }

    Key& keyb(Byte* memory_block, Int block_offset, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, max_size_);

        return blockItem<Key>(memory_block, block_offset, key_num);
    }

    const Key& keyb(Int block_offset, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, max_size_);

        return blockItem<Key>(block_offset, key_num);
    }

    Key& key(Int block_num, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, max_size_);

        return blockItem<Key>(getKeyBlockOffset(block_num), key_num);
    }

    const Key& key(Int block_num, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, max_size_);

        return blockItem<Key>(getKeyBlockOffset(block_num), key_num);
    }

    IndexKey& maxKey(Int block_num)
    {
        return index(block_num, 0);
    }

    const IndexKey& maxKey(Int block_num) const
    {
        return index(block_num, 0);
    }

    const IndexKey& maxKeyb(Int block_offset) const
    {
        return indexb(block_offset, 0);
    }

    Accumulator maxKeys() const
    {
    	Accumulator acc;

    	for (Int c = 0; c < Blocks; c++)
    	{
    		acc[c] = maxKey(c);
    	}

    	return acc;
    }

    Value& value(Int value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    const Value& value(Int value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    Value& valueb(Int block_offset, Int value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return blockItem<Value>(block_offset, value_num, getValueSize());
    }

    Value& valueb(Byte* memory_block, Int block_offset, Int value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return blockItem<Value>(memory_block, block_offset, value_num, getValueSize());
    }

    const Value& valueb(Int block_offset, Int value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return blockItem<Value>(block_offset, value_num, getValueSize());
    }

    static Int getValueSize()
    {
        return ValueTraits<Value>::Size;
    }

    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_ASSERT(copy_from, >=, 0);
        MEMORIA_ASSERT(copy_from + count, <=, max_size_);

        MEMORIA_ASSERT(copy_to, >=, 0);

        MEMORIA_ASSERT(copy_to + count, <=, other->max_size_);

        for (Int c = 0; c < Blocks; c++)
        {
            Int src_block_offset = this->getKeyBlockOffset(c)  + copy_from * sizeof(Key);
            Int tgt_block_offset = other->getKeyBlockOffset(c) + copy_to * sizeof(Key);

            CopyBuffer(memory_block_ + src_block_offset, other->memory_block_ + tgt_block_offset, count * sizeof(Key));
        }

        if (this->getValueSize() > 0)
        {
            Int src_block_offset = this->getValueBlockOffset()  + copy_from * getValueSize();
            Int tgt_block_offset = other->getValueBlockOffset() + copy_to * getValueSize();

            CopyBuffer(memory_block_ + src_block_offset, other->memory_block_ + tgt_block_offset, count * getValueSize());
        }
    }

    void clearKeys(Int block, Int from, Int to)
    {
        Int block_offset = this->getKeyBlockOffset(block);

        for (Int idx = from; idx < to; idx++)
        {
            keyb(block_offset, idx) = 0;
        }
    }

    void clearValues(Int from, Int to)
    {
        if (this->getValueSize() > 0)
        {
            Int block_offset = this->getValueBlockOffset();
            for (Int idx = from; idx < to; idx++)
            {
                valueb(block_offset, idx) = 0;
            }
        }
    }

    void clear(Int from, Int to)
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(to, <=, max_size_);
        MEMORIA_ASSERT(from, <=, to);

        for (Int c = 0; c < Blocks; c++)
        {
            clearKeys(c, from, to);
        }

        clearValues(from, to);
    }

    void clearIndex(Int block)
    {
        for (Int idx = 0; idx < indexSize(); idx++)
        {
            index(block, idx) = 0;
        }
    }

    void clearIndexes()
    {
        for (Int b = 0; b < Blocks; b++)
        {
            clearIndex(b);
        }
    }

    void clearUnusedData()
    {
        for (Int b = 0; b < Blocks; b++)
        {
            clearKeys(b, size(), maxSize());
        }

        clearValues(size(), maxSize());
    }

    void clearUnused() {
        clearUnusedData();
        clearIndexes();
    }

    void enlargeBlock(Int block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        repackDataWithSameTypes(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void enlargeTo(MyType* other)
    {
        repackDataWithSameTypes(other, other->memory_block_);
    }

    void shrinkBlock(Int block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        repackDataWithSameTypes(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void shrinkTo(MyType* other)
    {
        repackDataWithSameTypes(other, other->memory_block_);
    }

    template <typename TreeType>
    void transferTo(TreeType* other, Byte* memory_block = nullptr) const
    {
        MEMORIA_ASSERT(size(), <=, other->maxSize());
        MEMORIA_ASSERT(Blocks,  ==, other->Blocks);

        if (memory_block == nullptr)
        {
            memory_block = other->memory_block_;
        }

        if (
                sizeof(IndexKey) == sizeof(typename TreeType::IndexKey) &&
                sizeof(Key) == sizeof(typename TreeType::Key) &&
                getValueSize() == TreeType::getValueSize())
        {
            repackDataWithSameTypes(other, memory_block);
        }
        else {
            repackDataWithDifferentTypes(other, memory_block);
        }
    }

    void insertSpace(Int room_start, Int room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, max_size_);
        MEMORIA_ASSERT(room_start,  <=, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(size_ + room_length, <=, max_size_);

        Int value_size = getValueSize();

        if (value_size > 0)
        {
            Int offset = getValueBlockOffset();

            copyData(offset, room_start, room_length, value_size);
        }

        for (Int c = Blocks - 1; c >= 0; c--)
        {
            Int offset = getKeyBlockOffset(c);

            copyData(offset, room_start, room_length, sizeof(Key));
        }

        size_ += room_length;
    }

    void removeSpace(Int room_start, Int room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(room_start + room_length, <=, size_);

        Int value_size = getValueSize();

        for (Int c = 0; c < Blocks; c++)
        {
            Int offset = getKeyBlockOffset(c);

            copyData(offset, room_start + room_length, -room_length, sizeof(Key));
        }

        if (value_size > 0)
        {
            Int offset = getValueBlockOffset();

            copyData(offset, room_start + room_length, -room_length, value_size);
        }

        size_ -= room_length;
    }

    void add(Int block_num, Int idx, const Key& value)
    {
        key(block_num, idx) += value;

        Int index_block_offset  = getIndexKeyBlockOffset(block_num);

        Int index_level_size    = getIndexCellsNumberFor(max_size_);
        Int index_level_start   = index_size_ - index_level_size;

        while (index_level_start >= 0)
        {
            idx /= BranchingFactor;

            indexb(index_block_offset, idx + index_level_start) += value;

            Int index_parent_size   = getIndexCellsNumberFor(index_level_size);

            index_level_size        = index_parent_size;
            index_level_start       -= index_parent_size;
        }
    }


    void insert(const Accumulator& keys, const Value& val, Int at)
    {
        if (at < size_ - 1)
        {
            insertSpace(at, 1);
        }

        for (Int c = 0; c < Blocks; c++)
        {
            key(c, at) = keys[c];
        }

        value(at) = val;
    }


    template <typename Walker>
    Int find(Walker &walker) const
    {
    	Int levels = 0;
    	Int level_sizes[LEVELS_MAX];

    	Int level_size = max_size_;

    	do
    	{
    		level_size = getIndexCellsNumberFor(level_size);
    		level_sizes[levels++] = level_size;
    	}
    	while (level_size > 1);

    	Int base = 1, start = 0;

    	for (Int level = levels - 2; level >= 0; level--)
    	{
    		Int level_size  = level_sizes[level];
    		Int end         = (start + BranchingFactor < level_size) ? (start + BranchingFactor) : level_size;

    		Int idx = walker.walkIndex(start + base, end + base) - base;
    		if (idx < end)
    		{
    			start = idx * BranchingFactor;
    		}
    		else {
    			return size_;
    		}

    		base += level_size;
    	}

    	Int end = (start + BranchingFactor) > size_ ? size_ : start + BranchingFactor;

    	return walker.walkKeys(start, end);
    }


    template <typename Functor>
    void walkRange(Int start, Int end, Functor& walker) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end,   <=,  size());
        MEMORIA_ASSERT(start, <=,  end);

        if (end - start <= BranchingFactor * 2)
        {
            walker.walkKeys(start, end);
        }
        else {
            Int block_start_end     = getBlockStartEnd(start);
            Int block_end_start     = getBlockStart(end);

            walker.walkKeys(start, block_start_end);

            if (block_start_end < block_end_start)
            {
                Int level_size = getIndexCellsNumberFor(max_size_);
                walker.prepareIndex();
                walkIndexRange(
                        start/BranchingFactor + 1,
                        end/BranchingFactor,
                        walker,
                        index_size_ - level_size,
                        level_size
                );
            }

            walker.walkKeys(block_end_start, end);
        }
    }




    template <typename Walker>
    Int findFw(Int start, Walker& walker) const
    {
        MEMORIA_ASSERT(start, <=, size());

        Int block_limit     = getBlockStartEnd(start);

        if (block_limit >= size())
        {
            return walker.walkKeys(start, size());
        }
        else
        {
            Int limit = walker.walkKeys(start, block_limit);
            if (limit < block_limit)
            {
                return limit;
            }
            else {
                walker.prepareIndex();

                Int level_size      = getIndexCellsNumberFor(max_size_);
                Int level_limit     = getIndexCellsNumberFor(size_);
                Int last_start      = walkIndexFw(
                                        block_limit/BranchingFactor,
                                        walker,
                                        index_size_ - level_size,
                                        level_size,
                                        level_limit
                                      );

                Int last_start_end  = getBlockStartEnd(last_start);

                Int last_end = last_start_end <= size()? last_start_end : size();

                return walker.walkKeys(last_start, last_end);
            }
        }
    }


    template <typename Walker>
    Int findBw(Int start, Walker& walker) const
    {
        MEMORIA_ASSERT(start, >=, -1);

        Int block_end   = getBlockStartEndBw(start);

        if (block_end == -1)
        {
            return walker.walkKeys(start, -1);
        }
        else
        {
            Int limit = walker.walkKeys(start, block_end);
            if (limit > block_end)
            {
                return limit;
            }
            else {
                walker.prepareIndex();

                Int level_size = getIndexCellsNumberFor(max_size_);
                Int last_start = walkIndexBw(block_end/BranchingFactor, walker, index_size_ - level_size, level_size);

                Int last_start_end = getBlockStartEndBw(last_start);

                return walker.walkKeys(last_start, last_start_end);
            }
        }
    }


    void dumpRanges(Byte* memory_block = nullptr, std::ostream& out = std::cout)
    {
        if (memory_block == nullptr)
        {
            memory_block = memory_block_;
        }

        for (Int c = 0; c <= Blocks; c++)
        {
            Int     offset      = getKeyBlockOffset(c);
            BigInt  key_offset  = T2T<SizeT>(memory_block + offset) - MemBase;

            out<<key_offset<<" "<<flush;
        }

        Int     offset      = getValueBlockOffset();
        BigInt  key_offset  = T2T<SizeT>(memory_block + offset + getValueSize()*maxSize()) - MemBase;

        out<<key_offset<<endl;
    }


private:

    template <typename Functor>
    void walkIndexRange(Int start, Int end, Functor& walker, Int level_offet, Int level_size) const
    {
        if (end - start <= BranchingFactor*2)
        {
            walker.walkIndex(start + level_offet, end + level_offet);
        }
        else {
            Int block_start_end     = getBlockStartEnd(start);
            Int block_end_start     = getBlockStart(end);

            walker.walkIndex(start + level_offet, block_start_end + level_offet);

            if (block_start_end < block_end_start)
            {
                Int level_size0 = getIndexCellsNumberFor(level_size);
                walkIndexRange(
                        start/BranchingFactor + 1,
                        end/BranchingFactor,
                        walker,
                        level_offet - level_size0,
                        level_size0
                );
            }

            walker.walkIndex(block_end_start + level_offet, end + level_offet);
        }
    }


    template <typename Walker>
    Int walkIndexFw(Int start, Walker& walker, Int level_offet, Int level_size, Int level_limit) const
    {
        Int block_start_end     = getBlockStartEnd(start);

        if (block_start_end >= level_limit)
        {
            return (walker.walkIndex(start + level_offet, level_limit + level_offet) - level_offet) * BranchingFactor;
        }
        else
        {
            Int limit = walker.walkIndex(start + level_offet, block_start_end + level_offet) - level_offet;
            if (limit < block_start_end)
            {
                return limit * BranchingFactor;
            }
            else {
                Int level_size0     = getIndexCellsNumberFor(level_size);
                Int level_limit0    = getIndexCellsNumberFor(level_limit);

                Int last_start      = walkIndexFw(
                                        block_start_end/BranchingFactor,
                                        walker,
                                        level_offet - level_size0,
                                        level_size0,
                                        level_limit0
                                      );

                Int last_start_end  = getBlockStartEnd(last_start);

                Int last_end = last_start_end <= level_limit ? last_start_end : level_limit;

                return (walker.walkIndex(last_start + level_offet, last_end + level_offet) - level_offet) * BranchingFactor;
            }
        }
    }


    template <typename Walker>
    Int walkIndexBw(Int start, Walker& walker, Int level_offet, Int level_size) const
    {
        Int block_start_end     = getBlockStartEndBw(start);

        if (block_start_end == -1)
        {
            return (walker.walkIndex(start + level_offet, level_offet - 1) - level_offet + 1) * BranchingFactor - 1;
        }
        else
        {
            Int idx = walker.walkIndex(start + level_offet, block_start_end + level_offet) - level_offet;
            if (idx > block_start_end)
            {
                return (idx + 1) * BranchingFactor - 1;
            }
            else {
                Int level_size0 = getIndexCellsNumberFor(level_size);
                Int last_start  = walkIndexBw(
                                    block_start_end/BranchingFactor,
                                    walker,
                                    level_offet - level_size0,
                                    level_size0
                                  );

                Int last_start_end = getBlockStartEndBw(last_start);

                return (walker.walkIndex(last_start + level_offet, last_start_end + level_offet) - level_offet + 1)
                        * BranchingFactor - 1;
            }
        }
    }




protected:
    static Int getBlockStart(Int i)
    {
        return (i / BranchingFactor) * BranchingFactor;
    }

    static Int getBlockStartEnd(Int i)
    {
        return (i / BranchingFactor + 1) * BranchingFactor;
    }

    static Int getBlockStartEndBw(Int i)
    {
        return (i / BranchingFactor) * BranchingFactor - 1;
    }

    static Int getBlockEnd(Int i)
    {
        return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
    }

    static Int getIndexCellsNumberFor(Int i)
    {
        return i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0);
    }

private:

    template <typename TreeType>
    void repackDataWithSameTypes(TreeType* other, Byte* target_memory_block) const
    {
        Int new_keys_size           = other->maxSize();
        Byte* target_keys           = other->getKeysPtr(target_memory_block);

        Int target_keys_block_size  = new_keys_size * sizeof(Key);

        if (this->isMeInsideTarget(other, target_memory_block))
        {
            Int range = this->findBlockRange(target_keys, target_keys_block_size, std::less_equal<const Byte*>());

            if (range < Blocks)
            {
                copyValuesBlock(other, target_memory_block);
            }

            for (Int block = Blocks - 1; block >= range; block--)
            {
                copyKeysBlock(other, target_memory_block, block);
            }

            for (Int block = 0; block < range; block++)
            {
                copyKeysBlock(other, target_memory_block, block);
            }

            if (range == Blocks)
            {
                copyValuesBlock(other, target_memory_block);
            }
        }
        else if (this->isTargetInsideMe(other, target_memory_block))
        {
            Int range = this->findBlockRange(target_keys, target_keys_block_size, std::greater_equal<const Byte*>());

            if (range == Blocks)
            {
                copyValuesBlock(other, target_memory_block);
            }

            for (Int block = range - 1; block >= 0; block--)
            {
                copyKeysBlock(other, target_memory_block, block);
            }

            for (Int block = range; block < Blocks; block++)
            {
                copyKeysBlock(other, target_memory_block, block);
            }

            if (range < Blocks)
            {
                copyValuesBlock(other, target_memory_block);
            }
        }
        else if (target_keys < this->getKeysPtr())
        {
            for (Int block = 0; block < Blocks; block++)
            {
                copyKeysBlock(other, target_memory_block, block);
            }

            copyValuesBlock(other, target_memory_block);
        }
        else
        {
            copyValuesBlock(other, target_memory_block);

            for (Int block = Blocks - 1; block >= 0; block--)
            {
                copyKeysBlock(other, target_memory_block, block);
            }
        }
    }

    template <typename TreeType>
    void repackDataWithDifferentTypes(TreeType* other, Byte* target_memory_block) const
    {
        if (DebugCounter == 1) {
            int a = 0;
            a++;
        }

        Int target_max_size         = other->maxSize();
        Byte* target_keys           = other->getKeysPtr(target_memory_block);

        Int target_keys_block_size  = target_max_size * sizeof(typename TreeType::Key);

        if (this->isMeInsideTarget(other, target_memory_block))
        {
            Int range = this->findBlockRange(target_keys, target_keys_block_size, std::less_equal<const Byte*>());

            if (range < Blocks)
            {
                copyValues(other, target_memory_block);
            }

            for (Int block = Blocks - 1; block >= range; block--)
            {
                copyKeys(other, target_memory_block, block);
            }

            for (Int block = 0; block < range; block++)
            {
                copyKeys(other, target_memory_block, block);
            }

            if (range == Blocks)
            {
                copyValues(other, target_memory_block);
            }
        }
        else if (this->isTargetInsideMe(other, target_memory_block))
        {
            Int range = this->findBlockRange(target_keys, target_keys_block_size, std::greater_equal<const Byte*>());

            if (range == Blocks)
            {
                copyValues(other, target_memory_block);
            }

            for (Int block = range - 1; block >= 0; block--)
            {
                copyKeys(other, target_memory_block, block);
            }

            for (Int block = range; block < Blocks; block++)
            {
                copyKeys(other, target_memory_block, block);
            }

            if (range < Blocks)
            {
                copyValues(other, target_memory_block);
            }
        }
        else if (target_keys < this->getKeysPtr())
        {
            for (Int block = 0; block < Blocks; block++)
            {
                copyKeys(other, target_memory_block, block);
            }

            copyValues(other, target_memory_block);
        }
        else
        {
            copyValues(other, target_memory_block);

            for (Int block = Blocks - 1; block >= 0; block--)
            {
                copyKeys(other, target_memory_block, block);
            }
        }
    }

    template <typename TreeType>
    bool isTargetInsideMe(const TreeType* target, const Byte* target_memory_block) const
    {
        const Byte* start           = memory_block_ + this->getKeyBlockOffset(0);
        const Byte* end             = memory_block_ + this->getValueBlockOffset() + maxSize() * getValueSize();

        const Byte* target_start    = target_memory_block + target->getKeyBlockOffset(0);
        const Byte* target_end      = target_memory_block + target->getValueBlockOffset() + target->maxSize() * target->getValueSize();

        return start <= target_start && end >= target_end;
    }

    bool isTargetInsideMe(const Byte* memory_block, const Byte* target_memory_block, Int size, Int target_size) const
    {
        return target_memory_block >= memory_block && (target_memory_block + target_size) <= (memory_block + size);
    }

    template <typename TreeType>
    bool isMeInsideTarget(const TreeType* target, const Byte* target_memory_block) const
    {
        const Byte* start           = memory_block_ + this->getKeyBlockOffset(0);
        const Byte* end             = memory_block_ + this->getValueBlockOffset() + maxSize() * getValueSize();

        const Byte* target_start    = target_memory_block + target->getKeyBlockOffset(0);
        const Byte* target_end      = target_memory_block + target->getValueBlockOffset() + target->maxSize() * target->getValueSize();

        return start >= target_start && end <= target_end;
    }

    bool isMeInsideTarget(const Byte* memory_block, const Byte* target_memory_block, Int size, Int target_size) const
    {
        return target_memory_block <= memory_block && (target_memory_block + target_size) >= (memory_block + size);
    }

    template <typename Comparator>
    Int findBlockRange(const Byte* target_memory_block, Int target_block_size, const Comparator& cmp) const
    {
        const Byte* mptr = getKeysPtr();
        const Byte* tptr = target_memory_block;

        for (Int c = 0; c < Blocks; c++)
        {
            if (cmp(mptr, tptr))
            {
                return c;
            }

            mptr += max_size_ * sizeof(Key);
            tptr += target_block_size;
        }

        return Blocks;
    }

    template <typename Comparator>
    Int findRange(const Byte* memory_block, const Byte* target_memory_block, Int item_size, Int target_item_size, const Comparator& cmp) const
    {
        for (Int c = 0; c < size(); c++)
        {
            if (cmp(memory_block, target_memory_block))
            {
                return c;
            }

            memory_block        += item_size;
            target_memory_block += target_item_size;
        }

        return size();
    }


    template <typename TreeType>
    void copyKeysBlock(TreeType* other, Byte* target_memory_block, Int block) const
    {
        Int offset          = getKeyBlockOffset(block);
        Int target_offset   = other->getKeyBlockOffset(block);

        copyData(target_memory_block, offset, target_offset, sizeof(Key));
    }

    template <typename TreeType>
    void copyKeys(TreeType* other, Byte* target_memory_block, Int block) const
    {
        Int offset          = getKeyBlockOffset(block);
        Int target_offset   = other->getKeyBlockOffset(block);

        const Byte* key_block   = memory_block_ + offset;
        Byte* target_key_block  = target_memory_block + target_offset;

        Int item_size               = sizeof(Key);
        Int target_item_size        = sizeof(typename TreeType::Key);

        Int key_block_size          = size() * item_size;
        Int target_key_block_size   = size() * target_item_size;

        if (this->isMeInsideTarget(key_block, target_key_block, key_block_size, target_key_block_size))
        {
            Int range = this->findRange(key_block, target_key_block, item_size, target_item_size, std::less_equal<const Byte*>());

            copyKeysAsc(other, target_memory_block, offset, target_offset, 0, range);
            copyKeysDsc(other, target_memory_block, offset, target_offset, range, size());
        }
        else if (this->isTargetInsideMe(key_block, target_key_block, key_block_size, target_key_block_size))
        {
            Int range = this->findRange(key_block, target_key_block, item_size, target_item_size, std::greater_equal<const Byte*>());

            copyKeysDsc(other, target_memory_block, offset, target_offset, 0, range);
            copyKeysAsc(other, target_memory_block, offset, target_offset, range, size());
        }
        else if (target_key_block < key_block)
        {
            copyKeysAsc(other, target_memory_block, offset, target_offset, 0, size());
        }
        else
        {
            copyKeysDsc(other, target_memory_block, offset, target_offset, 0, size());
        }
    }

    template <typename TreeType>
    void copyKeysAsc(TreeType* other, Byte* target_memory_block, Int offset, Int target_offset, Int from, int to) const
    {
        for (Int c = from; c < to; c++)
        {
            other->keyb(target_memory_block, target_offset, c) = keyb(offset, c);
        }
    }

    template <typename TreeType>
    void copyKeysDsc(TreeType* other, Byte* target_memory_block, Int offset, Int target_offset, Int from, int to) const
    {
        for (Int c = to - 1; c >= from; c--)
        {
            other->keyb(target_memory_block, target_offset, c) = keyb(offset, c);
        }
    }


    template <typename TreeType>
    void copyValuesBlock(TreeType* other, Byte* target_memory_block) const
    {
        if (getValueSize() > 0)
        {
            Int offset      = getValueBlockOffset();
            Int new_offset  = other->getValueBlockOffset();

            copyData(target_memory_block, offset, new_offset, getValueSize());
        }
    }

    template <typename TreeType>
    void copyValues(TreeType* other, Byte* target_memory_block) const
    {
        if (getValueSize() == 0) return;

        Int offset          = getValueBlockOffset();
        Int target_offset   = other->getValueBlockOffset();

        const Byte* value_block     = memory_block_ + offset;
        Byte* target_value_block    = target_memory_block + target_offset;

        Int value_block_size        = size() * getValueSize();
        Int target_value_block_size = size() * other->getValueSize();

        if (this->isMeInsideTarget(value_block, target_value_block, value_block_size, target_value_block_size))
        {
            Int range = this->findRange(value_block, target_value_block, getValueSize(), other->getValueSize(), std::less_equal<const Byte*>());

            copyValuesAsc(other, target_memory_block, offset, target_offset, 0, range);
            copyValuesDsc(other, target_memory_block, offset, target_offset, range, size());

        }
        else if (this->isTargetInsideMe(value_block, target_value_block, value_block_size, target_value_block_size))
        {
            Int range = this->findRange(value_block, target_value_block, getValueSize(), other->getValueSize(), std::greater_equal<const Byte*>());

            copyValuesDsc(other, target_memory_block, offset, target_offset, 0, range);
            copyValuesAsc(other, target_memory_block, offset, target_offset, range, size());
        }
        else if (target_value_block < value_block)
        {
            copyValuesAsc(other, target_memory_block, offset, target_offset, 0, size());
        }
        else
        {
            copyValuesDsc(other, target_memory_block, offset, target_offset, 0, size());
        }
    }

    template <typename TreeType>
    void copyValuesAsc(TreeType* other, Byte* target_memory_block, Int offset, Int target_offset, Int from, int to) const
    {
        for (Int c = from; c < to; c++)
        {
            other->valueb(target_memory_block, target_offset, c) = valueb(offset, c);
        }
    }

    template <typename TreeType>
    void copyValuesDsc(TreeType* other, Byte* target_memory_block, Int offset, Int target_offset, Int from, int to) const
    {
        for (Int c = to - 1; c >= from; c--)
        {
            other->valueb(target_memory_block, target_offset, c) = valueb(offset, c);
        }
    }

    void copyData(Byte* target_memory_block, Int offset, Int new_offset, Int item_size) const
    {
        CopyBuffer(
                memory_block_       + offset,
                target_memory_block + new_offset,
                size_ * item_size
        );
    }

    void copyData(Int offset, Int room_start, Int room_length, Int item_size)
    {
        Byte* src = memory_block_ + offset + room_start * item_size;
        Byte* dst = src + room_length * item_size;

        CopyBuffer(src, dst, (size_ - room_start) * item_size);
    }

    static Int getBlockSize(Int item_count)
    {
        Int key_size  = sizeof(IndexKey) * Blocks;
        Int item_size = sizeof(Key) * Blocks + getValueSize();

        return getIndexSize(item_count) * key_size + item_count * item_size;
    }

    static Int getIndexSize(Int csize)
    {
        if (csize == 1)
        {
            return 1;
        }
        else {
            Int sum = 0;
            for (Int nlevels=0; csize > 1; nlevels++)
            {
                csize = ((csize % BranchingFactor) == 0) ? (csize / BranchingFactor) : (csize / BranchingFactor) + 1;
                sum += csize;
            }
            return sum;
        }
    }

    static Int getLevelsForSize(Int csize)
    {
        Int nlevels;
        for (nlevels = 0; csize > 1; nlevels++)
        {
            Int idx = csize / BranchingFactor;

            csize = ((csize % BranchingFactor) == 0) ? idx : idx + 1;
        }

        return nlevels;
    }

    static Int getParentNodesFor(Int n)
    {
        Int idx = n / BranchingFactor;

        return (n % BranchingFactor) == 0 ? idx : idx + 1;
    }


    static Int getMaxSize(Int block_size)
    {
        Int item_size   = sizeof(Key) * Blocks + getValueSize();

        Int first       = 1;
        Int last        = block_size / item_size;

        while (first < last - 1)
        {
            Int middle = (first + last) / 2;

            Int size = getBlockSize(middle);
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

        Int max_size;

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

        return max_size;
    }
};

}


#endif
