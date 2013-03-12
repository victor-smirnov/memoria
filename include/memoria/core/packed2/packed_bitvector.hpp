
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_BITVECTOR_HPP_
#define MEMORIA_CORE_PACKED_BITVECTOR_HPP_

#include <memoria/core/packed2/packed_tree_base.hpp>
#include <memoria/core/packed2/packed_tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>

namespace memoria {

template <
	typename Allocator_ = EmptyAllocator,
	Int BF = PackedTreeBranchingFactor,
	Int VPB = PackedSeqValuesPerBranch
>
struct PackedBitVectorTypes {
    typedef Allocator_  	Allocator;

    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    static const Int ALIGNMENT_BLOCK        = 8;
};



template <typename Types_>
class PackedBitVector: public PackedTreeBase<
	PackedBitVector<Types_>,
	Int,
	Types_::BranchingFactor,
	Types_::ValuesPerBranch,
	Types_::ALIGNMENT_BLOCK
> {

	typedef  PackedTreeBase<
			PackedBitVector<Types_>,
			Int,
			Types_::BranchingFactor,
			Types_::ValuesPerBranch,
			Types_::ALIGNMENT_BLOCK
	>																			Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedBitVector<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;

	typedef Int																	Key;
	typedef Int																	IndexKey;
	typedef UBigInt																Value;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= 2;

	static const Int ALIGNMENT_BLOCK		= Types::ALIGNMENT_BLOCK; //Bytes

	struct Codec {
		size_t length(const Value* buffer, size_t idx) const {return 1;}
		size_t length(Value value) const {return 1;}

		size_t decode(const Value* buffer, Value& value, size_t idx) const {
			value = buffer[idx];
			return 1;
		}

		size_t encode(Value* buffer, Value value, size_t idx) const
		{
			buffer[idx] = value;
			return 1;
		}
	};

private:

	Int size_;
	Int index_size_;
	Int max_size_;

	UByte buffer_[];

public:
	PackedBitVector() {}

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

	int block_size() const
	{
		Int index_length = index_size_ * sizeof(IndexKey) * Indexes;

		return sizeof(MyType) + index_length + roundBitsToAlignmentBlocks(max_size_);
	}

	IndexKey* indexes(Int index_block)
	{
		return T2T<IndexKey*>(buffer_ + index_size_ * sizeof(IndexKey) * (index_block));
	}

	const IndexKey* indexes(Int index_block) const
	{
		return T2T<const IndexKey*>(buffer_ + index_size_ * sizeof(IndexKey) * (index_block));
	}

	IndexKey maxValue(Int index_block) const
	{
		return indexes(index_block)[0];
	}

	Value* values()
	{
		return T2T<Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes);
	}

