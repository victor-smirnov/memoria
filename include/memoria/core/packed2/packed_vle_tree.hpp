
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_TREE_HPP_

#include <memoria/core/packed2/packed_tree_base.hpp>
#include <memoria/core/packed2/packed_tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {

using namespace vapi;

template <
	typename K,
	typename IK,
	typename V,
	typename Allocator_ 	= EmptyAllocator,
	Int Blocks_ 			= 2,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= PackedExintVLETreeValuesPerBranch
>
struct PackedVLETreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;
    static const Int ALIGNMENT_BLOCK		= 8; //Bytes

    template <typename T, typename VV>
    using Codec = ExintCodec<T, VV>;
};


template <typename Value>
class VLETreeValueDescr {
	Value value_;
	Int pos_;
	Int idx_;
public:
	VLETreeValueDescr(BigInt value, Int pos, Int idx): value_(value), pos_(pos), idx_(idx) {}

	Value value() const 	{return value_;}
	Int pos() const 		{return pos_;}
	Int idx() const 		{return idx_;}
};


template <typename Types_>
class PackedVLETree: public PackedTreeBase<
	PackedVLETree<Types_>,
	typename Types_::IndexKey,
	Types_::BranchingFactor,
	Types_::ValuesPerBranch,
	Types_::ALIGNMENT_BLOCK
> {

	typedef PackedTreeBase<
			PackedVLETree<Types_>,
			typename Types_::IndexKey,
			Types_::BranchingFactor,
			Types_::ValuesPerBranch,
			Types_::ALIGNMENT_BLOCK
		>																		Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedVLETree<Types>               									MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::Key													Key;
	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	typedef UBigInt																OffsetsType;

	typedef typename Types::template Codec<UByte, Value>						Codec;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;

	static const Int BITS_PER_OFFSET		= 4;
	static const Int ALIGNMENT_BLOCK		= Types::ALIGNMENT_BLOCK; //Bytes

private:

	Int size_;
	Int index_size_;
	Int max_size_;

	UByte buffer_[];

public:
	PackedVLETree() {}

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		Base::allocator_offset() = diff;
	}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	const Int& index_size() const {return index_size_;}

	const Int& max_size() const {return max_size_;}

	Int block_size() const
	{
		return sizeof(MyType) + getDataOffset() + max_size();
	}

	OffsetsType* offsetsBlock() {
		return T2T<OffsetsType*> (buffer_);
	}

	const OffsetsType* offsetsBlock() const {
		return T2T<OffsetsType*> (buffer_);
	}

	IndexKey* indexes(Int index_block)
	{
		return T2T<IndexKey*>(buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * index_block);
	}

	IndexKey* sizes() {
		return indexes(0);
	}

	const IndexKey* indexes(Int index_block) const
	{
		return T2T<const IndexKey*>(buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * index_block);
	}

	const IndexKey* sizes() const {
		return indexes(0);
	}

	UByte* values()
	{
		return buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * Indexes;
	}

	const UByte* values() const
	{
		return buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * Indexes;
	}



	static Int getValueBlocks(Int items_num)
	{
		return items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);
	}

	Int getValueBlocks() const {
		return getValueBlocks(max_size_);
	}

	Int getOffsetsLengts() const
	{
		return getOffsetsLengts(max_size_);
	}

	static Int getOffsetsLengts(Int items_num)
	{
		if (items_num > ValuesPerBranch)
		{
			Int value_blocks = getValueBlocks(items_num);

			Int offsets_bits = value_blocks * BITS_PER_OFFSET;

			Int offsets_bytes = Base::roundBitsToAlignmentBlocks(offsets_bits);

			return offsets_bytes;
		}
		else {
			return 0;
		}
	}

	static Int block_size(Int items_num)
	{
		Int offsets_length = getOffsetsLengts(items_num);

		Int index_size = getIndexSize(items_num);

		return offsets_length + index_size * Indexes * sizeof(IndexKey) + items_num;
	}

	static Int expected_block_size(Int items_num)
	{
		Int offsets_length = getOffsetsLengts(items_num);

		Int index_size = getIndexSize(items_num);

		return sizeof(MyType) + offsets_length + index_size * Indexes * sizeof(IndexKey) + items_num * 2;
	}

	static Int getIndexSize(Int items_number)
	{
		if (items_number > ValuesPerBranch)
		{
			return MyType::compute_index_size(items_number);
		}
		else {
			return 0;
		}
	}

	// ================================= Reindexing ======================================== //

