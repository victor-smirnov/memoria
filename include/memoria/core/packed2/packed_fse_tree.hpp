
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_TREE_HPP_
#define MEMORIA_CORE_PACKED_FSE_TREE_HPP_

#include <memoria/core/packed2/packed_tree_base.hpp>
#include <memoria/core/packed/tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>

namespace memoria {

template <typename Value, typename Walker = EmptyType>
class FSEValueDescr {
	Value value_;
	Value prefix_;
	Int idx_;

	Walker walker_;
public:
	FSEValueDescr(BigInt value, Int idx, Value prefix, const Walker& walker = EmptyType()):
		value_(value),
		prefix_(prefix),
		idx_(idx),
		walker_(walker)
	{}

	Value value() const 	{return value_;}
	Value prefix() const 	{return prefix_;}
	Int idx() const 		{return idx_;}

	const Walker& walker() const 	{return walker_;}
	Walker& walker() 		 		{return walker_;}
};


template <
	typename K,
	typename IK,
	typename V,
	typename Allocator_ = PackedAllocator,
	Int BF = PackedTreeBranchingFactor,
	Int VPB = PackedTreeBranchingFactor
>
struct PackedFSETreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = 1;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    static const Int ALIGNMENT_BLOCK        = 8;
};



template <typename Types_>
class PackedFSETree: public PackedTreeBase<
	PackedFSETree<Types_>,
	typename Types_::IndexKey,
	Types_::BranchingFactor,
	Types_::ValuesPerBranch,
	Types_::ALIGNMENT_BLOCK
