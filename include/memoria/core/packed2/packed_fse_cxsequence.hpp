
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_CXSEQUENCE_HPP_
#define MEMORIA_CORE_PACKED_FSE_CXSEQUENCE_HPP_

#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/packed2/packed_fse_array.hpp>

#include <ostream>


namespace memoria {

using namespace std;

template <
	Int BitsPerSymbol_		= 8,
	typename V				= UByte,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= 4096,
	typename Allocator_ 	= PackedAllocator
>
struct PackedFSECxSequenceTypes {
    typedef Int             IndexKey;
    typedef V               Value;
    typedef Allocator_  	Allocator;

    static const Int Blocks                 = 1 << BitsPerSymbol_;
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;
    static const Int BitsPerSymbol			= BitsPerSymbol_;
};


template <typename Types_>
class PackedFSECxSequence: public PackedAllocator {

	typedef PackedAllocator														Base;

public:
	static const UInt VERSION               									= 1;

	typedef Types_																Types;
	typedef PackedFSECxSequence<Types_>               							MyType;

	typedef typename Types::Allocator											Allocator;

	typedef typename Types::IndexKey											IndexKey;
	typedef UByte																Value;

	static const Int BranchingFactor        = Types::BranchingFactor;
	static const Int ValuesPerBranch        = Types::ValuesPerBranch;
	static const Int Indexes        		= Types::Blocks;

	typedef PackedFSETreeTypes <
				Int,
				Int,
				UShort,
				PackedAllocator,
				BranchingFactor,
				64
				>																IndexTypes;
	typedef PackedFSETree<IndexTypes>											Index;

	typedef typename Index::Codec												Codec;

	static const Int IndexSizeThreshold											= 0;


	class Metadata {
		Int size_;
		Int max_size_;
	public:
		Int& size() 		{return size_;}
		Int& max_size() 	{return max_size_;}

		const Int& size() const 	{return size_;}
		const Int& max_size() const {return max_size_;}
	};

public:
	PackedFSECxSequence() {}

	Int& size() {return metadata()->size();}
	const Int& size() const {return metadata()->size();}

	Int max_size() const {return metadata()->max_size();}

protected:

	Metadata* metadata()
	{
		return Base::template get<Metadata>(0);
	}

	const Metadata* metadata() const
	{
		return Base::template get<Metadata>(0);
	}

public:

	Index* index()
	{
		return Base::template get<Index>(1);
	}

	const Index* index() const
	{
		return Base::template get<Index>(1);
	}

	bool has_index() const {
		return Base::element_size(1) > 0;
	}

	Value* symbols()
	{
		return Base::template get<Value>(2);
	}

	const Value* symbols() const
	{
		return Base::template get<Value>(2);
	}

	Int rank(Int idx, Int symbol) const
	{
		if (has_index())
		{
			Int value_block_start 	= (idx / ValuesPerBranch) * ValuesPerBranch;

			Int value_blocks 		= getValueBlocks(max_size());

			Int index_from 			= value_blocks * symbol;
			Int index_to 			= index_from + idx / ValuesPerBranch;

			const Index* seq_index 	= index();

			Int sum = seq_index->sum(index_from, index_to);

			const Value* buffer = symbols();

			for (Int c = value_block_start; c < idx; c++)
			{
				if (buffer[c] == symbol)
				{
					sum++;
				}
			}

			return sum;
		}
		else {
			const Value* buffer = symbols();

			Int sum = 0;

			for (Int c = 0; c < idx; c++)
			{
				if (buffer[c] == symbol)
				{
					sum++;
				}
			}

			return sum;
		}
	}

	Value& value(Int idx) {
		return symbols()[idx];
	}

	const Value& value(Int idx) const {
		return symbols()[idx];
	}

	Int rank(Int from, Int to, Int symbol) const
	{
		return rank(to, symbol) - rank(from, symbol);
	}

	class SelectResult {
		Int idx_;
		Int rank_;
	public:
		SelectResult(Int idx, Int rank): idx_(idx), rank_(rank) {}
		Int idx() const {return idx_;}
		Int rank() const {return rank_;}
	};