private:

	class ReindexFn: public ReindexFnBase<MyType> {

		typedef ReindexFnBase<MyType> 				Base;

		const UByte* values_;

	public:
		ReindexFn(MyType& me): Base(me)
		{
			values_ = me.values();
		}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size)
		{
			if (Base::indexSize() == 0)
			{
				return;
			}

			Codec codec;

			Int pos = 0;

			Int limit = ValuesPerBranch;
			Int value_block = 0;

			if (Base::me_.offsets() > 0)
			{
				Base::me_.offset(0) = 0;
			}

			for (Int c = 0; c < Base::me_.size(); c++)
			{
				Value value;
				Int len = codec.decode(values_, value, pos);

				Int idx = index_level_start + value_block;

				Base::indexes_[0][idx] += 1;
				Base::indexes_[1][idx] += value;

				pos += len;

				if (pos >= limit)
				{
					value_block++;

					if (value_block < Base::me_.offsets())
					{
						Base::me_.offset(value_block) = pos - limit;
					}

					limit += ValuesPerBranch;
				}

			}
		}
	};

public:

	void reindex()
	{
		ReindexFn fn(*this);
		Base::reindex(0, size(), fn);
	}

	// ==================================== Value ========================================== //

private:

	class GetValueOffsetFn: public GetValueOffsetFnBase<MyType, GetValueOffsetFn> {
		typedef GetValueOffsetFnBase<MyType, GetValueOffsetFn> Base;
	public:
		GetValueOffsetFn(const MyType& me, Int limit): Base(me, limit){}

		void processIndexes(Int start, Int end)	{}
		void processValue(Value value)	{}
	};

public:

	Int getValueOffset(Int idx) const
	{
		GetValueOffsetFn fn(*this, idx);

		Int pos = Base::find_fw(fn);

		return pos;
	}

	Int data_size() const
	{
		if (size() > 0)
		{
			Int pos = getValueOffset(size() - 1);

			const UByte* values_ = values();
			Codec codec;

			return pos + codec.length(values_, pos);
		}
		else {
			return 0;
		}
	}

	Value getValue(Int idx) const
	{
		const UByte* values_ = values();

		Int pos = getValueOffset(idx);

		Codec codec;
		Value value;

		codec.decode(values_, value, pos);
		return value;
	}

	Int setValue(Int idx, Value value)
	{
		UByte* values_ = values();

		Int pos = getValueOffset(idx);

		MEMORIA_ASSERT(pos, <, max_size_);

		Int total_size = data_size();

		Codec codec;

		Int value_len = codec.length(value);

		MEMORIA_ASSERT(pos + value_len, <, max_size_);

		Int stored_value_len = codec.length(values_, pos);
		Int delta 	 		 = value_len - stored_value_len;

		if (delta > 0)
		{
			Int capacity = max_size() - total_size;

			if (capacity < delta)
			{
				if (!enlarge(delta))
				{
					return delta;
				}
			}
		}

		UByte* value_pos = values_ + pos;

		CopyBuffer(value_pos + stored_value_len, value_pos + value_len, total_size - (pos + stored_value_len));

		codec.encode(values_, value, pos);

		reindex();

		return 0;
	}

	void appendValue(Value value)
	{
		Int len = setValue(size(), value);
		if (len == 0)
		{
			size_++;
			reindex();
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Out of block memory for value at "<<size_);
		}
	}


	FnAccessor<Value> value(Int idx)
	{
		return FnAccessor<Value>(
			[this, idx]() {return this->getValue(idx);},
			[this, idx](const Value& value) {
				if (this->setValue(idx, value) > 0)
				{
					throw Exception(MA_SRC, SBuf()<<"Out of block memory for value at "<<idx);
				}
			}
		);
	}

	ConstFnAccessor<Value> value(Int idx) const
	{
		return FnAccessor<Value>(
			[this, idx]() {return this->getValue(idx);}
		);
	}

	// =================================== Offsets ======================================== //

	Int offsets() const
	{
		Int blocks = getValueBlocks();
		return blocks > 1 ? blocks : 0;
	}

	BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>
	offset(Int idx)
	{
		return BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
	}

	BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>
	offset(Int idx) const
	{
		return BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
	}

	// =============================== Initialization ====================================== //