	const Value* values() const
	{
		return T2T<const Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes);
	}

	Value* symbols()
	{
		return values();
	}

	const Value* symbols() const
	{
		return values();
	}

	static Int roundBitsToAlignmentBlocks(Int bits)
	{
		return roundBytesToAlignmentBlocks(roundBitToBytes(bits));
	}

	static Int roundBytesToAlignmentBlocks(Int bytes)
	{
		return (bytes / ALIGNMENT_BLOCK + (bytes % ALIGNMENT_BLOCK ? 1 : 0)) * ALIGNMENT_BLOCK;
	}

	static Int getValueBlocks(Int items_num)
	{
		return items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);
	}

	Int getValueBlocks() const {
		return getValueBlocks(max_size_);
	}

	static Int block_size(Int items_num)
	{
		Int index_size 		= getIndexSize(items_num);

		Int base_length 	= sizeof(MyType) + index_size * Indexes * sizeof(IndexKey);
		Int bitmap_length 	= roundBitsToAlignmentBlocks(items_num);

		return base_length + bitmap_length;
	}

	static Int expected_block_size(Int items_num)
	{
		return block_size(items_num);
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
		typedef ReindexFnBase<MyType> Base;
	public:
		ReindexFn(MyType& me): Base(me) {}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size)
		{
			if (Base::me_.index_size() == 0)
			{
				return;
			}

			const Value* values = Base::me_.values();

			for (Int c = 0; c < Base::size(); c += ValuesPerBranch, index_level_start++)
			{
				Int end;
				if (c + ValuesPerBranch < Base::size())
				{
					end = c + ValuesPerBranch;
				}
				else {
					end = Base::size();
				}

				Base::indexes_[1][index_level_start] = PopCount(values, c, end);
				Base::indexes_[0][index_level_start] = (end - c) - Base::indexes_[1][index_level_start];
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

	Value& operator[](Int idx)
	{
		return value(idx);
	}

	Value& operator[](Int idx) const
	{
		return value(idx);
	}

	void appendValue(Value value) {
		(*this)[size()++] = value;
		reindex();
	}

	BitmapAccessor<Value*, Int, 1>
	value(Int idx)
	{
		return BitmapAccessor<Value*, Int, 1>(values(), idx);
	}

	BitmapAccessor<const Value*, Int, 1>
	value(Int idx) const
	{
		return BitmapAccessor<const Value*, Int, 1>(values(), idx);
	}

	Int setValue(Int idx, Value val)
	{
		value(idx) = val;
		return 0;
	}

	// ==================================== Allocation =========================================== //

private:
	struct InitFn {
		InitFn() {}

		Int getBlockSize(Int items_number) const {
			return MyType::block_size(items_number * 8);
		}

		Int extend(Int items_number) const {
			return items_number;
		}

		Int getIndexSize(Int items_number) const {
			return MyType::compute_index_size(items_number * 8);
		}
	};

public:
	void init(Int block_size)
	{
		size_ = 0;

		max_size_   = MyType::getMaxSize(block_size, InitFn()) * 8;
		index_size_ = getIndexSize(max_size_);
	}

	Int data_size() const
	{
		return roundBitToBytes(size());
	}
//
//	Int getTotalDataSize() const
//	{
//		return max_size() * sizeof(Value);
//	}
//
	Int getDataOffset() const
	{
		return index_size_ * sizeof(IndexKey) * Indexes;
	}

	static Int roundBitToBytes(Int bits)
	{
		return bits / 8 + (bits % 8 > 0);
	}

	Allocator* allocator()
	{
		UByte* my_ptr = T2T<UByte*>(this);
		return T2T<Allocator*>(my_ptr - Base::allocator_offset());
	}

	const Allocator* allocator() const
	{
		const UByte* my_ptr = T2T<const UByte*>(this);
		return T2T<const Allocator*>(my_ptr - Base::allocator_offset());
	}


	void transferTo(MyType* other, Value* target_memory_block = nullptr) const
	{
		if (target_memory_block == nullptr)
		{
			target_memory_block = other->values();
		}

		Int data_size = this->data_size();

		const Value* data = values();

		CopyByteBuffer(data, target_memory_block, data_size);
	}

	bool enlarge(Int bit_amount)
	{
		Int amount = roundBitToBytes(bit_amount);

		Allocator* alloc = allocator();

		if (alloc)
		{
			Int size = block_size();

			MyType other;

			Int new_size = alloc->enlargeBlock(this, size + amount);

			if (new_size)
			{
				other.init(new_size);

				transferTo(&other, T2T<Value*>(buffer_ + other.getDataOffset()));

				*this = other;

				return true;
			}
			else {
				return false;
			}
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
		out<<"Indexes     = "<<Indexes<<endl;
		out<<endl;

		Int idx_max = index_size_;

		out<<"Indexes:"<<endl;

		for (Int c = 0; c < idx_max; c++)
		{
			out<<c<<" ";

			for (Int idx = 0; idx < Indexes; idx++)
			{
				out<<this->indexes(idx)[c]<<" ";
			}

			out<<endl;
		}

		out<<endl;

		out<<"Data:"<<endl;

		dumpSymbols<Value>(out, size_, 1, [this](Int idx) -> Value {
			return this->value(idx);
		});
	}

	// ==================================== Query ========================================== //

	Int rank(Int idx, Int bit)
	{
		BitRankFn<MyType> fn(*this, bit);

		this->walk_range(idx, fn);

		return fn.sum();
	}

	class SelectResult {
		Int idx_;
		Int rank_;
	public:
		SelectResult(Int idx, Int rank): idx_(idx), rank_(rank) {}
		Int idx() const {return idx_;}
		Int rank() const {return rank_;}
	};

	SelectResult select(Int rank, Int bit)
	{
		BitSelectFn<MyType> fn(*this, rank, bit);

		Int idx = this->find_fw(fn);

		if (fn.is_found())
		{
			return SelectResult(idx, rank);
		}
		else {
			return SelectResult(size(), fn.sum() + fn.rank());
		}
	}
};





}


#endif
