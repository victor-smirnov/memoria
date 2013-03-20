
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
public:
	VLETreeValueDescr(BigInt value, Int pos, Int idx): value_(value), pos_(pos), idx_(idx) {}

	Value value() const 	{return value_;}
	Int pos() const 		{return pos_;}
	Int idx() const 		{return idx_;}
};


template <typename Types_>
class PackedVLETree: public PackedAllocator
{
	typedef PackedAllocator														Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedVLETree<Types>               								MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	typedef UBigInt																OffsetsType;

	typedef typename Types::template Codec<Value>								Codec;
	typedef typename Codec::BufferType											BufferType;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;

	static const Int BITS_PER_OFFSET		= Codec::BitsPerOffset;

	typedef PackedTreeTools<IndexKey, BranchingFactor, ValuesPerBranch>			TreeTools;

	class Metadata {
		Int size_;
		Int max_size_;
		Int index_size_;
	public:
		Metadata() {}

		const Int& size() const {return size_;};
		Int max_size() const 	{return max_size_;};
		Int index_size() const 	{return index_size_;};

		Int& size() 			{return size_;};
		Int& max_size() 		{return max_size_;};
		Int& index_size() 		{return index_size_;};
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

	const Int index_size() const {return metadata()->index_size();}

	const Int max_size() const {return metadata()->max_size();}

	OffsetsType* offsetsBlock() {
		return Base::template get<OffsetsType>(1);
	}

	const OffsetsType* offsetsBlock() const {
		return Base::template get<OffsetsType>(1);
	}

	IndexKey* indexes(Int index_block)
	{
		//if (!Base::is_empty(2)) {
			return Base::template get<IndexKey>(2) + index_block * index_size();
//		}
//		else {
//			return nullptr;
//		}
	}

	IndexKey* sizes() {
		return indexes(0);
	}

	const IndexKey* indexes(Int index_block) const
	{
//		if (!Base::is_empty(2)) {
			return Base::template get<IndexKey>(2) + index_block * index_size();
//		}
//		else {
//			return nullptr;
//		}
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
		return getValueBlocks(max_size());
	}

	Int getOffsetsLengts() const
	{
		return getOffsetsBlockLength(max_size());
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

			Int max_size = Base::me_.max_size();

			for (Int c = 0; c < Base::me_.size(); c++)
			{
				Value value;
				Int len = codec.decode(values_, value, pos, max_size);

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
		TreeTools::reindex(0, size(), fn);
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

		Int pos = TreeTools::find_fw(fn);

		return pos;
	}

	Int data_size() const
	{
		if (size() > 0)
		{
			Int pos = getValueOffset(size() - 1);

			const BufferType* values_ = values();
			Codec codec;

			return pos + codec.length(values_, pos, max_size());
		}
		else {
			return 0;
		}
	}

	Value getValue(Int idx) const
	{
		const auto* values_ = values();

		Int pos = getValueOffset(idx);

		Codec codec;
		Value value;

		codec.decode(values_, value, pos, max_size());
		return value;
	}

	Int setValue(Int idx, Value value)
	{
		auto* values_ = values();

		Int pos = getValueOffset(idx);

		MEMORIA_ASSERT(pos, <, max_size());

		Int total_size = data_size();

		Codec codec;

		Int value_len = codec.length(value);

		MEMORIA_ASSERT(pos + value_len, <, max_size());

		Int stored_value_len = codec.length(values_, pos, max_size());
		Int delta 	 		 = value_len - stored_value_len;

		if (delta > 0)
		{
			Int capacity = max_size() - total_size;

			if (capacity < delta)
			{
				enlarge(delta);
			}
		}

		codec.move(values_, pos + stored_value_len, pos + value_len, total_size - (pos + stored_value_len));

		codec.encode(values_, value, pos, max_size());

		reindex();

		return 0;
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
			throw Exception(MA_SRC, SBuf()<<"Out of block memory for value at "<<size());
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
		meta->max_size() 	= max_size;
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

	}

	void shrink(Int amount)
	{

	}

	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout) const
	{
		out<<"Layout: "<<endl;
		Base::dump(out);


		out<<"size_       = "<<size()<<endl;
		out<<"max_size_   = "<<max_size()<<endl;
		out<<"index_size_ = "<<index_size()<<endl;
		out<<endl;

		out<<"Offsets:"<<endl;

		Int value_blocks = getValueBlocks(max_size());

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
			pos += codec.decode(values, value, pos, meta->max_size());
			return value;
		});
	}

private:
};


}


#endif
