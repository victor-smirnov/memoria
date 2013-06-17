
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_TREE2_HPP_
#define MEMORIA_CORE_PACKED_VLE_TREE2_HPP_

#include <memoria/core/packed2/packed_tree_tools.hpp>
#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/packed2/packed_tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria {

using namespace vapi;

template <
	typename IK,
	typename V,
	template <typename> class CodecType = UByteExintCodec,
	Int Blocks_ 			= 2,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= PackedExintVLETreeValuesPerBranch,
	typename Allocator_ 	= PackedAllocator
>
struct PackedVLETreeTypes {
    typedef IK              IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;

    template <typename VV>
    using Codec = CodecType<VV>;
};


template <typename Value>
class VLETreeValueDescr {
	Value value_;
	Int pos_;
	Int idx_;
	Value prefix_;
public:
	VLETreeValueDescr(BigInt value, Int pos, Int idx, Value prefix = 0):
		value_(value),
		pos_(pos),
		idx_(idx),
		prefix_(prefix)
		{}

	Value value() const 	{return value_;}
	Int pos() const 		{return pos_;}
	Int idx() const 		{return idx_;}
	Value prefix() const 	{return prefix_;}
};


template <typename Types_>
class PackedVLETree: public PackedAllocator
{
	typedef PackedAllocator														Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedVLETree<Types>               									MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	typedef UBigInt																OffsetsType;

	typedef typename Types::template Codec<Value>								Codec;
	typedef typename Codec::BufferType											BufferType;



	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= 2;
	static const Int Blocks 				= Types::Blocks;
	static const Int IOBatchSize			= 64;

	static const Int BITS_PER_OFFSET		= Codec::BitsPerOffset;

	typedef core::StaticVector<Int, Blocks>										Dimension;
	typedef core::StaticVector<Dimension, 2>									BlockRange;
	typedef core::StaticVector<Value, Blocks>									Values;

	typedef PackedTreeTools<IndexKey, BranchingFactor, ValuesPerBranch>			TreeTools;

	typedef VLETreeValueDescr<IndexKey>											ValueDescr;

	class Metadata {
		Int size_;
		Int data_size_;
		Int index_size_;
	public:
		Metadata() {}

		const Int& size() const 		{return size_;};
		const Int& data_size() const 	{return data_size_;};
		Int index_size() const 			{return index_size_;};

		Int& size() 			{return size_;};
		Int& data_size() 		{return data_size_;};
		Int& index_size() 		{return index_size_;};

		template<typename> friend class PackedVLETree;
	};

public:
	PackedVLETree() {}

public:
	Metadata* metadata() {
		return Base::template get<Metadata>(0);
	}

	const Metadata* metadata() const {
		return Base::template get<Metadata>(0);
	}

	Int& size() {return metadata()->size();}
	const Int& size() const {return metadata()->size();}

	Int raw_size() const {return size() * Blocks;}

	const Int index_size() const {return metadata()->index_size();}

	const Int& data_size() const {return metadata()->data_size();}
	Int& data_size() {return metadata()->data_size();}

	const Int max_data_size() const
	{
		return element_size(3) * 8 / Codec::ElementSize;
	}

	OffsetsType* offsetsBlock() {
		return Base::template get<OffsetsType>(1);
	}

	const OffsetsType* offsetsBlock() const {
		return Base::template get<OffsetsType>(1);
	}

	Int offsetsBlockLength() const {
		return Base::element_size(1) / sizeof(OffsetsType);
	}

	IndexKey* indexes(Int index_block)
	{
		return Base::template get<IndexKey>(2) + index_block * index_size();
	}

	IndexKey* sizes() {
		return indexes(0);
	}

	const IndexKey* indexes(Int index_block) const
	{
		return Base::template get<IndexKey>(2) + index_block * index_size();
	}

	const IndexKey* sizes() const {
		return indexes(0);
	}

	BufferType* values()
	{
		return Base::template get<BufferType>(3);
	}

	const BufferType* values() const
	{
		return Base::template get<BufferType>(3);
	}

