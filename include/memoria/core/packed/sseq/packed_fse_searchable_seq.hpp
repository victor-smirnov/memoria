
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_SEARCHABLESEQ_HPP_
#define MEMORIA_CORE_PACKED_FSE_SEARCHABLESEQ_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>
#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>


#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_rank_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_reindex_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_select_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <ostream>


namespace memoria {


template <
	Int BitsPerSymbol_		= 1,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= 512,

	template <typename>	class IndexType 	= PkdFTree,
	template <typename>	class CodecType 	= ValueFSECodec,
	template <typename>	class ReindexFnType = BitmapReindexFn,
	template <typename>	class SelectFnType	= BitmapSelectFn,
	template <typename>	class RankFnType	= BitmapRankFn,
	template <typename>	class ToolsFnType	= BitmapToolsFn
>
struct PkdFSSeqTypes {

    static const Int Blocks                 = 1 << BitsPerSymbol_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;
    static const Int BitsPerSymbol			= BitsPerSymbol_;

    template <typename T>
    using Index 	= IndexType<T>;

    template <typename V>
    using Codec 	= CodecType<V>;

    template <typename Seq>
    using ReindexFn	= ReindexFnType<Seq>;

    template <typename Seq>
    using SelectFn	= SelectFnType<Seq>;

    template <typename Seq>
    using RankFn	= RankFnType<Seq>;

    template <typename Seq>
    using ToolsFn	= ToolsFnType<Seq>;
};


template <typename Types_>
class PkdFSSeq: public PackedAllocator {

	typedef PackedAllocator														Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PkdFSSeq<Types_>               										MyType;

	typedef PackedAllocator														Allocator;

	typedef Int 																IndexValue;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;
	static const Int BitsPerSymbol			= Types::BitsPerSymbol;

	enum {
		METADATA, INDEX, SYMBOLS
	};

	typedef typename IfThenElse<
			BitsPerSymbol == 8,
			UByte,
			UBigInt
	>::Result																	Value;

	typedef Packed2TreeTypes<
				IndexValue,
				IndexValue,
				1<<BitsPerSymbol,
				Types::template Codec,
				BranchingFactor,
				512
	>																			IndexTypes;

	typedef typename Types::template Index<IndexTypes>							Index;
	typedef typename Index::Codec												Codec;

	static const Int IndexSizeThreshold											= 0;

	typedef core::StaticVector<BigInt, Indexes + 1> 							Values;

	typedef typename Types::template ToolsFn<MyType>							Tools;


	class Metadata {
		Int size_;
	public:
		Int& size() 				{return size_;}
		const Int& size() const 	{return size_;}
	};

public:
	PkdFSSeq() {}

	Int& size() {return metadata()->size();}
	const Int& size() const {return metadata()->size();}

	Int max_size() const
	{
		Int values_length = Base::element_size(SYMBOLS);

		Int symbols = values_length * 8 / BitsPerSymbol;

		return symbols;
	}

	Int capacity() const
	{
		return max_size() - size();
	}

	// ====================================== Accessors ================================= //

	Metadata* metadata()
	{
		return Base::template get<Metadata>(METADATA);
	}

	const Metadata* metadata() const
	{
		return Base::template get<Metadata>(METADATA);
	}

	Index* index()
	{
		return Base::template get<Index>(INDEX);
	}

	const Index* index() const
	{
		return Base::template get<Index>(INDEX);
	}

	bool has_index() const {
		return Base::element_size(INDEX) > 0;
	}

	Value* symbols()
	{
		return Base::template get<Value>(SYMBOLS);
	}

	const Value* symbols() const
	{
		return Base::template get<Value>(SYMBOLS);
	}

	class SymbolAccessor {
		MyType& seq_;
		Int idx_;
	public:
		SymbolAccessor(MyType& seq, Int idx): seq_(seq), idx_(idx) {}

		Value operator=(Value val)
		{
			seq_.set(idx_, val);
			return val;
		}

		operator Value() const {
			return seq_.get(idx_);
		}

		Value value() const {
			return seq_.get(idx_);
		}
	};

