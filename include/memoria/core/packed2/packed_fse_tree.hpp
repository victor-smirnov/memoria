
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
	static const Int Indexes        		= Types::Blocks;
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
	using FindLTFnBase = FSEFindElementFnBase<TreeType, btree::BTreeCompareLE, MyType>;

	template <typename TreeType, typename MyType>
	using FindLEFnBase = FSEFindElementFnBase<TreeType, btree::BTreeCompareLT, MyType>;



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
		if (to < ValuesPerBranch)
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

	IndexKey suml(Int to) const
	{
		IndexKey sum = 0;

		const Value* values = this->values();

		for (Int c = 0; c < to; c++) {
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
		FSEFindElementFn<MyType, btree::BTreeCompareLE> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	ValueDescr findLE(IndexKey val) const
	{
		FSEFindElementFn<MyType, btree::BTreeCompareLT> fn(*this, val);

		Int pos = this->find_fw(fn);

		Value actual_value = value(pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.sum());
	}

	ValueDescr findLEl(IndexKey val) const
	{
		FSEFindElementFn<MyType, btree::BTreeCompareLT> fn(*this, val);

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


	void insertSpace(Int idx, Int room_length)
	{
		MEMORIA_ASSERT(idx, <=, size());

		if (capacity() < room_length)
		{
			enlarge(room_length - capacity());
		}

		Value* values = this->values();

		CopyBuffer(values + idx, values + idx + room_length, size() - idx);

		size_ += room_length;
	}

	void removeSpace(Int idx, Int room_length)
	{
		MEMORIA_ASSERT(room_length, <= , size() - idx);

		Value* values = this->values();

		CopyBuffer(values + idx + room_length, values + idx, size() - idx - room_length);

		size_ -= room_length;
	}

	void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		CopyBuffer(this->values() + copy_from, other->values() + copy_to, count);
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
	}


	bool append(Value value) {
		return insert(size_, value);
	}

	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->startGroup("PACKED_TREE");

		handler->value("ALLOCATOR",     &Base::allocator_offset());
		handler->value("SIZE",          &size_);
		handler->value("MAX_SIZE",      &max_size_);
		handler->value("INDEX_SIZE",    &index_size_);

		handler->startGroup("INDEXES", index_size_);

		for (Int idx = 0; idx < index_size_; idx++)
		{
			IndexKey indexes[Blocks];
			for (Int block = 0; block < Blocks; block++)
			{
				indexes[block] = this->indexes(block)[idx];
			}

			handler->value("INDEX", indexes, Blocks);
		}

		handler->endGroup();

		handler->startGroup("DATA", size_);

		for (Int idx = 0; idx < size_; idx++)
		{
			handler->value("TREE_ITEM", &value(idx));
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

		FieldFactory<IndexKey>::serialize(buf, indexes(0), Blocks * index_size());

		FieldFactory<Key>::serialize(buf, values(), size_);
	}

	void deserialize(DeserializationData& buf)
	{
		FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
		FieldFactory<Int>::deserialize(buf, size_);
		FieldFactory<Int>::deserialize(buf, max_size_);
		FieldFactory<Int>::deserialize(buf, index_size_);

		FieldFactory<IndexKey>::deserialize(buf, indexes(0), Blocks * index_size());

		FieldFactory<Key>::deserialize(buf, values(), size_);
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
		values()[idx] += key_value;

		if (index_size() > 0)
		{
			Base::update_up(idx, UpdateUpFn(*this, block_num, key_value));
//			reindex();
		}
	}

};





}


#endif
