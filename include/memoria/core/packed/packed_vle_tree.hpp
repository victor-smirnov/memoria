
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_TREE_HPP_

#include <memoria/core/packed/packed_tree_base.hpp>
#include <memoria/core/packed/tree_walkers.hpp>
#include <memoria/core/tools/exint_codec.hpp>

#include <memoria/core/tools/accessors.hpp>

namespace memoria {

struct EmptyResizeHandler {

};



template <
	typename K,
	typename IK,
	typename V,
	typename ResizeHandler_ = EmptyResizeHandler,
	Int Blocks_ = 1,
	Int BF = PackedTreeBranchingFactor
>
struct PackedVLETreeTypes {
    typedef K               Key;
    typedef IK              IndexKey;
    typedef V               Value;
    typedef ResizeHandler_  ResizeHandler;

    static const Int Blocks                 = Blocks_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = 256;

    template <typename T, typename VV>
    using Codec = ExintCodec<T, VV>;
};



template <typename Types>
class PackedVLETree: public PackedTreeBase<PackedVLETree<Types>, Types> {

	typedef PackedTreeBase<PackedVLETree<Types>, Types>							Base;

public:
	static const UInt VERSION               									= 1;


	typedef PackedVLETree<Types>               									MyType;

	typedef typename Types::ResizeHandler										ResizeHandler;

	typedef typename Types::Key													Key;
	typedef typename Types::IndexKey											IndexKey;
	typedef typename Types::Value												Value;

	typedef UBigInt																OffsetsType;

	typedef typename Types::template Codec<UByte, Value>						Codec;



	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks + 1;

	static const Int BITS_PER_OFFSET		= 4;
	static const Int ALIGNMENT_BLOCK		= 8; //Bytes

private:
	Int size_;
	Int index_size_;
	Int max_size_;
	Int block_size_;

	UByte buffer_[];

public:
	PackedVLETree() {}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	const Int& index_size() const {return index_size_;}

	const Int& max_size() const {return max_size_;}
	const Int& block_size() const {return block_size_;}

	OffsetsType* offsetsBlock() {
		return T2T<OffsetsType*> (buffer_);
	}

	const OffsetsType* offsetsBlock() const {
		return T2T<OffsetsType*> (buffer_);
	}

	IndexKey* indexes(Int index_block)
	{
		return T2T<IndexKey*>(buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * (index_block + 1));
	}

	const IndexKey* indexes(Int index_block) const
	{
		return T2T<const IndexKey*>(buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * (index_block + 1));
	}

	IndexKey* sizes()
	{
		return T2T<IndexKey*>(buffer_ + getOffsetsLengts());
	}

	const IndexKey* sizes() const
	{
		return T2T<const IndexKey*>(buffer_ + getOffsetsLengts());
	}

	UByte* getValues()
	{
		return buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * Indexes;
	}

	const UByte* getValues() const
	{
		return buffer_ + getOffsetsLengts() + index_size_ * sizeof(IndexKey) * Indexes;
	}

	static Int roundBitsToBytes(Int bitsize)
	{
		return (bitsize / 8 + (bitsize % 8 ? 1 : 0));
	}

	static Int roundBytesToAlignmentBlocks(Int value)
	{
		return (value / ALIGNMENT_BLOCK + (value % ALIGNMENT_BLOCK ? 1 : 0)) * ALIGNMENT_BLOCK;
	}

	static Int roundBitsToAlignmentBlocks(Int bitsize)
	{
		Int byte_size = roundBitsToBytes(bitsize);
		return roundBytesToAlignmentBlocks(byte_size);
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
		Int value_blocks = getValueBlocks(items_num);

		Int offsets_bits = value_blocks * BITS_PER_OFFSET;

		Int offsets_bytes = roundBitsToAlignmentBlocks(offsets_bits);

		return offsets_bytes;
	}

	static Int getBlockSize(Int items_num)
	{
		Int offsets_length = getOffsetsLengts(items_num);

		Int index_size = getIndexSize(items_num);

		return offsets_length + index_size * Indexes * sizeof(IndexKey) + items_num;
	}

	static Int getIndexSize(Int items_number)
	{
		return MyType::template compute_index_size<MyType>(items_number);
	}

	// ================================= Reindexing ======================================== //

private:

	class ReindexFn {
	public:
		static const Int BranchingFactor        = Types::BranchingFactor;
		static const Int ValuesPerBranch        = Types::ValuesPerBranch;

	private:
		MyType& me_;

		const UByte* values_;

		IndexKey* indexes_[Indexes];

	public:
		ReindexFn(MyType& me): me_(me)
		{
			values_ = me.getValues();

			indexes_[0] = me.sizes();
			indexes_[1] = me.indexes(0);
		}