	SymbolAccessor symbol(Int idx)
	{
		return SymbolAccessor(*this, idx);
	}

	class ConstSymbolAccessor {
		const MyType& seq_;
		Int idx_;
	public:
		ConstSymbolAccessor(const MyType& seq, Int idx): seq_(seq), idx_(idx) {}

		operator Value() const {
			return seq_.get(idx_);
		}

		Value value() const {
			return seq_.get(idx_);
		}
	};

	ConstSymbolAccessor symbol(Int idx) const
	{
		return ConstSymbolAccessor(*this, idx);
	}


public:

	// ===================================== Allocation ================================= //

	void init(Int block_size)
	{
		MEMORIA_ASSERT(block_size, >=, empty_size());

		init();
	}

	void init()
	{
		Base::init(empty_size(), 3);

		Metadata* meta 	= Base::template allocate<Metadata>(METADATA);

		meta->size() 	= 0;

		Base::setBlockType(INDEX, 	PackedBlockType::ALLOCATABLE);
		Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

		// other sections are empty at this moment
	}

	Int block_size() const {
		return Base::block_size();
	}


	static Int empty_size()
	{
		Int metadata_length	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
		Int index_length 	= 0;
		Int values_length 	= 0;
		Int block_size 		= Base::block_size(metadata_length + index_length + values_length, 3);
		return block_size;
	}

	static Int estimate_block_size(Int size, Int density_hi = 1, Int density_lo = 1)
	{
		Int symbols_block_size 	= Base::roundUpBitsToAlignmentBlocks(size * BitsPerSymbol);
		Int index_size 			= divUp(size , ValuesPerBranch);
		Int index_block_size	= Index::estimate_block_size(index_size, density_hi, density_lo);
		Int metadata_block_size = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

		Int client_area 		= metadata_block_size + index_block_size + symbols_block_size;
		Int block_size 			= Base::block_size(client_area, 3);

		return block_size;
	}

	void removeIndex()
	{
		Base::free(INDEX);
	}

	void createIndex(Int index_size)
	{
		Int index_block_size = Index::block_size(index_size);
		Base::resizeBlock(INDEX, index_block_size);

		Index* index = this->index();
		index->init(index_block_size);
		index->setAllocatorOffset(this);
	}

	std::pair<Int, Int> density() const {
		return std::pair<Int, Int>(1,1);
	}


	// ========================================= Update ================================= //

	void reindex()
	{
		typename Types::template ReindexFn<MyType> reindex_fn;
		reindex_fn(*this);
	}

	void check() const
	{
		if (has_index()) {
			index()->check();
		}
	}

	void set(Int idx, Int symbol)
	{
		tools().set(symbols(), idx, symbol);
	}

	void clear()
	{
		Base::resizeBlock(SYMBOLS, 0);
		removeIndex();

		size() = 0;

		Base::pack();
	}

protected:

	void enlargeData(Int length)
	{
		Int capacity = this->capacity();

		if (length >= capacity)
		{
			Int new_size 		= size() + length;
			Int new_block_size 	= roundUpBitToBytes(new_size * BitsPerSymbol);
			Base::resizeBlock(SYMBOLS, new_block_size);
		}
	}

	void insertDataRoom(Int pos, Int length)
	{
		enlargeData(length);

		auto symbols = this->symbols();

		Int rest = size() - pos;

		tools().move(symbols, pos, (pos + length), rest);

		size() += length;
	}

	void shrinkData(Int length)
	{
		Int new_size 		= size() - length;

		if (new_size >= 0)
		{
			Int new_block_size 	= roundUpBitToBytes(new_size * BitsPerSymbol);

			Base::resizeBlock(SYMBOLS, new_block_size);
		}
	}

public:



	void insert(Int pos, Int symbol)
	{
		insertDataRoom(pos, 1);

		Value* symbols = this->symbols();

		tools().set(symbols, pos, symbol);

		reindex();
	}