> {

	typedef  PackedTreeBase<
			PackedFSETree<Types_>,
			typename Types_::IndexKey,
			Types_::BranchingFactor,
			Types_::ValuesPerBranch,
			Types_::ALIGNMENT_BLOCK
	>																			Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSETree<Types>               									MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::Key													Key;
	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;

	static const Int ALIGNMENT_BLOCK		= 8; //Bytes


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

	typedef FSEValueDescr<IndexKey> 											ValueDescr;

	template <typename TreeType, typename MyType>
	using FindLTFnBase = FSEFindElementFnBase<TreeType, BTreeCompareLE, MyType>;

	template <typename TreeType, typename MyType>
	using FindLEFnBase = FSEFindElementFnBase<TreeType, BTreeCompareLT, MyType>;

private:

	Int size_;
	Int index_size_;
	Int max_size_;

	UByte buffer_[];

public:
	PackedFSETree() {}

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
		return sizeof(MyType) + index_size_ * sizeof(IndexKey) * Indexes + max_size_ * sizeof(Value);
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

	Value* getValues()
	{
		return buffer_ + index_size_ * sizeof(IndexKey) * Indexes;
	}

	const Value* getValues() const
	{
		return buffer_ + index_size_ * sizeof(IndexKey) * Indexes;
	}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / ALIGNMENT_BLOCK + (value % ALIGNMENT_BLOCK ? 1 : 0)) * ALIGNMENT_BLOCK;
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

	static Int block_size(Int items_num)
	{
		Int index_size = getIndexSize(items_num);
		Int raw_block_size = sizeof(MyType) + index_size * Indexes * sizeof(IndexKey) + items_num * sizeof(Value);

		return Allocator::roundUpBytesToAlignmentBlocks(raw_block_size);
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

			Int limit = (ValuesPerBranch - 1 < Base::size()) ? ValuesPerBranch - 1 : Base::size() - 1;

			for (Int c = 0; c < Base::size(); c++)
			{
				Base::indexes_[0][index_level_start] += Base::me_[c];

				if (c == limit)
				{
					index_level_start++;

					if (limit + ValuesPerBranch < Base::size())
					{
						limit += ValuesPerBranch;
					}
					else {
						limit = Base::size() - 1;
					}
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

	Value& value(Int idx)
	{
		return *T2T<Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes + idx * sizeof(Value));
	}

	const Value& value(Int idx) const
	{
		return *T2T<const Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes + idx*sizeof(Value));
	}

	Int setValue(Int idx, Value val)
	{
		value(idx) = val;
		return 0;
	}

	Value* values() {
		return &value(0);
	}

	const Value* values() const {
		return &value(0);
	}

	// ==================================== Allocation =========================================== //

private:
	struct InitFn {
//		Int getBlockSize(Int items_number) const {
//			return MyType::block_size(items_number);
//		}
//
//		Int extend(Int items_number) const {
//			return items_number;
//		}
//
//		Int getIndexSize(Int items_number) const {
//			return MyType::compute_index_size(items_number);
//		}

		Int block_size(Int items_number) const {
			return MyType::block_size(items_number);
		}

		Int max_elements(Int block_size)
		{
			return block_size * 8;
		}
	};

public:
	void init(Int block_size)
	{
		size_ = 0;

		max_size_   = block_size >= (Int)sizeof(Value) ? FindTotalElementsNumber2(block_size, InitFn()) : 0;
		index_size_ = getIndexSize(max_size_);
	}

	Int data_size() const
	{
		return size() * sizeof(Value);
	}

	Int getTotalDataSize() const
	{
		return max_size() * sizeof(Value);
	}

	Int getDataOffset() const
	{
		return index_size_ * sizeof(IndexKey) * Indexes;
	}

	Int capacity() const {
		return max_size_ - size_;
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

	bool enlarge(Int items_num)
	{
		Allocator* alloc = allocator();

		if (alloc)
		{
			MyType other;

			Int requested_block_size = MyType::block_size(max_size_ + items_num);

			Int new_size = alloc->enlargeBlock(this, requested_block_size);

			if (new_size)
			{
				other.init(new_size);
				other.size() 				= this->size();
				other.allocator_offset() 	= this->allocator_offset();

				MEMORIA_ASSERT(other.size(), <=, other.max_size());
				MEMORIA_ASSERT(other.capacity(), >=, items_num);

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

//	void shrink(Int amount)
//	{
//		Allocator* alloc = allocator();
//		Int size = block_size();
//
//		MEMORIA_ASSERT(size - amount, >=, 0);
//
//		MyType other;
//		other.init(size - amount);
//
//		Int new_size = alloc->shrinkBlock(this, size - amount);
//
//		other.init(new_size);
//
//		transferTo(&other, buffer_ + other.getDataOffset());
//	}


	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout, bool dump_index = true) const
	{
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;

		if (dump_index)
		{
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
		}

		out<<endl;

		out<<"Data:"<<endl;

		dumpArray<Value>(out, size_, [this](Int idx) -> Value {
			return this->value(idx);
		});
	}

	// ==================================== Query ========================================== //

	IndexKey sum() const {
		return sum(size_);
	}

	IndexKey sum(Int to) const
	{
		GetFSEValuesSumFn<MyType> fn(*this);

		this->walk_range(to, fn);

		return fn.sum();
	}

	IndexKey suml(Int to) const
	{
		IndexKey sum = 0;

		cout<<"=================== SUML: "<<to<<" ================================"<<endl;

		const Value* values = this->values();

		for (Int c = 0; c < to; c++) {
//			cout<<c<<" "<<values[c]<<" "<<sum<<endl;
			sum += values[c];
		}

		return sum;
	}

	IndexKey sum(Int from, Int to) const
	{
		return sum(to) - sum(from);
	}

	ValueDescr findLT(IndexKey val) const
	{
		FSEFindElementFn<MyType, BTreeCompareLE> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	ValueDescr findLE(IndexKey val) const
	{
		FSEFindElementFn<MyType, BTreeCompareLT> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	ValueDescr findLEl(IndexKey val) const
	{
		FSEFindElementFn<MyType, BTreeCompareLT> fn(*this, val);

		Int pos = fn.walkLastValuesBlock(0);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	template <
		template <typename TreeType, template <typename, typename> class BaseClass> class Extender
	>
	FSEValueDescr<
		Value,
		Extender<MyType, FindLEFnBase>
	>
	find_le(Value value) const
	{
		Extender<MyType, FindLEFnBase> fn(*this, value);

		Int pos = this->find_fw(fn);

		Value actual_value = this->value(pos);

		return FSEValueDescr<Value, Extender<MyType, FindLEFnBase>>(actual_value + fn.sum(), pos, fn.sum(), fn);
	}


	template <
		template <typename TreeType, template <typename, typename> class BaseClass> class Extender
	>
	FSEValueDescr<
		Value,
		Extender<MyType, FindLTFnBase>
	>
	find_lt(Value value) const
	{
		Extender<MyType, FindLTFnBase> fn(*this, value);

		Int pos = this->find_fw(fn);

		Value actual_value = this->value(pos);

		return FSEValueDescr<Value, Extender<MyType, FindLTFnBase>>(actual_value + fn.sum(), pos, fn.sum(), fn);
	}

	// ==================================== Update ========================================== //

	bool insert(Int idx, Value value)
	{
		if (capacity() == 0)
		{
			if (!enlarge(1))
			{
				return false;
			}
		}

		MEMORIA_ASSERT(idx, <=, size());

		Value* values = this->values();

		CopyBuffer(values + idx, values + idx + 1, size() - idx);

		this->value(idx) = value;

		size_++;

		return true;
	}

	bool append(Value value) {
		return insert(size_, value);
	}
};





}


#endif