	SelectResult select(Int rank, Int symbol) const
	{
		if (has_index())
		{
			Int value_blocks 		= getValueBlocks(max_size());
			Int index_from 			= value_blocks * symbol;
			Int index_to 			= value_blocks * (symbol + 1);

			const Index* seq_index 	= index();

			Int prefix   = seq_index->sum(0, index_from);
			auto result  = seq_index->findLEForward(rank + prefix);

			MEMORIA_ASSERT(result.idx(), <, index_to);

			Int value_block_start 	= (result.idx() - index_from) * ValuesPerBranch;
			Int value_block_end		= value_block_start + ValuesPerBranch;

			Int local_rank 			= rank + prefix - result.prefix();

			const Value* buffer 	= symbols();

			for (Int c = value_block_start; c < value_block_end; c++)
			{
				local_rank -= buffer[c] == symbol;

				if (local_rank == 0)
				{
					return SelectResult(c, rank);
				}
			}

			return SelectResult(size(), 0);
		}
		else
		{
			const Value* buffer = symbols();
			Int size 			= this->size();

			Int local_rank = rank;
			for (Int c = 0; c < size; c++)
			{
				local_rank -= buffer[c] == symbol;

				if (local_rank == 0)
				{
					return SelectResult(c, rank);
				}
			}

			return SelectResult(size, rank - local_rank);
		}
	}

	void dump(std::ostream& out = cout, bool dump_index = true) const
	{
		out<<"size_       = "<<size()<<endl;
		out<<"max_size_   = "<<max_size()<<endl;

		if (dump_index)
		{
			out<<endl;

			out<<"Layout:"<<endl;

			Base::dump(out);

			out<<dec<<endl;

			out<<"Sequence Indexes:"<<endl;

			const Index* index = this->index();
			index->dump(out);
		}

		out<<endl;

		out<<"Symbols:"<<endl;

		const auto* values = this->symbols();

		dumpSymbols<UByte>(out, size(), 8, [&](Int pos) {
			return values[pos];
		});
	}

private:
	struct InitFn {
		Int block_size(Int items_number) const
		{
			return MyType::block_size(items_number);
		}

		Int max_elements(Int block_size) const
		{
			return block_size;
		}
	};

public:
	void init(Int block_size)
	{
		Base::init(block_size, 3);

		Metadata* meta		= Base::template allocate<Metadata>(0);

		meta->max_size() 	= FindTotalElementsNumber2(block_size, InitFn());
		meta->size()	 	= 0;

		Int value_blocks 	= getValueBlocks(meta->max_size());
		Int index_block_size = value_blocks > IndexSizeThreshold ? Index::expected_block_size(value_blocks * Indexes) : 0;

		Base::template allocate<Index>(1, index_block_size);

		Int symbols_size = Base::client_area() - (Base::element_size(0) + Base::element_size(1));

		Base::allocate(2, symbols_size, PackedBlockType::RAW_MEMORY);
	}

	static Int metadata_block_size()
	{
		return Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
	}

	static Int getSingleIndexBlockSize(Int items_number)
	{
		return Index::expected_block_size(items_number);
	}

	static Int getValueBlocks(Int sequence_size)
	{
		return sequence_size / ValuesPerBranch + (sequence_size % ValuesPerBranch ? 1 : 0);
	}

	static Int block_size(Int sequence_size)
	{
		Int max_sequence_size 	= roundUpBytesToAlignmentBlocks(sequence_size);

		Int value_blocks 		= getValueBlocks(max_sequence_size);

		Int index_size			= value_blocks > IndexSizeThreshold ? Index::block_size(value_blocks * Indexes) : 0;

		Int index_block_size 	= roundUpBytesToAlignmentBlocks(index_size);

		Int metadata_size		= roundUpBytesToAlignmentBlocks(sizeof(Metadata));

		Int client_area 		= metadata_size + index_block_size + max_sequence_size;

		Int block_size			= Base::block_size(client_area, 3);

		return block_size;
	}

	Int capacity() const
	{
		const Metadata* meta = metadata();
		return meta->max_size() - meta->size();
	}

	class IndexDescr {
		Int elements_;
		Int length_;
	public:
		IndexDescr(): elements_(0), length_(0) {}
		IndexDescr(Int elements, Int length): elements_(elements), length_(length) {}