	void remove(Int start, Int end)
	{
		Int& size = this->size();

		MEMORIA_ASSERT(start, >=, 0);
		MEMORIA_ASSERT(end, >=, 0);
		MEMORIA_ASSERT(start, <=, end);

		MEMORIA_ASSERT(end, <=, size);

		auto symbols = this->symbols();

		Int rest = size - end;

		tools().move(symbols, end, start, rest);

		shrinkData(end - start);

		size -= (end - start);

		reindex();

		Base::pack();
	}

	void removeSymbol(Int idx) {
		remove(idx, idx + 1);
	}



	// ====================================== Batch IO ================================= //

	void read(IData* data, Int pos, Int length) const
	{
		IDataTarget<Value>* tgt = static_cast<IDataTarget<Value>*>(data);

		IDataAPI api_type = tgt->api();

		auto symbols = this->symbols();

		MEMORIA_ASSERT_TRUE(api_type == IDataAPI::Batch || api_type == IDataAPI::Both);

		SizeT remainder 	= tgt->getRemainder();
		SizeT to_read_local = length <= remainder ? length : remainder;

		while (to_read_local > 0)
		{
			SizeT processed = tgt->put(symbols, pos, to_read_local);

			pos 			+= processed;
			to_read_local 	-= processed;
		}
	}

	void assertAligns1() const
	{
		MEMORIA_ASSERT(T2T<size_t>(metadata()) % 8, ==, 0);

		if (has_index()) {
			MEMORIA_ASSERT(T2T<size_t>(index()) % 8, ==, 0);
		}

		MEMORIA_ASSERT(T2T<size_t>(symbols()) % 8, ==, 0);
	}

	void assertAligns2() const
	{
		MEMORIA_ASSERT(T2T<size_t>(metadata()) % 8, ==, 0);

		if (has_index()) {
			MEMORIA_ASSERT(T2T<size_t>(index()) % 8, ==, 0);
		}

		MEMORIA_ASSERT(T2T<size_t>(symbols()) % 8, ==, 0);
	}


	void assertAligns3() const
	{
		MEMORIA_ASSERT(T2T<size_t>(metadata()) % 8, ==, 0);

		if (has_index()) {
			MEMORIA_ASSERT(T2T<size_t>(index()) % 8, ==, 0);
		}

		MEMORIA_ASSERT(T2T<size_t>(symbols()) % 8, ==, 0);
	}

	void insert(IData* data, Int pos, Int length)
	{
		IDataSource<Value>* tgt = static_cast<IDataSource<Value>*>(data);

		IDataAPI api_type = tgt->api();

		auto symbols = this->symbols();

		MEMORIA_ASSERT_TRUE(api_type == IDataAPI::Batch || api_type == IDataAPI::Both);

		SizeT remainder 		= tgt->getRemainder();
		SizeT to_process_local 	= length <= remainder ? length : remainder;

		insertDataRoom(pos, to_process_local);

		while (to_process_local > 0)
		{
			SizeT processed = tgt->get(symbols, pos, to_process_local);

			pos 				+= processed;
			to_process_local 	-= processed;
		}

		reindex();
	}

	void fill(Int start, Int end, std::function<Value ()> fn)
	{
		auto symbols = this->symbols();
		auto tools = this->tools();

		for (Int c = start; c < end; c++)
		{
			Value val = fn();
			tools.set(symbols, c, val);
		}
	}


	void insert(Int start, Int length, std::function<Value ()> fn)
	{
		insertDataRoom(start, length);
		fill(start, start + length, fn);
		reindex();
	}

	void append(IData* data, Int length)
	{
		insert(data, size(), length);
	}

	void update(IData* data, Int pos, Int length)
	{
		IDataSource<Value>* tgt = static_cast<IDataSource<Value>*>(data);

		IDataAPI api_type = tgt->api();

		auto symbols = this->symbols();

		MEMORIA_ASSERT_TRUE(api_type == IDataAPI::Batch || api_type == IDataAPI::Both);

		SizeT remainder 		= tgt->getRemainder();
		SizeT to_process_local 	= length <= remainder ? length : remainder;

		while (to_process_local > 0)
		{
			SizeT processed = tgt->get(symbols, pos, to_process_local);

			pos 				+= processed;
			to_process_local 	-= processed;
		}

		reindex();
	}

