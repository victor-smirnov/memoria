
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_CXSEQUENCE_HPP_
#define MEMORIA_CORE_PACKED_FSE_CXSEQUENCE_HPP_

#include <memoria/core/packed2/packed_dynamic_allocator.hpp>
#include <memoria/core/packed2/packed_fse_array.hpp>

#include <ostream>


namespace memoria {

using namespace std;

template <
	Int BitsPerSymbol_,
	typename V,
	typename Allocator_ 	= EmptyAllocator,
	Int BF 					= PackedTreeBranchingFactor,
	Int VPB 				= 4096
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
class PackedFSECxSequence: public PackedAllocatable {

	typedef PackedAllocatable													Base;

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


	typedef PackedDynamicAllocator<>											IndexesArrayType;

private:

	Int block_size_;
	Int size_;
	Int max_size_;
	Value buffer_[];

public:
	PackedFSECxSequence() {}

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	Int max_size() {return max_size_;}
	Int block_size() {return block_size_;}

	IndexesArrayType& indexes() 			{return *T2T<IndexesArrayType*>(buffer_);}
	const IndexesArrayType& indexes() const {return *T2T<const IndexesArrayType*>(buffer_);}

	Index* index(Int idx)
	{
		return T2T<Index*>(indexes().describe(idx).ptr());
	}

	const Index* index(Int idx) const
	{
		return T2T<const Index*>(indexes().describe(idx).ptr());
	}

	Value* symbols()
	{
		Int indexes_block_size = indexes().block_size();
		return T2T<Value*>(buffer_ + indexes_block_size);
	}

	const Value* symbols() const
	{
		Int indexes_block_size = indexes().block_size();
		return T2T<const Value*>(buffer_ + indexes_block_size);
	}

	Int rank(Int idx, Int symbol) const
	{
		Int value_block_start 	= (idx / ValuesPerBranch) * ValuesPerBranch;

		Int value_blocks 		= getValueBlocks(max_size_);

		Int index_from 			= value_blocks * symbol;
		Int index_to 			= index_from + idx / ValuesPerBranch;

		const Index* seq_index 	= index(0);

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
		Int value_blocks 		= getValueBlocks(max_size_);
		Int index_from 			= value_blocks * symbol;

		const Index* seq_index 	= index(0);

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
		out<<"size_       = "<<size_<<endl;
		out<<"max_size_   = "<<max_size_<<endl;
		out<<endl;

		out<<"Layout:"<<endl;

		indexes().dump(out);

		out<<dec<<endl;

		out<<"Indexes:"<<endl;

		for (Int c = 0; c < indexes().getElementsNumber(); c++)
		{
			AllocationBlockConst block = indexes().describe(c);

			if (block.size() > 0)
			{
				out<<"Index "<<c<<":"<<endl;
				block.cast<Index>()->dump(out);
				out<<endl;
			}
		}

		out<<endl;

		out<<"Data:"<<endl;

		const auto* values = this->symbols();

		dumpArray<UByte>(out, size_, [&](Int pos) -> UByte {
			return (UByte)values[pos];
		});
	}

private:
	struct InitFn {
		Int getBlockSize(Int items_number) const
		{
			return MyType::getBlockSize(items_number);
		}

		Int extend(Int items_number) const
		{
			return items_number;
		}

		Int getIndexSize(Int items_number) const
		{
			return MyType::getSingleIndexBlockSize(items_number);
		}
	};

public:
	void init(Int block_size)
	{
		size_ = 0;
		max_size_ = FindTotalElementsNumber(block_size, InitFn());

		Int indexes_block_size = block_size - max_size_ - sizeof(MyType);

		indexes().init(indexes_block_size, 1);
	}


	static Int getSingleIndexBlockSize(Int items_number)
	{
		return Index::expected_block_size(items_number);
	}

	static Int getValueBlocks(Int sequence_size)
	{
		return sequence_size / ValuesPerBranch + (sequence_size % ValuesPerBranch ? 1 : 0);
	}

	static Int getBlockSize(Int sequence_size)
	{
		Int value_blocks 		= getValueBlocks(sequence_size);
		Int index_block_size	= getSingleIndexBlockSize(value_blocks * Indexes);
		index_block_size		= IndexesArrayType::roundBytesToAlignmentBlocks(index_block_size);

		Int indexes_block_size  = IndexesArrayType::block_size_by_elements(1) + index_block_size;

		return sizeof(MyType) + indexes_block_size + sequence_size;
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

		for (Int idx = 0; idx < size_; idx++)
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

				if (limit + ValuesPerBranch < size_)
				{
					limit += ValuesPerBranch;
				}
				else {
					limit = size_ - 1;
				}

				for (auto& v: sums) {v = 0;}
			}
		}

		return stat;
	}

	void clearIndexes()
	{
		for (Int idx = indexes().getElementsNumber() - 1; idx >= 0; idx--)
		{
			indexes().free(idx);
		}

		indexes().clearMemoryBlock();
	}

	void allocateIndexes(const IndexStat& stat)
	{
//		Int total_length = 0;
//		Int total_size   = 0;
//
//		for (Int idx = 0; idx < Indexes; idx++)
//		{
//			total_length += stat[idx].length();
//			total_size	 += stat[idx].elements();
//		}

		Int total_size = getValueBlocks(max_size_) * 256;

		Int index_block_size = Index::expected_block_size(total_size);
		allocateIndex(0, index_block_size);
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

		Index* seq_index = index(0);
		Int blocks = getValueBlocks(max_size_);
		seq_index->size() = blocks * Indexes;

		Int block_num = 0;

		for (Int idx = 0; idx < size_; idx++)
		{
			sums[values[idx]]++;

			if (idx == limit)
			{
				for (Int c = 0; c < Indexes; c++)
				{
					seq_index->setValue(blocks * c + block_num, sums[c]);
				}

				if (limit + ValuesPerBranch < size_)
				{
					limit += ValuesPerBranch;
				}
				else {
					limit = size_ - 1;
				}

				for (auto& v: sums) {v = 0;}

				block_num++;
			}
		}

		seq_index->reindex();

		return 0;
	}



protected:
	Index* allocateIndex(Int idx, Int index_block_size)
	{
		AllocationBlock block = indexes().allocate(idx, index_block_size);
		Index* index = T2T<Index*>(block.ptr());
		index->init(block.size());
		index->setAllocatorOffset(&indexes());
		return index;
	}
};


}


#endif
