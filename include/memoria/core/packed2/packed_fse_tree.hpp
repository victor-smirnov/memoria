
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_TREE_HPP_
#define MEMORIA_CORE_PACKED_FSE_TREE_HPP_

#include <memoria/core/packed2/packed_tree_base.hpp>
#include <memoria/core/packed2/packed_tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/reflection.hpp>

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
	Int Blocks_				= 1,
	typename Allocator_ 	= PackedAllocator,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= PackedTreeBranchingFactor
>
struct PackedFSETreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    static const Int ALIGNMENT_BLOCK        = 8;
};



template <typename Types_>
class PackedFSETree: public PackedTreeBase <
	PackedFSETree<Types_>,
	typename Types_::IndexKey,
	Types_::BranchingFactor,
	Types_::ValuesPerBranch
> {

	typedef  PackedTreeBase<
			PackedFSETree<Types_>,
			typename Types_::IndexKey,
			Types_::BranchingFactor,
			Types_::ValuesPerBranch
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
	static const Int Indexes        		= 1;
	static const Int Blocks        			= Types::Blocks;

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
	using FindLTFnBase = FSEFindElementFnBase<TreeType, PackedCompareLE, MyType>;

	template <typename TreeType, typename MyType>
	using FindLEFnBase = FSEFindElementFnBase<TreeType, PackedCompareLT, MyType>;




private:

	Int size_;
	Int index_size_;
	Int max_size_;

	UByte buffer_[];

public:

	typedef typename MergeLists<
				typename Base::FieldsList,
	            ConstValue<UInt, VERSION>,
	            decltype(size_),
	            decltype(max_size_),
	            decltype(index_size_),
	            Key,
	            IndexKey,
	            Value
	>::Result                                                                   FieldsList;



	PackedFSETree() {}

	void setAllocatorOffset(const void* allocator)
	{
		const char* my_ptr = T2T<const char*>(this);
		const char* alc_ptr = T2T<const char*>(allocator);
		size_t diff = T2T<size_t>(my_ptr - alc_ptr);
		Base::allocator_offset() = diff;
	}

	Int raw_size() const {return size_ * Blocks;}
	Int raw_max_size() const {return max_size_ * Blocks;}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	const Int& index_size() const {return index_size_;}

	const Int& max_size() const {return max_size_;}

	Int content_size_from_start(Int block) const
	{
		IndexKey max = sum(block);
		return this->findLEForward(block, 0, max).idx() + 1;
	}

	Int block_size() const
	{
		return sizeof(MyType) + index_size_ * sizeof(IndexKey) * Indexes + max_size_ * sizeof(Value) * Blocks;
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

	static Int empty_size() {
		return sizeof(MyType);
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
		Int index_size = MyType::index_size(items_num);
		Int raw_block_size = sizeof(MyType) + index_size * Indexes * sizeof(IndexKey) + items_num * sizeof(Value) * Blocks;

		return Allocator::roundUpBytesToAlignmentBlocks(raw_block_size);
	}

	static Int elements_for(Int block_size)
	{
		return tree_size(block_size);
	}

	static Int expected_block_size(Int items_num)
	{
		return block_size(items_num);
	}

	static Int index_size(Int items_number)
	{
		if (items_number > ValuesPerBranch)
		{
			return MyType::compute_index_size(items_number * Blocks);
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


			IndexKey cell = 0;

			for (Int c = 0; c < Base::size(); c++)
			{
				cell += Base::me_[c];

				if (c == limit)
				{
					Base::indexes_[0][index_level_start] = cell;

					index_level_start++;
					cell = 0;

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


	class CheckFn: public CheckFnBase<MyType> {
		typedef CheckFnBase<MyType> Base;
	public:
		CheckFn(const MyType& me): Base(me) {}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size) const
		{
			if (Base::me_.index_size() == 0)
			{
				return;
			}

			Int limit = (ValuesPerBranch - 1 < Base::size()) ? ValuesPerBranch - 1 : Base::size() - 1;


			IndexKey cell = 0;

			for (Int c = 0; c < Base::size(); c++)
			{
				cell += Base::me_[c];

				if (c == limit)
				{
					//Base::indexes_[0][index_level_start] = cell;

					if (Base::indexes_[0][index_level_start] != cell)
					{
						throw Exception(MA_SRC,
								SBuf()<<"Invalid first index: index["<<0<<"]["
									  <<index_level_start<<"]="<<Base::indexes_[0][index_level_start]
								      <<", actual="<<cell);
					}

					index_level_start++;
					cell = 0;

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

	void check() const
	{
		CheckFn fn(*this);
		Base::reindex(0, size(), fn);
	}


	// ==================================== Value ========================================== //

	Value& operator[](Int idx)
	{
		return value(idx);
	}

	const Value& operator[](Int idx) const
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
		return *T2T<const Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes + idx * sizeof(Value));
	}


	Value& value(Int block, Int idx)
	{
		return *T2T<Value*>(buffer_ + index_size_ * sizeof(IndexKey) * Indexes + (idx + block * size_) * sizeof(Value));
	}

	const Value& value(Int block, Int idx) const
	{
		return *T2T<const Value*>(buffer_+index_size_ * sizeof(IndexKey) * Indexes + (idx + block * size_) * sizeof(Value));
	}

	Int setValue(Int idx, Value val)
	{
		value(idx) = val;
		return 0;
	}

	Value* values()
	{
		return &value(0);
	}

	const Value* values() const
	{
		return &value(0);
	}

	Value* values(Int block)
	{
		return &value(block, 0);
	}

	const Value* values(Int block) const
	{
		return &value(block, 0);
	}

	Value last_value(Int block) const {
		return value(block, size_ - 1);
	}

	Value first_value(Int block) const {
		return value(block, 0);
	}

	void clearIndex()
	{
		for (Int i = 0; i < Indexes; i++)
		{
			auto index = this->indexes(i);

			for (Int c = 0; c < index_size_; c++)
			{
				index[c] = 0;
			}
		}
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

	// ==================================== Allocation =========================================== //

private:
	struct InitFn {
		Int block_size(Int items_number) const {
			return MyType::block_size(items_number);
		}

		Int max_elements(Int block_size)
		{
			return block_size;
		}
	};

public:
	static Int tree_size(Int block_size)
	{
		return block_size >= (Int)sizeof(Value) ? FindTotalElementsNumber2(block_size, InitFn()) : 0;
	}

	void init(Int block_size)
	{
		size_ = 0;

		max_size_   = tree_size(block_size);
		index_size_ = index_size(max_size_);
	}

	void object_size() const
	{
		Int object_size = sizeof(MyType) + getDataOffset() + data_size();
		return Allocator::roundUpBytesToAlignmentBlocks(object_size);
	}

	Int data_size() const
	{
		return size() * sizeof(Value) * Blocks;
	}

	Int getTotalDataSize() const
	{
		return max_size() * sizeof(Value) * Blocks;
	}

	Int getDataOffset() const
	{
		return index_size_ * sizeof(IndexKey) * Indexes;
	}

	Int capacity() const {
		return max_size_ - size_;
	}

	Int total_capacity() const
	{
		Int my_size		= allocator()->element_size(this);
		Int free_space  = allocator()->free_space();
		Int total_size 	= MyType::tree_size(my_size + free_space);
		Int capacity 	= total_size - size_;

		return capacity >= 0 ? capacity : 0;
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

	void enlarge(Int items_num)
	{
		Allocator* alloc = allocator();

		MyType other;

		Int requested_block_size = MyType::block_size(max_size_ + items_num);

		Int new_size = alloc->resizeBlock(this, requested_block_size);

		other.init(new_size);
		other.size() 				= this->size();
		other.allocator_offset() 	= this->allocator_offset();

		MEMORIA_ASSERT(other.size(), <=, other.max_size());
		MEMORIA_ASSERT(other.capacity(), >=, items_num);

		transferTo(&other, T2T<Value*>(buffer_ + other.getDataOffset()));

		*this = other;
	}

	void shrink(Int items_num)
	{
		MEMORIA_ASSERT(items_num, <=, max_size_);
		MEMORIA_ASSERT(max_size_ - items_num, >=, size_);

		Allocator* alloc = allocator();

		MyType other;

		Int requested_block_size = MyType::block_size(max_size_ - items_num);
		Int current_block_size	 = alloc->element_size(this);

		if (requested_block_size < current_block_size)
		{
			other.init(requested_block_size);
			other.size() 				= this->size();
			other.allocator_offset() 	= this->allocator_offset();

			MEMORIA_ASSERT(other.size(), <=, other.max_size());

			transferTo(&other, T2T<Value*>(buffer_ + other.getDataOffset()));

			*this = other;

			alloc->resizeBlock(this, requested_block_size);
		}
	}




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

		for (Int c = 0; c < size_; c++)
		{
			out<<c<<" ";

			for (Int block = 0; block < Blocks; block++)
			{
				out<<value(block, c)<<" ";
			}

			out<<endl;
		}
	}

	// ==================================== Query ========================================== //

	IndexKey sum0(Int to) const
	{
		if (index_size_ == 0 || to < ValuesPerBranch)
		{
			IndexKey sum = 0;

			const Value* values = this->values();

			for (Int c = 0; c < to; c++)
			{
				sum += values[c];
			}

			return sum;
		}
		else {
			GetFSEValuesSumFn<MyType> fn(*this);

			this->walk_range(to, fn);

			return fn.sum();
		}
	}

	IndexKey sum0(Int from, Int to) const
	{
		return sum0(to) - sum0(from);
	}


	IndexKey sum(Int block) const
	{
		return sum(block, size_);
	}

	IndexKey sum(Int block, Int to) const
	{
		Int base = block * size_;
		return sum0(base, base + to);
	}


	IndexKey sumWithoutLastElement(Int block) const
	{
		return sum(block, size_ - 1);
	}

	IndexKey suml(Int to) const
	{
		IndexKey sum = 0;

		const Value* values = this->values();

		for (Int c = 0; c < to; c++) {
			sum += values[c];
		}

		return sum;
	}

	IndexKey sum(Int block, Int from, Int to) const
	{
		Int base = block * size_;
		return sum0(base + to) - sum0(base + from);
	}

	ValueDescr findLTForward(IndexKey val) const
	{
		FSEFindElementFn<MyType, PackedCompareLE> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value, pos, fn.sum());
	}


	ValueDescr findLTForward(Int start, IndexKey val) const
	{
		auto prefix = start > 0 ? sum(start) : 0;

		FSEFindElementFn<MyType, PackedCompareLE> fn(*this, val + prefix);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value, pos, fn.sum() - prefix);
	}


	ValueDescr findLTForward(Int block, Int start, IndexKey val) const
	{
		Int block_start = block * size_;

		auto prefix = sum0(block_start + start);

		FSEFindElementFn<MyType, PackedCompareLE> fn(*this, val + prefix);

		Int pos = this->find_fw(fn);

		if (pos < block_start + size_)
		{
			Value actual_value = value(pos);

			return ValueDescr(actual_value, pos - block_start, fn.sum() - prefix);
		}
		else {
			return ValueDescr(0, size_, sum(block) - prefix);
		}
	}


	ValueDescr findLTBackward(Int start, IndexKey val) const
	{
		auto prefix = sum0(start + 1);
		auto target = prefix - val;

		if (target > 0)
		{
			FSEFindElementFn<MyType, PackedCompareLE> fn(*this, target);

			Int pos = this->find_fw(fn);

			Value actual_value = value(pos);

			return ValueDescr(actual_value, pos, prefix - (fn.sum() + actual_value));
		}
		else if (target == 0)
		{
			Value actual_value = value(start);
			return ValueDescr(actual_value, start, prefix - actual_value);
		}
		else {
			return ValueDescr(0, -1, prefix);
		}
	}


	ValueDescr findLTBackward(Int block, Int start, IndexKey val) const
	{
		Int block_start = block * size_;

		auto prefix = sum0(block_start + start + 1);
		auto target = prefix - val;

		if (target >= 0)
		{
			FSEFindElementFn<MyType, PackedCompareLE> fn(*this, target);

			Int pos = this->find_fw(fn);

			if (pos > block_start + start)
			{
				return ValueDescr(0, start, 0);
			}
			else if (pos >= block_start)
			{
				Value actual_value = value(pos);
				return ValueDescr(actual_value, pos - block_start, prefix - (fn.sum() + actual_value));
			}
			else {
				return ValueDescr(0, -1, prefix - sum0(block_start));
			}
		}
		else {
			return ValueDescr(0, -1, prefix);
		}
	}




	ValueDescr findLEForward(IndexKey val) const
	{
		FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	ValueDescr findLEForward(Int start, IndexKey val) const
	{
		auto prefix = start > 0 ? sum0(start) : 0;

		FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos + prefix);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum() - prefix);
	}

	ValueDescr findLEForward(Int block, Int start, IndexKey val) const
	{
		Int block_start = block * size_;

		auto prefix = sum0(block_start + start);

		FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val + prefix);

		Int pos = this->find_fw(fn);

		if (pos < block_start + size_)
		{
			Value actual_value = value(pos);

			return ValueDescr(actual_value, pos - block_start, fn.sum() - prefix);
		}
		else {
			return ValueDescr(0, size_, sum(block) - prefix);
		}
	}

	ValueDescr findLEBackward(Int block, Int start, IndexKey val) const
	{
		Int block_start = block * size_;

		auto prefix = sum0(block_start + start + 1);
		auto target = prefix - val;

		if (target >= 0)
		{
			FSEFindElementFn<MyType, PackedCompareLT> fn(*this, target);

			Int pos = this->find_fw(fn);

			if (pos >= block_start)
			{
				Value actual_value = value(pos);
				return ValueDescr(actual_value, pos - block_start, prefix - (fn.sum() + actual_value));
			}
			else {
				return ValueDescr(0, -1, prefix - sum0(block_start));
			}
		}
		else if (target == 0)
		{
			Value actual_value = value(0);
			return ValueDescr(actual_value, 0, prefix - actual_value);
		}
		else {
			return ValueDescr(0, -1, prefix);
		}
	}

//	ValueDescr findLEForward(Int start, IndexKey val) const
//	{
//		auto prefix = sum(start);
//
//		FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val);
//
//		Int pos = this->find_fw(fn);
//
//		Value actual_value = value(pos + prefix);
//
//		return ValueDescr(actual_value + fn.sum(), pos, fn.sum() - prefix);
//	}


	ValueDescr findForward(SearchType search_type, Int block, Int start, IndexKey val) const
	{
		if (search_type == SearchType::LT)
		{
			return findLTForward(block, start, val);
		}
		else {
			return findLEForward(block, start, val);
		}
	}

	ValueDescr findBackward(SearchType search_type, Int block, Int start, IndexKey val) const
	{
		if (search_type == SearchType::LT)
		{
			return findLTBackward(block, start, val);
		}
		else {
			return findLEBackward(block, start, val);
		}
	}


	ValueDescr findLEl(IndexKey val) const
	{
		FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val);

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

	void insert(Int idx, Value value)
	{
		insertSpace(idx, 1);

		this->value(idx) = value;
	}

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

	void insertSpace(Int idx, Int room_length)
	{
		if (idx > this->size()) {
			int a = 0; a++;
		}

		MEMORIA_ASSERT(idx, <=, this->size());

		if (capacity() < room_length)
		{
			enlarge(room_length - capacity());
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

	void removeSpace(Int idx, Int room_length)
	{
		MEMORIA_ASSERT(room_length, <= , size() - idx);

		Value* values = this->values();

		for (Int block = 0; block < Blocks; block++)
		{
			Int offset = (Blocks - block - 1) * size_ + idx;

			CopyBuffer(
					values + offset + room_length,
					values + offset,
					size_ * Blocks - offset - room_length * (1 - block)
			);
		}

		size_ -= room_length;

		shrink(room_length);
	}

	void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		for (Int block = 0; block < Blocks; block++)
		{
			Int my_offset		= block * size_;
			Int other_offset 	= block * other->size_;

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
		const auto* my_values 	= values();
		auto* other_values 		= other->values();

		Int size = this->size();

		for (Int c = 0; c < size * Blocks; c++)
		{
			other_values[c] 	= my_values[c];
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


	bool append(Value value) {
		return insert(size_, value);
	}


	// ===================================== IO ============================================ //

	void insert(IData* data, Int pos, Int length)
	{
//		IDataSource<Value>* src = static_cast<IDataSource<Value>*>(data);
//		insertSpace(pos, length);
//
//		BigInt to_write_local = length;
//
//		while (to_write_local > 0)
//		{
//			SizeT processed = src->get(buffer_, pos, to_write_local);
//
//			pos 			+= processed;
//			to_write_local 	-= processed;
//		}
	}

	void update(IData* data, Int pos, Int length)
	{
//		MEMORIA_ASSERT(pos, <=, size_);
//		MEMORIA_ASSERT(pos + length, <=, size_);
//
//		IDataSource<Value>* src = static_cast<IDataSource<Value>*>(data);
//
//		BigInt to_write_local = length;
//
//		while (to_write_local > 0)
//		{
//			SizeT processed = src->get(buffer_, pos, to_write_local);
//
//			pos 			+= processed;
//			to_write_local 	-= processed;
//		}
	}

	void read(IData* data, Int pos, Int length) const
	{
//		MEMORIA_ASSERT(pos, <=, size_);
//		MEMORIA_ASSERT(pos + length, <=, size_);
//
//		IDataTarget<Value>* tgt = static_cast<IDataTarget<Value>*>(data);
//
//		BigInt to_read_local = length;
//
//		while (to_read_local > 0)
//		{
//			SizeT processed = tgt->put(buffer_, pos, to_read_local);
//
//			pos 			+= processed;
//			to_read_local 	-= processed;
//		}
	}



	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->startGroup("PACKED_TREE");

		handler->value("ALLOCATOR",     &Base::allocator_offset());
		handler->value("SIZE",          &size_);
		handler->value("MAX_SIZE",      &max_size_);
		handler->value("INDEX_SIZE",    &index_size_);

		handler->startGroup("INDEXES", index_size_);

		for (Int c = 0; c < index_size_; c++)
		{
			IndexKey indexes[Indexes];
			for (Int idx = 0; idx < Indexes; idx++)
			{
				indexes[idx] = this->indexes(idx)[c];
			}

			handler->value("INDEX", indexes, Indexes);
		}

		handler->endGroup();

		handler->startGroup("DATA", size_);

		for (Int idx = 0; idx < size_ ; idx++)
		{
			Value values[Blocks];
			for (Int block = 0; block < Blocks; block++)
			{
				values[block] = value(block, idx);
			}

			handler->value("TREE_ITEM", values, Blocks);
		}

		handler->endGroup();

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
		FieldFactory<Int>::serialize(buf, size_);
		FieldFactory<Int>::serialize(buf, max_size_);
		FieldFactory<Int>::serialize(buf, index_size_);

		FieldFactory<IndexKey>::serialize(buf, indexes(0), Indexes * index_size());

		FieldFactory<Value>::serialize(buf, values(), size_ * Blocks);
	}

	void deserialize(DeserializationData& buf)
	{
		FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
		FieldFactory<Int>::deserialize(buf, size_);
		FieldFactory<Int>::deserialize(buf, max_size_);
		FieldFactory<Int>::deserialize(buf, index_size_);

		FieldFactory<IndexKey>::deserialize(buf, indexes(0), Indexes * index_size());

		FieldFactory<Value>::deserialize(buf, values(), size_ * Blocks);
	}

private:

	class UpdateUpFn {

		MyType& me_;

		IndexKey* indexes_;

		Value value_;

	public:
		UpdateUpFn(MyType& me, Int index, Value value):
			me_(me), indexes_(me_.indexes(index)), value_(value)
		{}

		Int size() const {
			return me_.size();
		}

		Int maxSize() const {
			return me_.max_size();
		}

		Int indexSize() const {
			return me_.index_size();
		}

		void operator()(Int idx)
		{
			indexes_[idx] += value_;
		}
	};

public:


	void updateUp(Int block_num, Int idx, IndexKey key_value)
	{
		value(block_num, idx) += key_value;

		if (index_size() > 0)
		{
			Base::update_up(idx, UpdateUpFn(*this, block_num, key_value));
		}
	}

};





}


#endif