	// ========================================= Node ================================== //

	template <typename TreeType>
	void transferDataTo(TreeType* other) const
	{
		other->enlargeData(this->size());

		Int data_size = this->element_size(SYMBOLS);

		CopyByteBuffer(symbols(), other->symbols(), data_size);

		other->reindex();
	}

	void splitTo(MyType* other, Int idx)
	{
		Int to_move 	= this->size() - idx;
		Int other_size 	= other->size();

		other->enlargeData(to_move);

		auto tools = this->tools();

		tools.move(other->symbols(), other->symbols(), 0, to_move, other_size);

		tools.move(this->symbols(), other->symbols(), idx, 0, to_move);

		other->size() += to_move;
		other->reindex();

		remove(idx, this->size());
	}

	void mergeWith(MyType* other) const
	{
		Int my_size 	= this->size();
		Int other_size 	= other->size();

		other->enlargeData(my_size);

		tools().move(this->symbols(), other->symbols(), 0, other_size, my_size);

		other->size() += my_size;

		other->reindex();
	}


	// ========================================= Query ================================= //

	Values sums() const
	{
		if (has_index())
		{
			auto index = this->index();
			auto isums = index->sums();

			Values vsums;

			vsums.assignUp(isums);
			vsums[0] = size();

			return vsums;
		}
		else {
			return sums(size());
		}
	}

	Values sums(Int to) const
	{
		if (has_index())
		{
			auto index = this->index();

			Int index_block = to / ValuesPerBranch;

			auto isums = index->sums(0, index_block);

			auto vsums = tools().sum(index_block * ValuesPerBranch, to);

			vsums.sumUp(isums);
			vsums[0] += index_block * ValuesPerBranch;

			return vsums;
		}
		else
		{
			auto vsums = tools().sum(0, to);

			return vsums;
		}
	}

	Values ranks(Int to) const
	{
		Values vals;
		vals[0] = to;

		for (Int symbol = 0; symbol < Indexes; symbol++)
		{
			vals[symbol + 1] = rank(to, symbol);
		}

		return vals;
	}

	Values sums(Int from, Int to) const
	{
		return sums(to) - sums(from);
	}

	Values sum(Int from, Int to) const {
		return sums(from, to);
	}

	Int get(Int idx) const
	{
		return tools().get(symbols(), idx);
	}

	bool test(Int idx, Value symbol) const
	{
		return tools().test(symbols(), idx, symbol);
	}

	Int rank(Int symbol) const
	{
		if (has_index())
		{
			const Index* index = this->index();
			return index->sum(symbol);
		}
		else {
			return rank(size(), symbol);
		}
	}

	Int rank(Int start, Int end, Int symbol) const
	{
		Int rank_start 	= rank(start, symbol);
		Int rank_end 	= rank(end, symbol);

		return rank_end - rank_start;
	}

	Int rank(Int end, Int symbol) const
	{
		if (has_index())
		{
			const Index* index = this->index();

			Int values_block	= (end / ValuesPerBranch);
			Int start 			= values_block * ValuesPerBranch;

			Int sum = index->sum(symbol, values_block);

			typename Types::template RankFn<MyType> fn(*this);

			Int block_sum = fn(start, end, symbol);

			return sum + block_sum;
		}
		else {
			typename Types::template RankFn<MyType> fn(*this);

			return fn(0, end, symbol);
		}
	}


	SelectResult selectFw(Int start, Int symbol, BigInt rank) const
	{
		Int start_rank = this->rank(start, symbol);
		auto result = selectFw(symbol, start_rank + rank);

		result.rank() -= start_rank;

		return result;
	}

	SelectResult selectFw(Int symbol, BigInt rank) const
	{
		if (has_index())
		{
			const Index* index = this->index();

			Int index_size = index->size();

			auto result = index->findLEForward(symbol, 0, rank);

			if (result.idx() < index_size)
			{
				Int start = result.idx() * ValuesPerBranch;

				typename Types::template SelectFn<MyType> fn(*this);

				Int local_rank = rank - result.prefix();

				Int size = this->size();

				return fn(start, size, symbol, local_rank);
			}
			else {
				return SelectResult(result.idx(), result.prefix(), false);
			}
		}
		else {
			typename Types::template SelectFn<MyType> fn(*this);
			return fn(0, size(), symbol, rank);
		}
	}