	static Int getValueBlocks(Int items_num)
	{
		return items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);
	}

	Int getValueBlocks() const {
		return getValueBlocks(max_data_size());
	}

	Int getOffsetsLengts() const
	{
		return getOffsetsBlockLength(max_data_size());
	}

	static Int getOffsetsBlockLength(Int items_num)
	{
		Int value_blocks = getValueBlocks(items_num);

		Int offsets_bits = value_blocks * BITS_PER_OFFSET;

		Int offsets_bytes = Base::roundUpBitsToAlignmentBlocks(offsets_bits);

		return offsets_bytes;
	}

	Int block_size() const {
		return Base::block_size();
	}


	Int block_size(const MyType* other) const
	{
		Int my_max 		= this->max_data_size();
		Int other_max 	= other->max_data_size();

		return block_size(my_max + other_max);
	}


	static Int block_size(Int max_items_num)
	{
		Int metadata_length	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
		Int offsets_length 	= Base::roundUpBytesToAlignmentBlocks(getOffsetsBlockLength(max_items_num));

		Int index_size 		= MyType::index_size(max_items_num);
		Int index_length	= Base::roundUpBytesToAlignmentBlocks(index_size * Indexes * sizeof(IndexKey));

		Int values_length	= Base::roundUpBitsToAlignmentBlocks(max_items_num * Codec::ElementSize);

		return Base::block_size(metadata_length + offsets_length + index_length + values_length, 4);
	}

	static Int max_tree_size(Int block_size)
	{
		return FindTotalElementsNumber2(block_size, InitFn());
	}

	static Int elements_for(Int block_size)
	{
		return max_tree_size(block_size);
	}

	static Int index_size(Int items_number)
	{
		if (items_number > ValuesPerBranch)
		{
			return TreeTools::compute_index_size(items_number);
		}
		else {
			return 0;
		}
	}

	// ================================= Reindexing ======================================== //

private:

	class ReindexFn: public Reindex2FnBase<MyType> {

		typedef Reindex2FnBase<MyType> 				Base;

		const BufferType* values_;

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

			Base::me_.offset(0) = 0;

			Int max_size = Base::me_.data_size();

			IndexKey size_cell 	= 0;
			IndexKey value_cell = 0;

			Int size = Base::me_.raw_size();

			for (Int c = 0; c < size; c++)
			{
				Value value;
				Int len = codec.decode(values_, value, pos, max_size);



				size_cell 	+= 1;
				value_cell	+= value;

				pos += len;

				if (pos >= limit)
				{
					Int idx = index_level_start + value_block;

					Base::indexes_[0][idx] = size_cell;
					Base::indexes_[1][idx] = value_cell;

					value_cell = size_cell = 0;

					value_block++;

					if (value_block < Base::me_.offsets())
					{
						Base::me_.offset(value_block) = pos - limit;
					}

					limit += ValuesPerBranch;
				}
				else if (c == size - 1)
				{
					Int idx = index_level_start + value_block;

					Base::indexes_[0][idx] = size_cell;
					Base::indexes_[1][idx] = value_cell;
				}
			}
		}
	};



	class CheckFn: public Check2FnBase<MyType> {

		typedef Check2FnBase<MyType> 				Base;

		const BufferType* values_;
	public:
		Int data_size_ 	= 0;
		Int size_		= 0;

	public:
		CheckFn(const MyType& me): Base(me)
		{
			values_ = me.values();
		}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size)
		{
			Int size = Base::me_.raw_size();

			Codec codec;

			Int pos = 0;

			Int limit = ValuesPerBranch;
			Int value_block = 0;

			Int max_size = Base::me_.data_size();

			IndexKey size_cell 	= 0;
			IndexKey value_cell = 0;


			if (Base::indexSize() == 0)
			{
				while(pos < max_size)
				{
					Value value;
					Int len = codec.decode(values_, value, pos, max_size);

					data_size_ += len;
					size_++;

					pos += len;
				}
			}
			else {
				for (Int c = 0; c < size; c++)
				{
					Value value;
					Int len = codec.decode(values_, value, pos, max_size);

					data_size_ += len;
					size_++;

					size_cell 	+= 1;
					value_cell	+= value;

					pos += len;

					if (pos >= limit)
					{
						Int idx = index_level_start + value_block;

						MEMORIA_ASSERT(Base::indexes_[0][idx], ==, size_cell);
						MEMORIA_ASSERT(Base::indexes_[1][idx], ==, value_cell);

						value_cell = size_cell = 0;

						value_block++;

						if (value_block < Base::me_.offsets())
						{
							Int offset1 = Base::me_.offset(value_block);
							Int offset2 = (pos - limit);

							MEMORIA_ASSERT(offset1, ==, offset2);
						}

						limit += ValuesPerBranch;
					}
					else if (c == size - 1)
					{
						Int idx = index_level_start + value_block;

						MEMORIA_ASSERT(Base::indexes_[0][idx], ==, size_cell);
						MEMORIA_ASSERT(Base::indexes_[1][idx], ==, value_cell);
					}


				}
			}
		}
	};