		Int elements() const 	{return elements_;}
		Int& elements() 		{return elements_;}

		Int length() const 		{return length_;}
		Int& length() 			{return length_;}
	};

	class IndexStat {
		IndexDescr descriptors_[Indexes];
	public:
		IndexStat() {}

		IndexDescr& operator[](Int idx) 			{return descriptors_[idx];}
		const IndexDescr& operator[](Int idx) const {return descriptors_[idx];}
	};

	IndexStat computeIndexStat() const
	{
		IndexStat stat;

		Int sums[Indexes];
		Int limit = ValuesPerBranch - 1;
		for (auto& v: sums) {v = 0;}

		const Value* values = this->symbols();

		Codec codec;

		Int size = this->size();

		for (Int idx = 0; idx < size; idx++)
		{
			sums[values[idx]]++;

			if (idx == limit)
			{
				for (Int c = 0; c < Indexes; c++)
				{
					if (sums[c] > 0)
					{
						stat[c].elements()++;
						stat[c].length() += codec.length(sums[c]);
					}
				}

				if (limit + ValuesPerBranch < size)
				{
					limit += ValuesPerBranch;
				}
				else {
					limit = size - 1;
				}

				for (auto& v: sums) {v = 0;}
			}
		}

		return stat;
	}

	void clearIndexes()
	{
		Base::clear(1);
	}

	void allocateIndexes(const IndexStat& stat)
	{
		if (has_index())
		{
			Int value_blocks = getValueBlocks(metadata()->max_size());

			Index* index 	= this->index();
			Int index_size	= getSingleIndexBlockSize(value_blocks * Indexes);

			index->init(index_size);
			index->setAllocatorOffset(this);
		}
	}

	Int reindex()
	{
		if (!has_index()) {
			return 0;
		}


		IndexStat stat;// = computeIndexStat();
		clearIndexes();

		allocateIndexes(stat);

		Int sums[Indexes];

		Int limit;

		if (ValuesPerBranch < size())
		{
			limit = ValuesPerBranch - 1;
		}
		else {
			limit = size() - 1;
		}



		for (auto& v: sums) {v = 0;}

		const Value* values = this->symbols();

		Index* seq_index = index();
		Int blocks = getValueBlocks(max_size());
		seq_index->size() = blocks * Indexes;

		Int block_num = 0;

		Int size = this->size();

		for (Int idx = 0; idx < size; idx++)
		{
			sums[values[idx]]++;

			if (idx == limit)
			{
				for (Int c = 0; c < Indexes; c++)
				{
					seq_index->setValue(blocks * c + block_num, sums[c]);
				}

				if (limit + ValuesPerBranch < size)
				{
					limit += ValuesPerBranch;
				}
				else {
					limit = size - 1;
				}

				for (auto& v: sums) {v = 0;}

				block_num++;
			}
		}

		seq_index->reindex();
		return 0;
	}

	void enlarge(Int symbols)
	{
		Int requested_symbols 		= symbols - capacity();
		Int new_max_size			= max_size() + requested_symbols;

		Int requested_block_size 	= MyType::block_size(new_max_size);

		this->resizeBlock(this->symbols(), requested_block_size);
		metadata()->max_size()  	= new_max_size;

		Int value_blocks 		= getValueBlocks(new_max_size);
		if (value_blocks > IndexSizeThreshold)
		{
			Int index_block_size	= Index::block_size(value_blocks * Indexes);

			Int new_index_size 		= this->resizeBlock(index(), index_block_size);

			MEMORIA_ASSERT(new_index_size, >, 0);

			this->index()->init(index_block_size);

			if (value_blocks > IndexSizeThreshold)
			{
				Int idx_max_size = index()->max_size();
				MEMORIA_ASSERT(idx_max_size, ==, value_blocks * Indexes);
			}
		}
	}


	void insert(Int idx, Int symbol)
	{
		if (capacity() == 0)
		{
			enlarge(1);
		}

		Value* values = symbols();

		Int size = this->size();

		MEMORIA_ASSERT(idx, <=, size);

		CopyBuffer(values + idx, values + idx + 1, size - idx);

		values[idx] = symbol;

		this->size()++;
	}

	bool append(Int symbol) {
		return insert(size(), symbol);
	}
};


}


#endif