	SelectResult selectBw(Int start, Int symbol, BigInt rank) const
	{
		Int local_rank = this->rank(start, symbol);

		if (local_rank >= rank)
		{
			return selectFw(symbol, local_rank - rank + 1);
		}
		else {
			return SelectResult(-1,local_rank,false);
		}
	}

	SelectResult selectBw(Int symbol, BigInt rank) const
	{
		return selectBw(size(), symbol, rank);
	}

	void dump(std::ostream& out = cout, bool dump_index = true) const
	{
		out<<"size       = "<<size()<<endl;
		out<<"max_size   = "<<max_size()<<endl;

		if (dump_index)
		{
			out<<"Index: ";
			if (has_index())
			{
				out<<endl;
				out<<"========================================"<<endl;
				this->index()->dump(out);
				out<<"========================================"<<endl;
			}
			else {
				out<<"None"<<endl;
			}
		}

		out<<endl;

		out<<"Data:"<<endl;

		dumpSymbols<Value>(out, size(), BitsPerSymbol, [this](Int idx) -> Value {
			return this->get(idx);
		});
	}


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		Base::generateDataEvents(handler);

		handler->startGroup("PACKED_SEQUENCE");

		handler->value("SIZE",          &size());

		Int max_size = this->max_size();
		handler->value("MAX_SIZE",      &max_size);

		if (has_index())
		{
			index()->generateDataEvents(handler);
		}

		handler->startGroup("DATA", size());

		handler->symbols("SYMBOLS", symbols(), size(), BitsPerSymbol);

		handler->endGroup();

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		Base::serialize(buf);

		const Metadata* meta = this->metadata();

		FieldFactory<Int>::serialize(buf, meta->size());

		if (has_index()) {
			index()->serialize(buf);
		}

		FieldFactory<Value>::serialize(buf, symbols(), symbol_buffer_size());
	}

	void deserialize(DeserializationData& buf)
	{
		Base::deserialize(buf);

		Metadata* meta = this->metadata();

		FieldFactory<Int>::deserialize(buf, meta->size());

		if (has_index()) {
			index()->deserialize(buf);
		}

		FieldFactory<Value>::deserialize(buf, symbols(), symbol_buffer_size());
	}

private:
	Tools tools() const {
		return Tools(*this);
	}

	Int symbol_buffer_size() const
	{
		Int bit_size 	= this->element_size(SYMBOLS) * 8;
		Int byte_size 	= Base::roundUpBitsToAlignmentBlocks(bit_size);

		return byte_size / sizeof(Value);
	}
};



template <Int BitsPerSymbol>
class PkdFSSeqTF {
public:
	typedef typename IfThenElse<
				BitsPerSymbol == 1,
				PkdFSSeqTypes<
					1,
					PackedTreeBranchingFactor,
					1024,
					PkdFTree,
					ValueFSECodec,
					BitmapReindexFn,
					BitmapSelectFn,
					BitmapRankFn,
					BitmapToolsFn
				>,
				typename IfThenElse<
					(BitsPerSymbol > 1 && BitsPerSymbol < 8),
					PkdFSSeqTypes<
						BitsPerSymbol,
						PackedTreeBranchingFactor,
						256,
						PkdVTree,
						UByteExintCodec,
						VLEReindexFn,
						SeqSelectFn,
						SeqRankFn,
						SeqToolsFn
					>,
					PkdFSSeqTypes<
						BitsPerSymbol,
						PackedTreeBranchingFactor,
						1024,
						PkdVTree,
						UBigIntEliasCodec,
						VLEReindex8Fn,
						Seq8SelectFn,
						Seq8RankFn,
						Seq8ToolsFn
					>
				>::Result
	>::Result																	Type;
};


}


#endif