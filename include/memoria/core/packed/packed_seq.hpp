
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_SEQ_HPP_
#define MEMORIA_CORE_PMAP_PACKED_SEQ_HPP_

#include <memoria/core/container/page_traits.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/bitmap_select.hpp>
#include <memoria/core/tools/md5.hpp>

#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/packed/tree_walkers.hpp>

#include <functional>
#include <algorithm>

namespace memoria {


namespace intrnl1 {

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

template <
	typename IK = UInt,
	typename V  = UBigInt,
	Int Bits_ = 1,
	Int BF = PackedSeqBranchingFactor,
	Int VPB = PackedSeqValuesPerBranch
>
struct PackedSeqTypes {
    typedef IK              IndexKey;
    typedef V               Value;

    static const Int Bits                 	= Bits_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch		= VPB;
};

template <typename Types>
class PackedSeq {

    static const UInt VERSION               = 1;

    typedef PackedSeq<Types>               MyType;

public:

    typedef typename Types::IndexKey        IndexKey;
    typedef typename Types::Value           Value;
    typedef typename Types::Value           Symbol;

    static const Int Bits					= Types::Bits;
    static const Int Blocks                 = 1<<Bits;
    static const Int Symbols                = Blocks;
    static const Int BranchingFactor        = Types::BranchingFactor;
    static const Int ValuesPerBranch        = Types::ValuesPerBranch;

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
            IndexKey,
            Value
    >                                                                           FieldsList;

    PackedSeq() {}


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

        handler->symbols("BITMAP", valuesBlock(), size_, Bits);

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, size());
        FieldFactory<Int>::serialize(buf, max_size_);
        FieldFactory<Int>::serialize(buf, index_size_);

        FieldFactory<IndexKey>::serialize(buf, indexBlock(), Blocks * indexSize());

        const Value* values = valuesBlock();

        FieldFactory<Value>::serialize(buf, values, getUsedValueCells());
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, size());
        FieldFactory<Int>::deserialize(buf, max_size_);
        FieldFactory<Int>::deserialize(buf, index_size_);

        FieldFactory<IndexKey>::deserialize(buf, indexBlock(), Blocks * indexSize());

        Value* values = valuesBlock();