private:
	struct InitFn {
		Int getBlockSize(Int items_number) const
		{
			return MyType::block_size(items_number);
		}

		Int extend(Int items_number) const {
			return items_number;
		}

		Int getIndexSize(Int items_number) const {
			return MyType::compute_index_size(items_number);
		}
	};

public:
	void init(Int block_size)
	{
		size_ = 0;
		Base::allocator_offset() = 0;

		max_size_   = MyType::getMaxSize(block_size - sizeof(MyType), InitFn());
		index_size_ = getIndexSize(max_size_);
	}

	// ================================= Allocation ======================================== //

	Int getDataOffset() const
	{
		return getOffsetsLengts() + index_size_ * sizeof(IndexKey) * Indexes;
	}

	Allocator* allocator()
	{
		if (Base::allocator_offset() > 0)
		{
			UByte* my_ptr = T2T<UByte*>(this);
			return T2T<Allocator*>(my_ptr - Base::allocator_offset());
		}
		else {
			return nullptr;
		}
	}

	const Allocator* allocator() const
	{
		const UByte* my_ptr = T2T<const UByte*>(this);
		return T2T<const Allocator*>(my_ptr - Base::allocator_offset());
	}

	void transferTo(MyType* other, UByte* target_memory_block = nullptr) const
	{
		if (target_memory_block == nullptr)
		{
			target_memory_block = other->values();
		}

		Int data_size = this->data_size();

		const UByte* data = values();

		CopyByteBuffer(data, target_memory_block, data_size);
	}

	bool enlarge(Int amount)
	{
		Allocator* alloc = allocator();

		if (alloc)
		{
			Int size = block_size();

			MyType other;

			other.init(size + amount);

			Int new_size = alloc->enlargeBlock(this, other.block_size());

			other.init(new_size);

			transferTo(&other, buffer_ + other.getDataOffset());

			return true;
		}
		else {
			return false;
		}
	}

	void shrink(Int amount)
	{
		Allocator* alloc = allocator();
		Int size = block_size();

		MEMORIA_ASSERT(size - amount, >=, 0);

		MyType other;
		other.init(size - amount);

		Int new_size = alloc->shrinkBlock(this, size - amount);

		other.init(new_size);

		transferTo(&other, buffer_ + other.getDataOffset());
	}

	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout) const
	{
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;
		out<<"index_size_ = "<<index_size_<<endl;
		out<<endl;

		out<<"Offsets:"<<endl;

		Int value_blocks = getValueBlocks(max_size_);

		if (value_blocks > 1)
		{
			dumpSymbols<Value>(out, value_blocks, 5, [&](Int idx) -> Value {
				return this->offset(idx);
			});
		}

		out<<endl;

		Int idx_max = index_size_;

		out<<"Indexes:"<<dec<<endl;

		for (Int c = 0; c < idx_max; c++)
		{
			out<<c<<" ";
			for (Int block = 0; block < Indexes; block++)
			{
				out<<indexes(block)[c]<<" ";
			}
			out<<endl;
		}

		out<<endl;

		out<<"Data:"<<endl;

		const UByte* values_ = this->values();
		size_t pos = 0;

		Codec codec;

		dumpArray<Value>(out, size_, [&](Int) -> Value {
			Value value;
			pos += codec.decode(values_, value, pos);
			return value;
		});
	}

private:
};


}


#endif