		Int size() const {
			return me_.size();
		}

		Int maxSize() const {
			return me_.max_size();
		}

		Int indexSize() const {
			return me_.index_size();
		}

		void clearIndex(Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				indexes_[0][c] = 0;
				indexes_[1][c] = 0;
			}
		}

		void buildFirstIndexLine(Int index_level_start, Int index_level_size)
		{
			Codec codec;

			size_t pos = 0;
			size_t limit = ValuesPerBranch;
			Int value_block = 0;

			me_.offset(0) = 0;

			for (Int c = 0; c < me_.size(); c++)
			{
				Value value;
				size_t len = codec.decode(values_, value, pos);

				Int idx = index_level_start + value_block;

				indexes_[0][idx] += 1;
				indexes_[1][idx] += value;

				pos += len;

				if (pos >= limit)
				{
					value_block++;

					if (c + 1 < me_.max_size())
					{
						me_.offset(value_block) = pos - limit;
					}

					limit += ValuesPerBranch;
				}
			}
		}

		void processIndex(Int parent, Int start, Int end)
		{
			for (Int b = 0; b < 2; b++)
			{
				IndexKey sum = 0;

				for (Int c = start; c < end; c++)
				{
					sum += indexes_[b][c];
				}

				indexes_[b][parent] = sum;
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

	class GetValuesSumFn: public GetValueOffsetFnBase<MyType, GetValuesSumFn> {
		typedef GetValueOffsetFnBase<MyType, GetValuesSumFn> Base;

		const IndexKey* indexes_;

		Value value_ = 0;

	public:
		GetValuesSumFn(const MyType& me, Int limit): Base(me, limit)
		{
			indexes_ = me.indexes(0);
		}

		void processIndexes(Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				value_ += indexes_[c];
			}
		}

		void processValue(Value value)
		{
			value_ += value;
		}

		Value value() const {
			return value_;
		}
	};




	template <template <typename, typename> class Comparator>
	class FindElementFn: public FindForwardFnBase<MyType, FindElementFn<Comparator>, IndexKey, Comparator> {

		typedef FindForwardFnBase<MyType, FindElementFn<Comparator>, IndexKey, Comparator> 	Base;

	public:
		static const Int BranchingFactor        = Types::BranchingFactor;
		static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	private:

		const MyType& 		me_;

		const UByte* 		values_;
		const IndexKey* 	sizes_;

		Int position_;

	public:
		FindElementFn(const MyType& me, BigInt limit): Base(me.indexes(0), limit), me_(me), position_(0)
		{
			values_  = me.getValues();
			sizes_   = me.sizes();
		}

		Int max_size() const {
			return me_.max_size();
		}

		Int index_size() const {
			return me_.index_size();
		}

		Int size() const {
			return me_.max_size();
		}

		Int position() const {
			return position_;
		}

		Int walkFirstValuesBlock(Int start, Int end)
		{
			return walkValues(start, end);
		}

		Int walkLastValuesBlock(Int value_block_num)
		{
			Int offset = me_.offset(value_block_num);

			Int pos = value_block_num * ValuesPerBranch + offset;
			Int end = me_.max_size();

			return walkValues(pos, end);
		}

		Int walkValues(Int pos, Int end)
		{
			Comparator<Int, Key> compare;
			Codec codec;

			while (pos < end)
			{
				Value value;

				Int len = codec.decode(values_, value, pos);

				if (compare(value, Base::limit_))
				{
					Base::sum_ 	 += value;
					Base::limit_ -= value;

					pos += len;

					position_ ++;
				}
				else {
					return pos;
				}
			}

			return end;
		}

		void processIndexes(Int start, Int end)
		{
			for (Int c = start; c < end; c++)
			{
				position_ += sizes_[c];
			}
		}
	};




public:

	Int getValueOffset(Int idx) const
	{
		GetValueOffsetFn fn(*this, idx);

		Int pos = Base::find_fw(fn);

		return pos;
	}

	Int getValuesSize() const
	{
		if (size() > 0)
		{
			const UByte* values_ = getValues();

			Int pos = getValueOffset(size() - 1);

			return pos + Codec().length(values_, pos);
		}
		else {
			return 0;
		}
	}

	Value getValue(Int idx) const
	{
		const UByte* values_ = getValues();

		Int pos = getValueOffset(idx);

		Codec codec;
		Value value;

		codec.decode(values_, value, pos);
		return value;
	}

	void setValue(Int idx, Value value)
	{
		UByte* values_ = getValues();

		Int pos = getValueOffset(idx);
		Int remainder = getValuesSize();

		Codec codec;

		size_t value_len = codec.length(value);
		size_t stored_value_len = codec.length(values_, pos);

		CopyBuffer(values_ + stored_value_len, values_ + value_len, remainder);

		codec.encode(values_, value, pos);
	}

	class ValueDescr {
		Value value_;
		Int pos_;
		Int idx_;
	public:
		ValueDescr(BigInt value, Int pos, Int idx): value_(value), pos_(pos), idx_(idx) {}

		Value value() const 	{return value_;}
		Int pos() const 		{return pos_;}
		Int idx() const 		{return idx_;}
	};

	ValueDescr findLE1(Int start_idx, Value value) const
	{
		FindElementFn<BTreeCompareLT> fn(*this, value);

		Int pos;

		if (start_idx > 0)
		{
			Int start = getValueOffset(start_idx);
			pos = Base::find_fw(start, fn);
		}
		else {
			pos = Base::find_fw(fn);
		}

		Codec codec;
		Value actual_value;

		const UByte* values_ = getValues();
		codec.decode(values_, actual_value, pos);

		return ValueDescr(actual_value + fn.sum(), pos, start_idx + fn.position());
	}

	ValueDescr findLE(Value value) const
	{
		FindElementFn<BTreeCompareLT> fn(*this, value);

		Int pos = Base::find_fw(fn);

		Codec codec;
		Value actual_value;

		const UByte* values_ = getValues();
		codec.decode(values_, actual_value, pos);

		return ValueDescr(actual_value + fn.sum(), pos, fn.position());
	}

	ValueDescr findLE(Int start_idx, Value value) const
	{
		ValueDescr prefix = sum(start_idx);

		ValueDescr descr = findLE(value + prefix.value());

		return ValueDescr(descr.value() - prefix.value(), descr.pos(), descr.idx());
	}

	ValueDescr sum(Int to) const
	{
		GetValuesSumFn fn(*this, to);

		Int pos = Base::find_fw(fn);

		return ValueDescr(fn.value(), pos, to);
	}

	ValueDescr sum(Int from, Int to) const
	{
		ValueDescr prefix = sum(from);
		ValueDescr total = sum(to);

		return ValueDescr(total.value() - prefix.value(), total.pos(), to);
	}


	FnAccessor<Value> value(Int idx)
	{
		return FnAccessor<Value>(
			[this, idx](){return this->getValue(idx);},
			[this, idx](const Value& value){this->setValue(idx, value);}
		);
	}

	ConstFnAccessor<Value> value(Int idx) const
	{
		return FnAccessor<Value>(
			[this, idx](){return this->getValue(idx);}
		);
	}

	// =================================== Offsets ======================================== //

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
	class InitByBlockFn {
		const MyType& tree_;
	public:

		InitByBlockFn(const MyType& tree): tree_(tree) {}

		Int last(Int block_size) const {
			return block_size;
		}

		Int getBlockSize(Int items_number) const
		{
			return MyType::getBlockSize(items_number);
		}

		Int extend(Int items_number) const {
			return items_number;
		}

		Int getIndexSize(Int items_number) const {
			return MyType::template compute_index_size<MyType>(items_number);
		}
	};

public:
	void initByBlock(Int block_size)
	{
		size_ = 0;

		max_size_   = MyType::getMaxSize(block_size, InitByBlockFn(*this));
		index_size_ = getIndexSize(max_size_);

		block_size_ = block_size;
	}


	void initSizes(Int max)
	{
		size_       = 0;
		max_size_   = max;
		index_size_ = getIndexSize(max_size_);
	}

	// ==================================== Dump =========================================== //


	void dump(std::ostream& out = cout) const
	{
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;
		out<<"index_size_ = "<<index_size_<<endl;
		out<<"block_size_ = "<<block_size_<<endl;
		out<<endl;

		out<<"Offsets:"<<endl;

		Int value_blocks = getValueBlocks(max_size_);

		Base::template dumpSymbols<Value>(out, value_blocks, 5, [&](Int idx) -> Value {
			return this->offset(idx);
		});

		out<<endl;

		Int idx_max = index_size_; //getIndexSize(size_);

		const IndexKey* sizes 	= this->sizes();
		const IndexKey* indexes = this->indexes(0);

		out<<"Indexes:"<<endl;

		for (Int c = 0; c < idx_max; c++) {
			out<<c<<" "<<sizes[c]<<" "<<indexes[c]<<endl;
		}

		out<<endl;

		out<<"Data:"<<endl;

		const UByte* values_ = this->getValues();
		size_t pos = 0;

		Codec codec;

		Base::template dumpArray<Value>(out, size_, [&](Int) -> Value {
			Value value;
			pos += codec.decode(values_, value, pos);
			return value;
		});
	}

private:
};


}


#endif