        FieldFactory<Value>::deserialize(buf, values, getUsedValueCells());
    }

    void initByBlock(Int block_size)
    {
        size_ = 0;

        max_size_   = getMaxSize(block_size);
        index_size_ = getIndexSize(max_size_);

        MEMORIA_ASSERT(getDataSize(), <=, block_size);
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

    static size_t getObjectSzie(size_t capacity)
    {
    	MyType seq;
    	seq.initSizes(capacity);

    	return seq.getObjectSize();
    }

    Int getObjectDataSize() const
    {
        return sizeof(size_) + sizeof(max_size_) + sizeof(index_size_) + getBlockSize();
    }

    Int getBlockSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(max_size_);
    }

    static Int getValueCellsCount(Int values_count)
    {
    	Int total_bits 	= values_count * Bits;
    	size_t mask 	= TypeBitmask<Value>();
    	size_t bitsize	= TypeBitsize<Value>();

    	size_t suffix 	= total_bits & mask;

    	return total_bits / bitsize + (suffix > 0);
    }

    static Int getValueBlockSize(Int values_count)
    {
    	return getValueCellsCount(values_count) * sizeof(Value);
    }

    Int getDataSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(size_);
    }

    Int getTotalDataSize() const
    {
        return (index_size_ * sizeof(IndexKey)) * Blocks + getValueBlockSize(max_size_);
    }

    Int getUsedValueCells() const
    {
    	return getValueCellsCount(size_);
    }

    Int getTotalValueCells() const
    {
    	return getValueCellsCount(max_size_);
    }

    Int getValueCellsCapacity() const
    {
    	return getValueCellsCount(max_size_ - size_);
    }

    Int& size() {
        return size_;
    }

    const Int& size() const
    {
        return size_;
    }

    Int capacity() const {
    	return max_size_ - size_;
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
        return (indexSize * sizeof(IndexKey)) * Blocks + getValueBlockSize(max);
    }

    Byte* memoryBlock()
    {
        return memory_block_;
    }

    const Byte* memoryBlock() const
    {
        return memory_block_;
    }

    Value* valuesBlock()
    {
    	return T2T<Value*>(memory_block_ + getValueBlockOffset());
    }

    const Value* valuesBlock() const
    {
    	return T2T<const Value*>(memory_block_ + getValueBlockOffset());
    }

    IndexKey* indexBlock()
    {
    	return T2T<IndexKey*>(memory_block_);
    }

    const IndexKey* indexBlock() const
    {
    	return T2T<const IndexKey*>(memory_block_);
    }

    IndexKey* indexes(Int block) {
    	return T2T<IndexKey*>(memory_block_ + getIndexKeyBlockOffset(block));
    }

    const IndexKey* indexes(Int block) const {
    	return T2T<const IndexKey*>(memory_block_ + getIndexKeyBlockOffset(block));
    }


    Int getIndexKeyBlockOffset(Int block_num) const
    {
        return sizeof(IndexKey) * index_size_ * block_num;
    }

    Int getValueBlockOffset() const
    {
        return getIndexKeyBlockOffset(Blocks);
    }

    IndexKey& indexb(Int block_offset, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return *T2T<IndexKey*>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    const IndexKey& indexb(Int block_offset, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        return *T2T<const IndexKey*>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    IndexKey& index(Int block_num, Int key_num)
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        Int block_offset = getIndexKeyBlockOffset(block_num);

        return *T2T<IndexKey*>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    const IndexKey& index(Int block_num, Int key_num) const
    {
        MEMORIA_ASSERT(key_num, >=, 0);
        MEMORIA_ASSERT(key_num, <, index_size_);

        Int block_offset = getIndexKeyBlockOffset(block_num);

        return *T2T<IndexKey*>(memory_block_ + block_offset + key_num * sizeof(IndexKey));
    }

    IndexKey& maxIndex(Int block_num)
    {
        return index(block_num, 0);
    }

    const IndexKey& maxIndex(Int block_num) const
    {
        return index(block_num, 0);
    }

    const IndexKey& maxIndexb(Int block_offset) const
    {
        return indexb(block_offset, 0);
    }


    Int rank(Int from, Int to, Value symbol) const
    {
    	MEMORIA_ASSERT(from, >=, 0);
    	MEMORIA_ASSERT(to, >=, from);
    	MEMORIA_ASSERT(to, <, size());

    	RankWalker<MyType, Bits> walker(*this, symbol);

    	walkRange(from, to + 1, walker);

    	return walker.sum();
    }

    Int rank(Int to, Value symbol) const
    {
    	MEMORIA_ASSERT(to, <, size());

    	RankWalker<MyType, Bits> walker(*this, symbol);

    	walkRange(to + 1, walker);

    	return walker.sum();
    }


    Int rank1(Int from, Int to, Value symbol) const
    {
    	MEMORIA_ASSERT(from, >=, 0);
    	MEMORIA_ASSERT(to, >=, from);
    	MEMORIA_ASSERT(to, <=, size());

    	RankWalker<MyType, Bits> walker(*this, symbol);

    	walkRange(from, to, walker);

    	return walker.sum();
    }

    Int rank1(Int to, Value symbol) const
    {
    	MEMORIA_ASSERT(to, <=, size());

    	RankWalker<MyType, Bits> walker(*this, symbol);

    	walkRange(to, walker);

    	return walker.sum();
    }



    SelectResult selectFW(Int from, Value symbol, Int rank) const
    {
    	MEMORIA_ASSERT(from, >=, 0);
    	MEMORIA_ASSERT(from, <, size());

    	SelectFWWalker<MyType, Bits> walker(*this, symbol, rank);

    	Int idx = findFw(from, walker);

    	return SelectResult(idx, walker.rank(), walker.is_found());
    }

    SelectResult selectFW(Value symbol, Int rank) const
    {
    	SelectFWWalker<MyType, Bits> walker(*this, symbol, rank);

    	Int idx = findFw(walker);

    	return SelectResult(idx, walker.rank(), walker.is_found());
    }


    SelectResult selectBW(Int from, Value symbol, Int rank) const
    {
    	MEMORIA_ASSERT(from, >=, 0);
    	MEMORIA_ASSERT(from, <=, size());

    	SelectBWWalker<MyType, Bits> walker(*this, symbol, rank);

    	Int idx = findBw(from, walker);

    	return SelectResult(idx, walker.rank(), walker.is_found());
    }

    IndexKey countFW(Int from, Value symbol) const
    {
    	CountFWWalker<MyType, Bits> walker(*this, symbol);

    	findFw(from, walker);

    	return walker.rank();
    }

    IndexKey countBW(Int from, Value symbol) const
    {
    	CountBWWalker<MyType, Bits> walker(*this, symbol);

    	findBw(from, walker);

    	return walker.rank();
    }

    void dump(ostream& out_) const
    {
    	out_<<"size_ = "<<size_<<endl;
    	out_<<"max_size_ = "<<max_size_<<endl;
    	out_<<"index_size_ = "<<index_size_<<endl;

    	Expand(out_, 5);
    	for (Int d = 0; d < Blocks; d++)
    	{
    		out_.width(5);
    		out_<<d;
    	}

    	out_<<endl<<endl;

    	for (Int c = 0; c < index_size_; c++)
    	{
    		out_.width(4);
    		out_<<c<<": ";
    		for (Int d = 0; d < Blocks; d++)
    		{
    			out_.width(4);
    			out_<<this->indexb(this->getIndexKeyBlockOffset(d), c)<<" ";
    		}
    		out_<<endl;
    	}


    	Int columns;

    	switch (Bits) {
    	case 1: columns = 100; break;
    	case 2: columns = 100; break;
    	case 4: columns = 100; break;
    	default: columns = 50;
    	}

    	Int width = Bits <= 4 ? 1 : 3;

    	Int c = 0;

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

    		Int rows = 0;
    		for (; c < size() && rows < 10; c += columns, rows++)
    		{
    			Expand(out_, 12);
    			out_<<" ";
    			out_.width(6);
    			out_<<dec<<c<<" "<<hex;
    			out_.width(6);
    			out_<<c<<": ";

    			for (Int d = 0; d < columns && c + d < size(); d++)
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
    	MyType&	me_;
    	Int 	block_offset_;
    	Int 	idx_;

    public:
    	ValueSetter(MyType& me, Int block_offset, Int idx):
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
    ValueSetter value(Int value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    Value value(Int value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return valueb(getValueBlockOffset(), value_num);
    }

    ValueSetter valueb(Int block_offset, Int value_num)
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return ValueSetter(*this, block_offset, value_num);
    }

    Value valueb(Int block_offset, Int value_num) const
    {
        MEMORIA_ASSERT(value_num, >=, 0);
        MEMORIA_ASSERT(value_num, <, max_size_);

        return this->getValueItem(block_offset, value_num);
    }

    Value getValueItem(Int block_offset, Int item_idx) const
    {
    	if (Bits == 1 || Bits == 2 || Bits == 4)
    	{
    		const Value* buffer = T2T<const Value*>(memory_block_ + block_offset);
    		return GetBits0(buffer, item_idx * Bits, Bits);
    	}
    	else if (Bits == 8)
    	{
    		const UByte* buffer = T2T<const UByte*>(memory_block_ + block_offset);
    		return buffer[item_idx];
    	}
    	else
    	{
    		const Value* buffer = T2T<const Value*>(memory_block_ + block_offset);
    		return GetBits(buffer, item_idx * Bits, Bits);
    	}
    }

    void setValueItem(Int block_offset, Int item_idx, const Value& v)
    {
    	if (Bits == 1 || Bits == 2 || Bits == 4)
    	{
    		Value* buffer = T2T<Value*>(memory_block_ + block_offset);
    		SetBits0(buffer, item_idx * Bits, v, Bits);
    	}
    	else if (Bits == 8)
    	{
    		UByte* buffer = T2T<UByte*>(memory_block_ + block_offset);
    		buffer[item_idx] = v;
    	}
    	else
    	{
    		Value* buffer = T2T<Value*>(memory_block_ + block_offset);
    		SetBits(buffer, item_idx * Bits, v, Bits);
    	}
    }

    bool testb(Int block_offset, Int item_idx, Value value) const
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

    const Value* cellAddr(Int idx) const
    {
    	return T2T<const Value*>(valuesBlock() + idx);
    }

    Value* cellAddr(Int idx)
    {
    	return T2T<Value*>(valuesBlock() + idx);
    }

    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_ASSERT(copy_from, >=, 0);
        MEMORIA_ASSERT(copy_from + count, <=, max_size_);

        MEMORIA_ASSERT(copy_to, >=, 0);

        MEMORIA_ASSERT(copy_to + count, <=, other->max_size_);

        const Value* src 	= valuesBlock();
        Value* dst 			= other->valuesBlock();

        MoveBits(src, dst, copy_from * Bits, copy_to * Bits, count * Bits);
    }

    void clearValues(Int from, Int to)
    {
    	Int block_offset = this->getValueBlockOffset();

    	for (Int idx = from; idx < to; idx++)
    	{
    		valueb(block_offset, idx) = 0;
    	}
    }

    void clear(Int from, Int to)
    {
        MEMORIA_ASSERT(from, >=, 0);
        MEMORIA_ASSERT(to, <=, max_size_);
        MEMORIA_ASSERT(from, <=, to);

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
        clearValues(size(), maxSize());
    }

    void clearUnused()
    {
        clearUnusedData();
        clearIndexes();
    }

    void enlargeBlock(Int block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        transferTo(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void shrinkBlock(Int block_size)
    {
        MyType buf;
        buf.initByBlock(block_size);

        transferTo(&buf, memory_block_);

        buf.size() = this->size();

        *this = buf;

        clearUnused();
    }

    void transferTo(MyType* other, Byte* memory_block = nullptr) const
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

    void insertSpace(Int room_start, Int room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, max_size_);
        MEMORIA_ASSERT(room_start,  <=, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(size_ + room_length, <=, max_size_);

        copyTo(this, room_start, size() - room_start, room_start + room_length);

        size_ += room_length;
    }

    void removeSpace(Int room_start, Int room_length)
    {
        MEMORIA_ASSERT(room_start,  >=, 0);
        MEMORIA_ASSERT(room_start,  <, size_);

        MEMORIA_ASSERT(room_length, >=, 0);
        MEMORIA_ASSERT(room_start + room_length, <=, size_);

        Int copy_from = room_start + room_length;

        copyTo(this, copy_from, size() - copy_from, room_start);

        size_ -= room_length;
    }

    void add(Int block_num, Int idx, const IndexKey& value)
    {
        Int index_block_offset  = getIndexKeyBlockOffset(block_num);

        Int index_level_size    = getIndexCellsNumberFor(max_size_);
        Int index_level_start   = index_size_ - index_level_size;

        Int level = 0;
        while (index_level_start >= 0)
        {
            idx /= BranchingFactor;

            indexb(index_block_offset, idx + index_level_start) += value;

            Int index_parent_size   = getIndexCellsNumberFor(level, index_level_size);

            index_level_size        = index_parent_size;
            index_level_start       -= index_parent_size;

            level++;
        }
    }


    void insert(const Value& val, Int at)
    {
        if (at < size_ - 1)
        {
            insertSpace(at, 1);
        }

        value(at) = val;
    }


    void updateUp(Int block_num, Int idx, IndexKey key_value)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size());

        Int level_size      = maxSize();
        Int level_start     = indexSize();

        Int block_offset    = getIndexKeyBlockOffset(block_num);

        Int level 			= 0;

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

    void reindex(Int start, Int end)
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, <=, size());
        MEMORIA_ASSERT(start, <=, end);

        Int block_start = getBlockStartV(start);
        Int block_end   = getBlockEndV(end);

        Int value_block_offset  = getValueBlockOffset();

        Int index_level_size    = getIndexCellsNumberFor(0, maxSize());
        Int index_level_start   = indexSize() - index_level_size;

        Int level_max           = size();

        if (Bits < 3)
        {
        	for (Int block = 0; block < Blocks; block++)
        	{
        		Int index_block_offset  = getIndexKeyBlockOffset(block);

        		for (Int c = block_start; c < block_end; c += ValuesPerBranch)
        		{
        			Int max      = c + ValuesPerBranch <= level_max ? c + ValuesPerBranch : level_max;

        			IndexKey sum = popCount(value_block_offset, c, max, block);

        			Int idx = c / ValuesPerBranch + index_level_start;
        			indexb(index_block_offset, idx) = sum;
        		}
        	}
        }
        else {
        	for (Int c = block_start; c < block_end; c += ValuesPerBranch)
        	{
        		Int max      = c + ValuesPerBranch <= level_max ? c + ValuesPerBranch : level_max;

        		IndexKey sums[Blocks];
        		for (Int block = 0; block < Blocks; block++) sums[block] = 0;

        		for (Int d = c; d < max; d++)
        		{
        			Value symbol = valueb(value_block_offset, d);
        			sums[symbol]++;
        		}

        		Int idx = c / ValuesPerBranch + index_level_start;

        		for (Int block = 0; block < Blocks; block++)
        		{
        			index(block, idx) = sums[block];
        		}
        	}
        }


        for (Int block = 0; block < Blocks; block ++)
        {
        	Int block_start0 		= block_start;
        	Int block_end0 			= block_end;
        	Int level_max0			= level_max;
        	Int index_level_start0 	= index_level_start;
        	Int index_level_size0 	= index_level_size;


        	Int level = 0;
        	Int index_block_offset = getIndexKeyBlockOffset(block);

        	Int divider = ValuesPerBranch;

        	while (index_level_start0 > 0)
        	{
        		level_max0      = getIndexCellsNumberFor(level, level_max0);
        		block_start0    = getBlockStart(block_start0 / divider);
        		block_end0      = getBlockEnd(block_end0 / divider);

        		Int index_parent_size   = getIndexCellsNumberFor(level + 1, index_level_size0);
        		Int index_parent_start  = index_level_start0 - index_parent_size;

        		for (Int c = block_start0; c < block_end0; c += BranchingFactor)
        		{
        			IndexKey sum = 0;
        			Int max      = (c + BranchingFactor <= level_max0 ? c + BranchingFactor : level_max0) + index_level_start0;

        			for (Int d = c + index_level_start0; d < max; d++)
        			{
        				sum += indexb(index_block_offset, d);
        			}

        			Int idx = c / BranchingFactor + index_parent_start;
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


    IndexKey popCount(Int start, Int end, Value symbol) const
    {
    	Int block_offset = getValueBlockOffset();
    	return popCount(block_offset, start, end, symbol);
    }

    IndexKey popCount(Int block_offset, Int start, Int end, Value symbol) const
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

    		for (Int c = start; c < end; c++)
    		{
    			total += testb(block_offset, c, symbol);
    		}

    		return total;
    	}
    }


    template <typename Functor>
    void walkRange(Int start, Int end, Functor& walker) const
    {
    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(end,   <=,  size());
    	MEMORIA_ASSERT(start, <=,  end);

    	if (end - start <= ValuesPerBranch * 2)
    	{
    		walker.walkValues(start, end);
    	}
    	else {
    		Int block_start_end     = getBlockStartEndV(start);
    		Int block_end_start     = getBlockStartV(end);

    		walker.walkValues(start, block_start_end);

    		if (block_start_end < block_end_start)
    		{
    			Int level_size = getIndexCellsNumberFor(0, max_size_);
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
    void walkRange(Int target, Walker& walker) const
    {
    	MEMORIA_ASSERT(target,   <=,  size());

    	Int levels = 0;
    	Int level_sizes[LEVELS_MAX];

    	Int level_size = max_size_;
    	Int cell_size = 1;

    	do
    	{
    		level_size = getIndexCellsNumberFor(levels, level_size);
    		level_sizes[levels++] = level_size;
    	}
    	while (level_size > 1);

    	cell_size = ValuesPerBranch;
    	for (Int c = 0; c < levels - 2; c++)
    	{
    		cell_size *= BranchingFactor;
    	}

    	Int base = 1, start = 0, target_idx = target;

    	for (Int level = levels - 2; level >= 0; level--)
    	{
    		Int end = target_idx / cell_size;

    		walker.walkIndex(start + base, end + base);

    		start 		= level > 0 ? end * BranchingFactor : end * ValuesPerBranch;
    		base 		+= level_sizes[level];
    		cell_size 	/= BranchingFactor;
    	}

    	return walker.walkValues(start, target);
    }


    template <typename Walker>
    Int findFw(Walker &walker) const
    {
    	Int levels = 0;
    	Int level_sizes[LEVELS_MAX];

    	Int level_size = max_size_;

    	do
    	{
    		level_size = getIndexCellsNumberFor(levels, level_size);
    		level_sizes[levels++] = level_size;
    	}
    	while (level_size > 1);

    	Int base = 1, start = 0;

    	for (Int level = levels - 2; level >= 0; level--)
    	{
    		Int level_size  = level_sizes[level];
    		Int end         = (start + BranchingFactor < level_size) ? (start + BranchingFactor) : level_size;

    		Int idx = walker.walkIndex(start + base, end + base, 0) - base;
    		if (idx < end)
    		{
    			start = level > 0 ? idx * BranchingFactor : idx * ValuesPerBranch;
    		}
    		else {
    			return size_;
    		}

    		base += level_size;
    	}

    	Int end = (start + ValuesPerBranch) > size_ ? size_ : start + ValuesPerBranch;

    	return walker.walkValues(start, end);
    }



    template <typename Walker>
    Int findFw(Int start, Walker& walker) const
    {
    	MEMORIA_ASSERT(start, <=, size());


    	Int block_limit     = getBlockStartEndV(start);

    	if (block_limit >= size())
    	{
    		return walker.walkValues(start, size());
    	}
    	else
    	{
    		Int limit = walker.walkValues(start, block_limit);
    		if (limit < block_limit)
    		{
    			return limit;
    		}
    		else {
    			walker.prepareIndex();

    			Int level_size      = getIndexCellsNumberFor(0, max_size_);
    			Int level_limit     = getIndexCellsNumberFor(0, size_);
    			Int last_start      = walkIndexFw(
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
    				Int last_start_end  = getBlockStartEndV(last_start);

    				Int last_end = last_start_end <= size()? last_start_end : size();

    				return walker.walkValues(last_start, last_end);
    			}
    			else {
    				return size();
    			}
    		}
    	}
    }

    template <typename Walker>
    size_t findBw(Int start, Walker& walker) const
    {
        MEMORIA_ASSERT(start, >=, 0);

        Int block_end   = getBlockStartEndBwV(start);

        if (block_end == 0)
        {
            return walker.walkValues(start, 0);
        }
        else
        {
            Int limit = walker.walkValues(start, block_end);
            if (walker.is_found())
            {
                return limit;
            }
            else {
                walker.prepareIndex();

                Int level_size = getIndexCellsNumberFor(0, max_size_);
                Int last_start = walkIndexBw(
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
    static Int getBlockStart(Int i)
    {
        return (i / BranchingFactor) * BranchingFactor;
    }

    static Int getBlockStartEnd(Int i)
    {
        return (i / BranchingFactor + 1) * BranchingFactor;
    }

    static Int getBlockStartV(Int i)
    {
    	return (i / ValuesPerBranch) * ValuesPerBranch;
    }

    static Int getBlockStartEndV(Int i)
    {
    	return (i / ValuesPerBranch + 1) * ValuesPerBranch;
    }

    static Int getBlockStartEndBw(Int i)
    {
        return (i / BranchingFactor) * BranchingFactor - 1;
    }

    static Int getBlockStartEndBwV(Int i)
    {
    	return (i / ValuesPerBranch) * ValuesPerBranch;
    }

    static Int getBlockEnd(Int i)
    {
        return (i / BranchingFactor + ((i % BranchingFactor) ? 1 : 0)) * BranchingFactor;
    }

    static Int getBlockEndV(Int i)
    {
    	return (i / ValuesPerBranch + ((i % ValuesPerBranch) ? 1 : 0)) * ValuesPerBranch;
    }

    static Int getIndexCellsNumberFor(Int i)
    {
    	return getIndexCellsNumberFor(1, i);
    }

    static Int getIndexCellsNumberFor(Int level, Int i)
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
    void walkIndexRange(Int start, Int end, Functor& walker, Int level_offet, Int level_size, Int cell_size) const
    {
        if (end - start <= BranchingFactor * 2)
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
    Int walkIndexFw(
    		Int start,
    		Walker& walker,
    		Int level_offet,
    		Int level_size,
    		Int level_limit,
    		Int cells_number_on_lower_level,
    		Int cell_size
    ) const
    {
    	Int block_start_end     = getBlockStartEnd(start);

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
    		Int limit = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
    		if (limit < block_start_end)
    		{
    			return limit * cells_number_on_lower_level;
    		}
    		else {
    			Int level_size0     = getIndexCellsNumberFor(level_size);
    			Int level_limit0    = getIndexCellsNumberFor(level_limit);

    			Int last_start      = walkIndexFw(
    									block_start_end / BranchingFactor,
    									walker,
    									level_offet - level_size0,
    									level_size0,
    									level_limit0,
    									BranchingFactor,
    									cell_size * BranchingFactor
    								  );

    			Int last_start_end  = getBlockStartEnd(last_start);

    			Int last_end = last_start_end <= level_limit ? last_start_end : level_limit;

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
    Int walkIndexBw(
    		Int start,
    		Walker& walker,
    		Int level_offet,
    		Int level_size,
    		Int cells_number_on_lower_level,
    		Int cell_size
    ) const
    {
    	Int block_start_end     = getBlockStartEndBw(start);

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
    		Int idx = walker.walkIndex(start + level_offet, block_start_end + level_offet, cell_size) - level_offet;
    		if (idx > block_start_end)
    		{
    			return (idx + 1) * cells_number_on_lower_level;
    		}
    		else {
    			Int level_size0 = getIndexCellsNumberFor(level_size);
    			Int last_start  = walkIndexBw(
    								block_start_end / BranchingFactor,
    								walker,
    								level_offet - level_size0,
    								level_size0,
    								BranchingFactor,
    								cell_size * BranchingFactor
    							  ) - 1;

    			Int last_start_end = getBlockStartEndBw(last_start);

    			return (walker.walkIndex(
    								last_start + level_offet,
    								last_start_end + level_offet,
    								cell_size
    						   )
    						   - level_offet + 1) * cells_number_on_lower_level;
    		}
    	}
    }


    void copyValuesBlock(MyType* other, Byte* target_memory_block) const
    {
    	Value* tgt = T2T<Value*>(target_memory_block + other->getValueBlockOffset());

    	CopyBuffer(valuesBlock(), tgt, getValueCellsCount(size()));
    }

    static Int getBlockSize(Int item_count)
    {
        return getIndexSize(item_count) * sizeof(IndexKey) * Blocks + getValueBlockSize(item_count);
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

    static Int getMaxSize(Int block_size)
    {
        Int first       = 1;
        Int last        = block_size * 8 / Bits;

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

        Int cells = getValueCellsCount(max_size);
        Int max = cells * TypeBitsize<Value>() / Bits;

        if (getIndexSize(max) <= getIndexSize(max_size))
        {
        	return max;
        }

        return max_size;
    }
};

}


#endif
