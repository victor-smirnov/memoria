
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_INPUT_BUFFER_ROW_ORDER_HPP_
#define MEMORIA_CORE_PACKED_FSE_INPUT_BUFFER_ROW_ORDER_HPP_

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria {



template <
    typename V,
    Int Blocks_ = 1,
    typename Allocator_ = PackedAllocator
>
struct PackedFSERowOrderInputBufferTypes {
    typedef V               	Value;
    static const Int Blocks		= Blocks_;
};



template <typename Types_>
class PackedFSERowOrderInputBuffer: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSERowOrderInputBuffer<Types>                                 MyType;

    typedef PackedAllocator                                            			Allocator;
    typedef typename Types::Value                                               Value;

    static constexpr Int Indexes 	= 0;
    static constexpr Int Blocks 	= Types::Blocks;

    static constexpr Int SafetyMargin = 0;

    using InputType = Value;
    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;
    using SizesT = core::StaticVector<Int, Blocks>;

private:

    Int size_;
    Int max_size_;

    Value buffer_[];

public:
    PackedFSERowOrderInputBuffer() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int& max_size() {return max_size_;}
    const Int& max_size() const {return max_size_;}

    Int capacity() const {return max_size_ - size_;}

    Int total_capacity() const
    {
        Int my_size     = allocator()->element_size(this);
        Int free_space  = allocator()->free_space();
        Int data_size   = sizeof(Value) * size_ * Blocks;

        return (my_size + free_space - data_size) / (sizeof(Value) * Blocks);
    }

    Int block_size() const
    {
        return sizeof(MyType) + max_size_ * sizeof(Value) * Blocks;
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

public:

    static constexpr Int block_size(Int array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr Int packed_block_size(Int array_size)
    {
        return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value) * Blocks);
    }

    static constexpr Int elements_for(Int block_size)
    {
        return max_size_for(block_size);
    }

    Int allocated_block_size() const
    {
        if (Base::allocator_offset() != 0)
        {
            return this->allocator()->element_size(this);
        }
        else {
            return block_size();
        }
    }

    void init(Int block_size)
    {
        size_ = 0;
        max_size_ = max_size_for(block_size);
    }

    void init(const SizesT& capacities)
    {
        size_ = 0;
        max_size_ = capacities[0];
    }

    static constexpr Int max_size_for(Int block_size) {
        return (block_size - empty_size()) / (sizeof(Value) * Blocks);
    }

    static constexpr Int empty_size()
    {
        return sizeof(MyType);
    }


    void init()
    {
        size_ = 0;
        max_size_   = 0;
    }


    Value& value(Int block, Int idx) {
        return buffer_[idx * Blocks + block];
    }

    const Value& value(Int block, Int idx) const {
        return buffer_[idx * Blocks + block];
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



    // =================================== Update ========================================== //

    void reindex() {}
    void check() const {}

    bool ensureCapacity(Int size)
    {
        Int capacity = this->capacity();
        if (capacity < size)
        {
            enlarge(size - capacity);
            return true;
        }
        else {
            return false;
        }
    }

    void enlarge(Int items_num)
    {
        Allocator* alloc = allocator();

        Int requested_block_size    = (max_size_ + items_num) * sizeof(Value) * Blocks + empty_size();
        Int new_size                = alloc->resizeBlock(this, requested_block_size);
        max_size_                   = max_size_for(new_size);
    }

    void shrink(Int items_num)
    {
        MEMORIA_ASSERT(max_size_ - items_num, >=, size_);

        enlarge(-items_num);
    }

    void remove(Int start, Int end)
    {
    	MEMORIA_ASSERT_TRUE(start >= 0);
    	MEMORIA_ASSERT_TRUE(end >= 0);

    	Int room_length = end - start;
    	Int size = this->size();
    	MEMORIA_ASSERT(room_length, <= , size - start);

    	Value* values = this->values();

    	CopyBuffer(
    			values + end * Blocks,
				values + start * Blocks,
				(size_ - end) * Blocks
    	);

    	size_ -= room_length;

    	shrink(room_length);
    }

    void removeSpace(Int room_start, Int room_end) {
        remove(room_start, room_end);
    }

    void insertSpace(Int idx, Int room_length)
    {
        MEMORIA_ASSERT(idx, <=, this->size());
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(room_length, >=, 0);

        Int capacity = this->capacity();

        if (capacity < room_length)
        {
            enlarge(room_length - capacity);
        }

        auto values = this->values();

        CopyBuffer(
        		values + idx * Blocks,
				values + (idx + room_length) * Blocks,
				(size_ - idx) * Blocks
        );

        size_ += room_length;

        clear(idx, idx + room_length);
    }



    void clear(Int start, Int end)
    {
    	auto values = this->values();

    	for (Int c = start; c < end; c++)
    	{
    		for (Int block = 0; block < Blocks; block++)
    		{
    			values[c * Blocks + block] = 0;
    		}
    	}
    }

    void reset()
    {
    	size_ = 0;
    }


    void resize(Int delta)
    {
        if (delta > 0)
        {
            insertSpace(size_, delta);
        }
        else {
            removeSpace(size_, -delta);
        }
    }

    // ===================================== IO ============================================ //

    void insert(Int block, Int pos, Value val)
    {
        insertSpace(pos, 1);
        value(block, pos) = val;
    }

    void insert(Int pos, Int start, Int size, const InputBuffer* buffer)
    {
    	insertSpace(pos, size);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Value* vals 		= values(block);
    		const Value* data 	= buffer->values(block);
    		CopyBuffer(data + start, vals + pos, size);
    	}
    }


    SizesT positions(Int idx) const {
    	return SizesT(idx);
    }

    SizesT capacities() const {
    	return SizesT(capacity());
    }

    template <typename Adaptor>
    void append(SizesT& at, Int size, Adaptor&& adaptor)
    {
    	auto values = this->values();

    	Int start = at[0];

    	for (Int c = 0; c < size; c++)
    	{
    		auto value = adaptor(c);

    		for (Int block = 0; block < Blocks; block++)
    		{
    			values[(start + c) * Blocks + block] = value[block];
    		}
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		at[block] += size;
    	}

    	this->size() += size;
    }




    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
    	insertSpace(pos, size);

    	auto values = this->values();

    	for (Int c = pos; c < pos + size; c++)
    	{
    		auto item = adaptor();

    		for (Int b = 0; b < Blocks; b++) {
    			values[c * Blocks + b] = item[b];
    		}
    	}
    }





    template <typename Fn>
    SizesT scan(Int start, Int end, Fn&& fn) const
    {
        MEMORIA_ASSERT(start, <=, size_);
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, >=, 0);
        MEMORIA_ASSERT(end, <=, size_);

        auto values = this->values();

        for (Int c = start; c < end; c++)
        {
        	Values item;
        	for (Int b = 0; b < Blocks; b++) {
        		item[b] = values[c * Blocks + b];
        	}

        	fn(item);
        }

        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        out<<"size_       = "<<size_<<endl;
        out<<"max_size_   = "<<max_size_<<endl;
        out<<endl;

        out<<"Data:"<<endl;

        const Value* values_ = buffer_;

        if (Blocks == 1)
        {
        	dumpArray<Value>(out, size_ * Blocks, [&](Int pos) -> Value {
        		return values_[pos];
        	});
        }
        else {
        	for (Int c = 0; c < size_; c++)
        	{
        		out<<c<<": ";
        		for (Int b = 0; b < Blocks; b++)
        		{
        			out<<values_[c * Blocks + b]<<", ";
        		}
        		out<<endl;
        	}
        }
    }
};





}


#endif