public:

	void reindex()
	{
		ReindexFn fn(*this);
		TreeTools::reindex(0, size() * Blocks, fn);
	}

	void check() const
	{
		CheckFn fn(*this);
		TreeTools::reindex(0, size() * Blocks, fn);

		MEMORIA_ASSERT(data_size(), <=, max_data_size());
		MEMORIA_ASSERT(fn.data_size_, ==, data_size());
		MEMORIA_ASSERT(fn.size_, ==, size());
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
		Int size = this->size();
		if (idx < size)
		{
			GetValueOffsetFn fn(*this, idx);

			Int pos = TreeTools::find_fw(fn);

			return pos;
		}
		else if (size > 0)
		{
			GetValueOffsetFn fn(*this, idx - 1);

			Int pos = TreeTools::find_fw(fn);

			Codec codec;

			size_t len = codec.length(values(), pos, this->data_size());

			return pos + len;
		}
		else {
			return 0;
		}
	}

	Int max_offset() const
	{
		return getValueOffset(size() * Blocks);
	}

	Int max_offset(Int block) const
	{
		return getValueOffset(size() * (block + 1));
	}

	Int data_length(Int block, Int start, Int end) const
	{
		Int size 		= this->size();
		Int pos_start 	= getValueOffset(size * block + start);
		Int pos_end 	= getValueOffset(size * block + end);

		return pos_end - pos_start;
	}

	Int getValueBlockOffset(Int block) const
	{
		GetValueOffsetFn fn(*this, block * size());

		Int pos = TreeTools::find_fw(fn);

		return pos;
	}

	Value getValue(Int idx) const
	{
		const auto* values_ = values();

		MEMORIA_ASSERT(idx, >=, 0);
		MEMORIA_ASSERT(idx, <, raw_size());

		Int pos = getValueOffset(idx);

		Codec codec;
		Value value;

		MEMORIA_ASSERT(pos, <, data_size());

		codec.decode(values_, value, pos, data_size());
		return value;
	}

	Value getValue(Int block, Int idx) const
	{
		Int value_idx = size() * block + idx;

		return getValue(value_idx);
	}


	Int setValue(Int idx, Value value)
	{
		MEMORIA_ASSERT(idx, >=, 0);
		MEMORIA_ASSERT(idx, <=, raw_size());

		auto* values_ = values();

		Int pos = getValueOffset(idx);

		MEMORIA_ASSERT(pos, <=, this->max_data_size());

		Int total_size = data_size();

		Codec codec;

		Int value_len = codec.length(value);

		Int stored_value_len = codec.length(values_, pos, this->max_data_size());
		Int delta 	 		 = value_len - stored_value_len;

		if (delta > 0)
		{
			Int capacity = max_data_size() - total_size;

			if (capacity < delta)
			{
				enlarge(delta);
			}
		}

		auto* meta = this->metadata();

		Int data_size = meta->data_size();

		MEMORIA_ASSERT(pos + value_len, <=, data_size + delta);

		codec.move(values_, pos + stored_value_len, pos + value_len, total_size - (pos + stored_value_len));

		codec.encode(values_, value, pos, data_size);

		reindex();

		meta->data_size() += delta;

		return 0;
	}


	Int setValue(Int block, Int idx, Value value)
	{
		Int value_idx = size() * block + idx;

		return setValue(value_idx, value);
	}



	void appendValue(Value value)
	{
		Int len = setValue(size(), value);
		if (len == 0)
		{
			size()++;
			reindex();
		}
		else {
			throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<size());
		}
	}


	FnAccessor<Value> value(Int idx)
	{
		return FnAccessor<Value>(
			[this, idx]() {return this->getValue(idx);},
			[this, idx](const Value& value) {
				if (this->setValue(idx, value) > 0)
				{
					throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<idx);
				}
			}
		);
	}

	FnAccessor<Value> value(Int block, Int idx)
	{
		return FnAccessor<Value>(
			[this, block, idx]() {return this->getValue(block, idx);},
			[this, block, idx](const Value& value) {
				if (this->setValue(block, idx, value) > 0)
				{
					throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<idx);
				}
			}
		);
	}


	ConstFnAccessor<Value> value(Int idx) const
	{
		return ConstFnAccessor<Value>(
			[this, idx]() {return this->getValue(idx);}
		);
	}

	ConstFnAccessor<Value> value(Int block, Int idx) const
	{
		return ConstFnAccessor<Value>(
				[this, block, idx]() {return this->getValue(block, idx);}
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
		Base::init(block_size, 4);

		Metadata* meta = Base::template allocate<Metadata>(0);

		Int max_size 		= FindTotalElementsNumber2(block_size, InitFn());

		meta->size() 		= 0;
		meta->data_size()	= 0;
		meta->index_size() 	= MyType::index_size(max_size);

		Base::template allocateArrayByLength<OffsetsType>(1, getOffsetsBlockLength(max_size));

		Int index_size = meta->index_size();
		Base::template allocateArrayBySize<IndexKey>(2, index_size * Indexes);

		Int values_block_length = Base::roundUpBitsToAlignmentBlocks(max_size * Codec::ElementSize);
		Base::template allocateArrayByLength<BufferType>(3, values_block_length);
	}


	// ================================= Allocation ======================================== //

	void enlarge(Int amount)
	{
		Int max_items_num 	= max_data_size() + amount;

		MEMORIA_ASSERT_TRUE(max_items_num >= 0);

		Int metadata_length	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
		Int offsets_length 	= Base::roundUpBytesToAlignmentBlocks(getOffsetsBlockLength(max_items_num));

		Int index_size 		= MyType::index_size(max_items_num);
		Int index_length	= Base::roundUpBytesToAlignmentBlocks(index_size * Indexes * sizeof(IndexKey));

		Int values_length	= Base::roundUpBitsToAlignmentBlocks(max_items_num * Codec::ElementSize);

		Int block_size		= Base::block_size(metadata_length + offsets_length + index_length + values_length, 4);

		if (amount > 0)
		{
			resize(block_size);

			resizeBlock(1, offsets_length);
			resizeBlock(2, index_length);
			resizeBlock(3, values_length);
		}
		else {
			resizeBlock(3, values_length);
			resizeBlock(2, index_length);
			resizeBlock(1, offsets_length);

			resize(block_size);
		}

		Metadata* meta 		= this->metadata();
		meta->index_size() 	= index_size;
	}

	void shrink(Int amount)
	{
		enlarge(-amount);
	}

	void insertSpace(Int, Int) {
		throw Exception(MA_SRC, "Method insertSpace(Int, Int) is not implemented for PackedVLETree");
	}

	Dimension insertSpace(Int idx, const Dimension& lengths)
	{
		MEMORIA_ASSERT_TRUE(idx >= 0);

		Int total 		= lengths.sum();
		Int size 		= this->size();
		Int max_size 	= this->max_data_size();
		Int max  		= this->data_size();

		Int capacity	= max_size - max;

		if (total > capacity)
		{
			enlarge(total - capacity);
		}

		Codec codec;

		Dimension starts;
		Dimension remainders;



		for (Int block = 0, inserted_length = 0; block < Blocks; block++, inserted_length += lengths[block])
		{
			Int offset			= block * size + idx;
			starts[block] 		= this->getValueOffset(offset) + inserted_length;
			remainders[block]	= max + inserted_length - starts[block];
		}

		auto* values = this->values();

		for (Int block = 0; block < Blocks; block++)
		{
			Int start 		= starts[block];
			Int end 		= starts[block] + lengths[block];
			Int remainder	= remainders[block];

			MEMORIA_ASSERT(remainder, >=, 0);

			codec.move(values, start, end, remainder);
		}

		this->data_size() += lengths.sum();

		return starts;
	}


	void removeSpace(Int start, Int end)
	{
		Int size = this->size();

		Codec codec;

		MEMORIA_ASSERT_TRUE(start >= 0);

		Int room_length = end - start;

		MEMORIA_ASSERT_TRUE(room_length >= 0);
		MEMORIA_ASSERT(room_length, <= , size - start);

		Int total = 0;
		Int ranges[3][Blocks];

		Int max = this->data_size();

		for (Int block = 0; block < Blocks; block++)
		{
			Int offset	= (Blocks - block - 1) * size + start;

			Int block_start		= ranges[0][block] = this->getValueOffset(offset);
			Int block_end		= ranges[1][block] = this->getValueOffset(offset + room_length);

			ranges[2][block] = max - block_end - total;

			total		+= block_end - block_start;
		}

		auto* values = this->values();

		for (Int block = 0; block < Blocks; block++)
		{
			Int block_start = ranges[0][block];
			Int block_end 	= ranges[1][block];
			Int remainder	= ranges[2][block];

			codec.move(values, block_end, block_start, remainder);
		}

		this->size() 		-= room_length;
		this->data_size()	-= total;

		shrink(total);

		reindex();
	}

	void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		throw Exception(MA_SRC, "copyTo() haven't not been fully implemented for this data structure yet");
	}

	template <typename TreeType>
	void transferDataTo(TreeType* other) const
	{
		Int size = this->size();

		Int total = this->getValueOffset(size * Blocks);
		other->enlarge(total);

		Codec codec;
		codec.copy(values(), 0, other->values(), 0, total);

		other->size() 		= size;
		other->data_size()	= this->data_size();

		other->reindex();
	}

	void splitTo(MyType* other, Int idx)
	{
		MEMORIA_ASSERT(other->size(), ==, 0);

		Int size = this->size();

		Dimension lengths;

		for (Int block = 0; block < Blocks; block++)
		{
			lengths[block] = this->data_length(block, idx, size);
		}

		Int total = lengths.sum();
		other->enlarge(total);

		Codec codec;
		for (Int block = 0; block < Blocks; block++)
		{
			Int pos = this->getValueOffset(size * block + idx);

			codec.copy(values(), pos, other->values(), 0, lengths[block]);
		}

		other->size() 		= size - idx;
		other->data_size()	= lengths.sum();

		other->reindex();

		removeSpace(idx, size);
	}

	void mergeWith(MyType* other)
	{
		Dimension lengths;

		Int my_size 	= this->size();
		Int other_size 	= other->size();

		for (Int block = 0; block < Blocks; block++)
		{
			lengths[block] = this->getValueOffset((block + 1) * my_size);
		}

		other->insertSpace(other->size(), lengths);

		Codec codec;

		for (Int block = 0; block < Blocks; block++)
		{
			Int my_start 	= this->getValueOffset(block * my_size);
			Int other_start = other->getValueOffset((block + 1) * other_size);

			codec.copy(values(), my_start, other->values(), other_start, lengths[block]);
		}

		other->size() += my_size;

		other->reindex();

		this->removeSpace(0, my_size);
	}

	void clear(Int, Int) {}


	// ==================================== IO ============================================= //



