
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
	Int BitsPerSymbol_,
	typename V,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= 512,
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
				PackedDynamicAllocator<>,
				BranchingFactor,
				64
				>																IndexTypes;
	typedef PackedFSETree<IndexTypes>											Index;

	typedef typename Index::Codec												Codec;


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
		Int value_blocks 		= getValueBlocks(max_size());
		Int index_from 			= value_blocks * symbol;

		const Index* seq_index 	= index();

		Int prefix  = seq_index->sum(index_from);

		auto result = seq_index->findLE(rank + prefix);


		Int value_block_start 	= (result.idx() - value_blocks * symbol) * ValuesPerBranch;
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

	void dump(std::ostream& out = cout) const
	{
		out<<"size_       = "<<size()<<endl;
		out<<"max_size_   = "<<max_size()<<endl;
		out<<endl;

		out<<"Layout:"<<endl;

		Base::dump(out);

		out<<dec<<endl;

		out<<"Sequence Indexes:"<<endl;

		const Index* index = this->index();
		index->dump();

		out<<endl;

		out<<"Data:"<<endl;

		const auto* values = this->symbols();

		dumpArray<UByte>(out, size(), [&](Int pos) {
			return (UByte)values[pos];
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

		Int allocated 		= Base::allocate(0, sizeof(Metadata)).size();

		Metadata* meta 		= metadata();

		meta->max_size() 	= FindTotalElementsNumber2(block_size, InitFn());
		meta->size()	 	= 0;

		Int value_blocks 	= getValueBlocks(meta->max_size());

		Int index_block_size = Index::expected_block_size(value_blocks * Indexes);

		allocated 			+= Base::allocate(1, index_block_size).size();
		Base::setBlockType(1, PackedBlockType::ALLOCATABLE);

		Index* index = this->index();
		index->init(index_block_size);

		Base::allocate(2, Base::client_area() - allocated);
	}

	static Int metadata_block_size()
	{
		return Base::roundBytesToAlignmentBlocks(sizeof(Metadata));
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
		Int max_sequence_size 	= roundBytesToAlignmentBlocks(sequence_size);

		Int value_blocks 		= getValueBlocks(max_sequence_size);

		Int index_size			= getSingleIndexBlockSize(value_blocks * Indexes);

		Int index_block_size 	= roundBytesToAlignmentBlocks(index_size);

		Int metadata_size		= roundBytesToAlignmentBlocks(sizeof(Metadata));

		Int client_area 		= metadata_size + index_block_size + max_sequence_size;

		Int block_size			= Base::block_size(client_area, 3);

		return block_size;
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
		Int value_blocks = getValueBlocks(metadata()->max_size());

		Index* index 	= this->index();
		Int index_size	= getSingleIndexBlockSize(value_blocks * Indexes);

		index->init(index_size);
	}

	Int reindex()
	{
		IndexStat stat = computeIndexStat();
		clearIndexes();

		allocateIndexes(stat);

		Int sums[Indexes];
		Int limit = ValuesPerBranch - 1;
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

};


}


#endif
