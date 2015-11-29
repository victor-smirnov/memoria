
// Copyright Victor Smirnov 2013+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_ARRAYHPP_
#define MEMORIA_CORE_PACKED_FSE_ARRAYHPP_

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>

namespace memoria {



template <
    typename V,
    Int Blocks_ = 1,
    typename Allocator_ = PackedAllocator
>
struct PackedFSEArrayTypes {
    typedef V               	Value;
    static const Int Blocks		= Blocks_;
    typedef Allocator_      	Allocator;
};



template <typename Types_>
class PackedFSEArray: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSEArray<Types>                                               MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Value                                               Value;

    static const Int Indexes 													= 0;
    static const Int Blocks 													= Types::Blocks;

    using InputType = Value;

    using InputBuffer = MyType;

    using Values = core::StaticVector<Value, Blocks>;

private:

    Int size_;
    Int max_size_;

    Value buffer_[];

public:
    PackedFSEArray() {}

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

    static constexpr Int max_size_for(Int block_size) {
        return (block_size - empty_size()) / (sizeof(Value) * Blocks);
    }

    static constexpr Int empty_size()
    {
        return sizeof(MyType);
    }

    void initEmpty()
    {
    	size_ = 0;
        max_size_   = 0;
    }

    void init()
    {
        size_ = 0;
        max_size_   = 0;
    }

    Int object_size() const
    {
        Int object_size = sizeof(MyType) + sizeof(Value) * size_ * Blocks;
        return PackedAllocator::roundUpBytesToAlignmentBlocks(object_size);
    }

    Value& operator[](Int idx) {
        return buffer_[idx];
    }

    const Value& operator[](Int idx) const {
        return buffer_[idx];
    }

    Value& value(Int idx) {
        return buffer_[idx];
    }

    const Value& value(Int idx) const {
        return buffer_[idx];
    }


    Value get_values(Int idx) const {
    	return value(idx);
    }

    Value get_values(Int idx, Int index) const {
    	return value(idx);
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

    Value* values(Int block) {
        return buffer_ + block * size_;
    }

    const Value* values(Int block) const {
        return buffer_ + block * size_;
    }


    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sub(Int start, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int idx, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
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

    	for (Int block = Blocks - 1, sum = 0; block >= 0; block--)
    	{
    		Int to      = block * size_ + start;
    		Int from    = block * size_ + end;

    		Int length = size_ - end + sum;

    		CopyBuffer(
    				values + from,
    				values + to,
    				length
    		);

    		sum += size_ - (end - start);
    	}

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

        Value* values = this->values();

        for (Int block = 0; block < Blocks; block++)
        {
            Int offset = (Blocks - block - 1) * size_ + idx;

            CopyBuffer(
                    values + offset,
                    values + offset + room_length,
                    size_ * Blocks - offset + room_length * block
            );
        }

        size_ += room_length;

        clear(idx, idx + room_length);
    }

    void clearValues(Int idx) {
        buffer_[idx] = 0;
    }

    void clear(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
        	Value* values = this->values(block);

        	for (Int c = start; c < end; c++)
        	{
        		values[c] = 0;
        	}
        }
    }

    void reset()
    {
    	size_ = 0;
    }


    void splitTo(MyType* other, Int idx)
    {
        MEMORIA_ASSERT(other->size(), ==, 0);

        Int split_size = this->size() - idx;
        other->insertSpace(0, split_size);

        copyTo(other, idx, split_size, 0);

        removeSpace(idx, this->size());
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->insertSpace(other_size, my_size);

        copyTo(other, 0, my_size, other_size);

        removeSpace(0, my_size);

        reindex();
    }


    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_ASSERT_TRUE(copy_from >= 0);
        MEMORIA_ASSERT_TRUE(count >= 0);

        for (Int block = 0; block < Blocks; block++)
        {
            Int my_offset       = block * size_;
            Int other_offset    = block * other->size_;

            CopyBuffer(
                    this->values() + copy_from + my_offset,
                    other->values() + copy_to + other_offset,
                    count
            );
        }
    }

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        const auto* my_values   = values();
        auto* other_values      = other->values();

        Int size = this->size() * Blocks;

        for (Int c = 0; c < size; c++)
        {
            other_values[c]     = my_values[c];
        }

        other->size() = size;
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

    void insert(Int pos, Value val)
    {
        insertSpace(pos, 1);
        value(pos) = val;
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

    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
    	insertSpace(pos, size);

    	for (Int c = 0; c < size; c++)
    	{
    		value(c + pos) = adaptor(0, c);
    	}
    }


    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class AccumItem>
    void _update(Int pos, Value val, AccumItem<T, Size>& accum)
    {
        value(pos) = val;
    }

    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class AccumItem>
    void _insert(Int pos, Value val, AccumItem<T, Size>& accum)
    {
    	insert(pos, val);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void _remove(Int idx, AccumItem<T, Size>& accum)
    {
    	remove(idx, idx + 1);
    }


    template <typename Fn>
    void read(Int block, Int start, Int end, Fn&& fn) const
    {
        MEMORIA_ASSERT(start, <, size_);
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, >=, 0);
        MEMORIA_ASSERT(end, <=, size_);

        auto values = this->values(block);

        for (Int c = start; c < end; c++)
        {
        	fn(values[c]);
        }
    }



    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        out<<"size_       = "<<size_<<endl;
        out<<"max_size_   = "<<max_size_<<endl;
        out<<endl;

        out<<"Data:"<<endl;

        const Value* values_ = buffer_;

        dumpArray<Value>(out, size_ * Blocks, [&](Int pos) -> Value {
            return values_[pos];
        });
    }


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	handler->startStruct();
        handler->startGroup("ARRAY");

        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);

        handler->startGroup("DATA", size_);

        vapi::ValueHelper<Value>::setup(handler, "DATA_ITEM", buffer_, size_ * Blocks, IPageDataEventHandler::BYTE_ARRAY);

        handler->endGroup();

        handler->endGroup();
    	handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<Int>::serialize(buf, max_size_);

        FieldFactory<Value>::serialize(buf, buffer_, size_ * Blocks);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<Int>::deserialize(buf, max_size_);

        FieldFactory<Value>::deserialize(buf, buffer_, size_ * Blocks);
    }

};


template <typename Types>
struct PkdStructSizeType<PackedFSEArray<Types>> {
	static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <typename T>
struct StructSizeProvider<PackedFSEArray<T>> {
    static const Int Value = 0;
};



}


#endif
