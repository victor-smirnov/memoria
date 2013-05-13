
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_ARRAYHPP_
#define MEMORIA_CORE_PACKED_FSE_ARRAYHPP_

#include <memoria/core/packed2/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/idata.hpp>

namespace memoria {



template <
	typename V,
	typename Allocator_ = PackedAllocator
>
struct PackedFSEArrayTypes {
    typedef V               Value;
    typedef Allocator_  	Allocator;
};



template <typename Types_>
class PackedFSEArray: public PackedAllocatable {

	typedef PackedAllocatable													Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSEArray<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;
	typedef typename Types::Value												Value;


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
		Int my_size		= allocator()->element_size(this);
		Int free_space 	= allocator()->free_space();
		Int data_size  	= sizeof(Value) * size_;

		return (my_size + free_space - data_size) / sizeof(Value);
	}

	Int block_size() const
	{
		return sizeof(MyType) + max_size_ * sizeof(Value);
	}

public:

	static Int block_size(int array_size)
	{
		return PackedAllocator::roundUpBytesToAlignmentBlocks(sizeof(MyType) + array_size * sizeof(Value));
	}

	static Int elements_for(Int block_size)
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

	static Int max_size_for(Int block_size) {
		return (block_size - sizeof(MyType)) / sizeof(Value);
	}

	static Int empty_size()
	{
		return sizeof(MyType);
	}

	void initEmpty()
	{
		size_ 		= 0;
		max_size_ 	= 0;
	}

	Int object_size() const
	{
		Int object_size = sizeof(MyType) + sizeof(Value) * size_;
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

	BigInt sum(Int start, Int end) const {
		return end - start;
	}

	// =================================== Update ========================================== //

	void reindex() {}


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

		Int requested_block_size 	= (max_size_ + items_num) * sizeof(Value) + sizeof(MyType);
		Int new_size 				= alloc->resizeBlock(this, requested_block_size);
		max_size_ 					= max_size_for(new_size);
	}

	void shrink(Int items_num)
	{
		MEMORIA_ASSERT(max_size_ - items_num, >=, size_);

		enlarge(-items_num);
	}

	void removeSpace(Int room_start, Int room_length)
	{
		MEMORIA_ASSERT(room_start, <=, max_size_);
		MEMORIA_ASSERT(room_start, <=, size_);

		MEMORIA_ASSERT(room_start + room_length, <=, size_);

		Int length = size_ - room_start - room_length;

		if (length > 0)
		{
			CopyBuffer(buffer_ + room_start + room_length, buffer_ + room_start, length);
		}

		size_ -= room_length;

		shrink(max_size_ - size_);
	}

	void insertSpace(Int room_start, Int room_length)
	{
		if (room_start > size_)
		{
			int a = 0; a++;
		}

		MEMORIA_ASSERT(room_start, <=, size_);

		if (capacity() < room_length)
		{
			enlarge(room_length - capacity());
		}

		Int length = size_ - room_start;

		if (length > 0)
		{
			CopyBuffer(buffer_ + room_start, buffer_ + room_start + room_length, length);
		}

		size_ += room_length;
	}

	void clearValues(Int idx) {
		buffer_[idx] = 0;
	}

	void clear(Int start, Int end)
	{
		Value* values = this->values();

		for (Int c = start; c < end; c++)
		{
			values[c] = 0;
		}
	}


	void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		CopyBuffer(buffer_ + copy_from, other->buffer_ + copy_to, count);
	}

	template <typename TreeType>
	void transferDataTo(TreeType* other) const
	{
		const auto* my_values 	= values();
		auto* other_values 		= other->values();

		Int size = this->size();

		for (Int c = 0; c < size; c++)
		{
			other_values[c] 	= my_values[c];
		}

		other->size() = size;
	}

	// ===================================== IO ============================================ //

	void insert(IData* data, Int pos, Int length)
	{
		IDataSource<Value>* src = static_cast<IDataSource<Value>*>(data);
		insertSpace(pos, length);

		BigInt to_write_local = length;

		while (to_write_local > 0)
		{
			SizeT processed = src->get(buffer_, pos, to_write_local);

			pos 			+= processed;
			to_write_local 	-= processed;
		}
	}

	void update(IData* data, Int pos, Int length)
	{
		MEMORIA_ASSERT(pos, <=, size_);
		MEMORIA_ASSERT(pos + length, <=, size_);

		IDataSource<Value>* src = static_cast<IDataSource<Value>*>(data);

		BigInt to_write_local = length;

		while (to_write_local > 0)
		{
			SizeT processed = src->get(buffer_, pos, to_write_local);

			pos 			+= processed;
			to_write_local 	-= processed;
		}
	}

	void read(IData* data, Int pos, Int length) const
	{
		MEMORIA_ASSERT(pos, <=, size_);
		MEMORIA_ASSERT(pos + length, <=, size_);

		IDataTarget<Value>* tgt = static_cast<IDataTarget<Value>*>(data);

		BigInt to_read_local = length;

		while (to_read_local > 0)
		{
			SizeT processed = tgt->put(buffer_, pos, to_read_local);

			pos 			+= processed;
			to_read_local 	-= processed;
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

		dumpArray<Value>(out, size_, [&](Int pos) -> Value {
			return values_[pos];
		});
	}


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->startGroup("ARRAY");

		handler->value("ALLOCATOR",     &Base::allocator_offset());
		handler->value("SIZE",          &size_);
		handler->value("MAX_SIZE",      &max_size_);

		handler->startGroup("DATA", size_);

		handler->value("DATA_ITEM", buffer_, size_, IPageDataEventHandler::BYTE_ARRAY);

		handler->endGroup();

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
		FieldFactory<Int>::serialize(buf, size_);
		FieldFactory<Int>::serialize(buf, max_size_);

		FieldFactory<Value>::serialize(buf, buffer_, size_);
	}

	void deserialize(DeserializationData& buf)
	{
		FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
		FieldFactory<Int>::deserialize(buf, size_);
		FieldFactory<Int>::deserialize(buf, max_size_);

		FieldFactory<Value>::deserialize(buf, buffer_, size_);
	}

};


}


#endif