private:
	void insertData(const Values* values, Int pos, Int processed)
	{
		Codec codec;

		Dimension total_lengths;

		for (Int block = 0; block < Blocks; block++)
		{
			for (SizeT c = 0; c < processed; c++)
			{
				total_lengths[block] += codec.length(values[c][block]);
			}
		}

		auto insertion_starts = insertSpace(pos, total_lengths);

		auto* buffer = this->values();

		for (Int block = 0; block < Blocks; block++)
		{
			size_t start = insertion_starts[block];
			size_t limit = start + total_lengths[block];

			for (SizeT c = 0; c < processed; c++)
			{
				start += codec.encode(buffer, values[c][block], start, limit);
			}
		}

		this->size() 		+= processed;

		reindex();
	}

	void readData(Values* values, Int pos, Int processed)
	{
		Codec codec;

		const auto* buffer 	= this->values();
		Int size 			= this->size();

		for (Int block = 0; block < Blocks; block++)
		{
			Int value_idx = size * block + pos;

			size_t start = this->getValueOffset(value_idx);

			for (SizeT c = 0; c < processed; c++)
			{
				start += codec.decode(buffer, values[c][block], start);
			}
		}
	}

public:
	void insert(IData* data, Int pos, Int length)
	{
		MEMORIA_ASSERT(pos, <=, size());

		IDataSource<Values>* src = static_cast<IDataSource<Values>*>(data);

		IDataAPI api_type = src->api();

		Values values[IOBatchSize];
		BigInt to_write_local = length;

		if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
		{
			while (to_write_local > 0)
			{
				SizeT remainder 	= src->getRemainder();
				SizeT batch_size	= remainder > IOBatchSize ? IOBatchSize : remainder;

				SizeT processed 	= src->get(&values[0], 0, batch_size);

				insertData(values, pos, processed);

				pos 			+= processed;
				to_write_local 	-= processed;
			}
		}
		else {
			while (to_write_local > 0)
			{
				SizeT remainder 	= src->getRemainder();
				SizeT batch_size	= remainder > IOBatchSize ? IOBatchSize : remainder;

				for (Int c = 0; c < batch_size; c++)
				{
					values[c] = src->get();
				}

				insertData(values, pos, batch_size);

				pos 			+= batch_size;
				to_write_local 	-= batch_size;
			}
		}



		reindex();
	}

	void append(IData* src, Int pos, Int length)
	{
		insert(src, size(), length);
	}

	void update(IData* data, Int pos, Int length)
	{
		removeSpace(pos, pos + length);
		insert(data, pos, length);
	}

	void read(IData* data, Int pos, Int length)
	{
		IDataTarget<Values>* tgt = static_cast<IDataTarget<Values>*>(data);

		IDataAPI api_type = tgt->api();

		Values values[IOBatchSize];
		BigInt to_read = length;

		if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
		{
			while (to_read > 0)
			{
				SizeT remainder		= tgt->getRemainder();
				SizeT batch_size	= remainder > IOBatchSize ? IOBatchSize : remainder;

				readData(values, pos, batch_size);

				Int local_pos = 0;
				Int to_read_local = batch_size;

				while (to_read_local > 0)
				{
					SizeT processed = tgt->put(&values[0], local_pos, batch_size);

					local_pos 		+= processed;
					to_read_local 	-= processed;
				}

				pos 	+= batch_size;
				to_read	-= batch_size;
			}
		}
		else {
			while (to_read > 0)
			{
				SizeT remainder		= tgt->getRemainder();
				SizeT batch_size	= remainder > IOBatchSize ? IOBatchSize : remainder;

				readData(values, pos, batch_size);

				for (Int c = 0; c < batch_size; c++)
				{
					tgt->put(values[c]);
				}

				pos 	+= batch_size;
				to_read	-= batch_size;
			}
		}
	}

	// ==================================== Sum ============================================ //

    ValueDescr sum0(Int to) const
    {
    	GetVLEValuesSumFn<MyType> fn(*this, to);

    	Int pos = TreeTools::find_fw(fn);

    	return ValueDescr(fn.value(), pos, to);
    }

	IndexKey sum0(Int from, Int to) const
	{
		return sum0(to).value() - sum0(from).value();
	}


	IndexKey sum(Int block) const
	{
		return sum(block, size());
	}

	IndexKey sum(Int block, Int to) const
	{
		Int base = block * size();
		return sum0(base, base + to);
	}


	IndexKey sumWithoutLastElement(Int block) const
	{
		return sum(block, size() - 1);
	}

	IndexKey sum(Int block, Int from, Int to) const
	{
		Int base = block * size();
		return sum0(base + to).value() - sum0(base + from).value();
	}

	Values sum_values(Int from, Int to) const
	{
		Values vals;

		for (Int block = 0; block < Blocks; block++)
		{
			vals[block] = sum(block, from, to);
		}

		return vals;
	}

	Values sums() const
	{
		Values vals;

		if (index_size() > 0)
		{
			for (Int block = 0; block < Blocks; block++)
			{
				vals[block] = indexes(block)[0];
			}
		}
		else {
			for (Int block = 0; block < Blocks; block++)
			{
				vals[block] = sum(block);
			}
		}

		return vals;
	}


	// ==================================== Find =========================================== //

	template <template <typename, typename> class Comparator>
	ValueDescr find(Value value) const
	{
		const Metadata* meta = this->metadata();

		FindElementFn<MyType, Comparator> fn(
			*this,
			value,
			meta->index_size(),
			meta->size(),
			meta->data_size()
		);

		Int pos = TreeTools::find_fw(fn);

		Codec codec;
		Value actual_value;

		const auto* values_ = this->values();
		codec.decode(values_, actual_value, pos, meta->max_size());

		return ValueDescr(actual_value, pos, fn.position());
	}

	ValueDescr findLT(Value value) const {
		return this->template find<PackedCompareLE>(value);
	}

	ValueDescr findLE(Value value) const {
		return this->template find<PackedCompareLT>(value);
	}

	template <template <typename, typename> class Comparator>
	ValueDescr findForwardT(Int block, Int start, IndexKey val) const
	{
		const Metadata* meta = this->metadata();

		Int size 		= meta->size();
		Int block_start = block * size;

		auto prefix = sum0(block_start + start).value();

		FindElementFn<MyType, Comparator> fn(
				*this,
				val,
				meta->index_size(),
				meta->size(),
				meta->data_size()
		);

		Int pos = TreeTools::find_fw(fn);

		Int idx = fn.position();

		if (idx < block_start + size)
		{
			Codec codec;
			Value actual_value;

			const auto* values = this->values();
			codec.decode(values, actual_value, pos, meta->data_size());

			return ValueDescr(actual_value, pos, idx - block_start, fn.sum() - prefix);
		}
		else {
			return ValueDescr(0, pos, idx, sum(block) - prefix);
		}
	}

	ValueDescr findLTForward(Int block, Int start, IndexKey val) const
	{
		return this->template findForwardT<PackedCompareLE>(block, start, val);
	}

	ValueDescr findLEForward(Int block, Int start, IndexKey val) const
	{
		return this->template findForwardT<PackedCompareLT>(block, start, val);
	}


	template <template <typename, typename> class Comparator>
	ValueDescr findBackwardT(Int block, Int start, IndexKey val) const
	{
		const Metadata* meta = this->metadata();

		Int size 		= meta->size();
		Int block_start = block * size;

		auto prefix = sum0(block_start + start + 1).value();
		auto target = prefix - val;

		if (target >= 0)
		{
			FindElementFn<MyType, Comparator> fn(
					*this,
					val,
					meta->index_size(),
					meta->size(),
					meta->data_size()
			);

			Int pos = TreeTools::find_fw(fn);

			if (pos > block_start + start)
			{
				return ValueDescr(0, start, 0, 0);
			}
			else if (pos >= block_start)
			{
				Codec codec;
				Value actual_value;

				const auto* values = this->values();
				codec.decode(values, actual_value, pos, meta->data_size());


				return ValueDescr(actual_value, pos - block_start, fn.position(), prefix - (fn.sum() + actual_value));
			}
			else {
				return ValueDescr(0, -1, -1, prefix - sum0(block_start).value());
			}
		}
		else {
			return ValueDescr(0, -1, -1, prefix);
		}
	}

	ValueDescr findLTBackward(Int block, Int start, IndexKey val) const
	{
		return this->template findBackwardT<PackedCompareLE>(block, start, val);
	}

	ValueDescr findLEBackward(Int block, Int start, IndexKey val) const
	{
		return this->template findBackwardT<PackedCompareLT>(block, start, val);
	}

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

	// ==================================== Dump =========================================== //

	void dumpValues(Int size) const
	{
		auto* values = this->values();
		Int pos = 0;
		const Metadata* meta = this->metadata();
		Codec codec;

		dumpArray<Value>(cout, size, [&](Int) -> Value {
			Value value;
			pos += codec.decode(values, value, pos, meta->max_data_size());
			return value;
		});
	}

	void dumpData() const
	{
		auto* values = this->values();

		Int size = Base::element_size(3);

		dumpArray<BufferType>(cout, size, [&](Int idx) -> Value {
			return values[idx];
		});
	}

	void dump(std::ostream& out = cout) const
	{
		out<<"Layout: "<<endl;
		Base::dump(out);


		out<<"size_         = "<<size()<<endl;
		out<<"data_size_	= "<<data_size()<<endl;
		out<<"max_data_size_= "<<max_data_size()<<endl;
		out<<"index_size_   = "<<index_size()<<endl;
		out<<endl;

		out<<"Offsets:"<<endl;

		Int value_blocks = getValueBlocks(max_data_size());

		if (value_blocks > 1)
		{
			dumpSymbols<Value>(out, value_blocks, Codec::BitsPerOffset, [this](Int idx) {
				return this->offset(idx);
			});
		}

		out<<endl;

		Int idx_max = index_size();

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

		const BufferType* values = this->values();

		size_t pos = 0;

		Codec codec;

		const Metadata* meta = metadata();

		dumpArray<Value>(out, meta->size(), [&](Int) -> Value {
			Value value;
			pos += codec.decode(values, value, pos, meta->max_data_size());
			return value;
		});
	}


	// ================================= Serialization ============================= //


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		Base::generateDataEvents(handler);

		handler->startGroup("PACKED_TREE");

		const Metadata* meta = this->metadata();

		handler->value("SIZE",          &meta->size_);
		handler->value("DATA_SIZE",     &meta->data_size_);
		handler->value("INDEX_SIZE",    &meta->index_size_);

		Int max_size = this->max_data_size();

		handler->value("MAX_DATA_SIZE", &max_size);

		handler->startGroup("INDEXES", index_size());

		for (Int c = 0; c < index_size(); c++)
		{
			IndexKey indexes[Indexes];
			for (Int idx = 0; idx < Indexes; idx++)
			{
				indexes[idx] = this->indexes(idx)[c];
			}

			handler->value("INDEX", indexes, Indexes);
		}

		handler->endGroup();

		handler->startGroup("DATA", size());

		for (Int idx = 0; idx < meta->size() ; idx++)
		{
			Value values[Blocks];
			for (Int block = 0; block < Blocks; block++)
			{
				values[block] = this->getValue(block, idx);
			}

			handler->value("TREE_ITEM", values, Blocks);
		}

		handler->endGroup();

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		Base::serialize(buf);

		const Metadata* meta = this->metadata();

		FieldFactory<Int>::serialize(buf, meta->size());
		FieldFactory<Int>::serialize(buf, meta->data_size());
		FieldFactory<Int>::serialize(buf, meta->index_size());

		FieldFactory<OffsetsType>::serialize(buf, offsetsBlock(), offsetsBlockLength());

		FieldFactory<IndexKey>::serialize(buf, indexes(0), Indexes * meta->index_size());

		FieldFactory<BufferType>::serialize(buf, values(), meta->data_size());
	}

	void deserialize(DeserializationData& buf)
	{
		Base::deserialize(buf);

		Metadata* meta = this->metadata();

		FieldFactory<Int>::deserialize(buf, meta->size());
		FieldFactory<Int>::deserialize(buf, meta->data_size());
		FieldFactory<Int>::deserialize(buf, meta->index_size());

		FieldFactory<OffsetsType>::deserialize(buf, offsetsBlock(), offsetsBlockLength());

		FieldFactory<IndexKey>::deserialize(buf, indexes(0), Indexes * meta->index_size());

		FieldFactory<BufferType>::deserialize(buf, values(), meta->data_size());
	}


};


}


#endif
