
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED2_CXMULTISEQUENCE_HPP_
#define MEMORIA_CORE_PACKED2_CXMULTISEQUENCE_HPP_

#include <memoria/core/packed2/packed_louds_tree.hpp>
#include <memoria/core/packed2/packed_fse_cxsequence.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <typename Types>
class PackedCxMultiSequence: public PackedAllocator {

	typedef PackedAllocator 								Base;
	typedef PackedCxMultiSequence<Types> 					MyType;

public:

	typedef PackedFSETreeTypes<Int, Int, Int>				LabelArrayTypes;
	typedef PackedFSETree<LabelArrayTypes>					LabelArray;

	typedef PackedFSECxSequenceTypes<>						SequenceTypes;
	typedef PackedFSECxSequence<SequenceTypes>				Sequence;

	typedef typename Sequence::SelectResult					SelectResult;

public:
	PackedCxMultiSequence() {}

	LabelArray* labels() {
		return Base::template get<LabelArray>(0);
	}

	const LabelArray* labels() const {
		return Base::template get<LabelArray>(0);
	}

	Sequence* sequence() {
		return Base::template get<Sequence>(1);
	}

	const Sequence* sequence() const {
		return Base::template get<Sequence>(1);
	}

	void init(Int block_size)
	{
		Base::init(block_size, 2);

		Int client_area = Base::client_area();

		Int label_array_size = roundBytesToAlignmentBlocks(client_area / 5);
		Int sequence_size	 = client_area - label_array_size;


		auto block = Base::allocate(0, label_array_size);
		Base::setBlockType(0, PackedBlockType::ALLOCATABLE);

		LabelArray* labels= this->labels();
		labels->init(block.size());
		labels->setAllocatorOffset(this);

		block = Base::allocate(1, sequence_size);
		MEMORIA_ASSERT(block, == , true);

		Base::setBlockType(1, PackedBlockType::ALLOCATABLE);

		Sequence* seq = this->sequence();
		seq->init(sequence_size);
		seq->setAllocatorOffset(this);
	}

	Int rank(Int subseq_num, Int to, Int symbol) const
	{
		const Sequence* 	seq 	= sequence();
		const LabelArray*	labels 	= this->labels();

		MEMORIA_ASSERT(to, <=, labels->value(subseq_num));

		Int seq_pefix	= labels->sum(subseq_num);

		return seq->rank(seq_pefix, seq_pefix + to, symbol);
	}

	SelectResult select(Int subseq_num, Int rank, Int symbol) const
	{
		const Sequence* 	seq 	= sequence();
		const LabelArray*	labels 	= this->labels();

		Int seq_size	= labels->value(subseq_num);
		Int seq_prefix	= labels->sum(subseq_num);
		Int rank_prefix = seq->rank(seq_prefix, symbol);

		SelectResult result = seq->select(rank_prefix + rank, symbol);
		if (result.idx() - seq_prefix < seq_size)
		{
			return SelectResult(result.idx() - seq_prefix, rank);
		}
		else {
			return SelectResult(seq_prefix + seq_size, seq->rank(seq_prefix, seq_prefix + seq_size));
		}
	}

	bool insertSubsequence(Int idx)
	{
		if (labels()->insert(idx, 0))
		{
			labels()->reindex();
			return true;
		}
		else {
			return false;
		}
	}

	bool appendSubsequence()
	{
		return insertSubsequence(labels()->size());
	}

	bool insertSymbol(Int subseq_num, Int idx, Int symbol)
	{
		Sequence* seq 		= sequence();
		LabelArray* labels 	= this->labels();

		Int seq_prefix	= labels->sum(subseq_num);

		MEMORIA_ASSERT(idx, <=, labels->value(subseq_num));

		if (seq->insert(seq_prefix + idx, symbol))
		{
			labels->value(subseq_num)++;

			labels->reindex();

//			seq->reindex();

			return true;
		}
		else {
			return false;
		}
	}

	bool appendSymbol(Int subseq_num, Int symbol)
	{
		LabelArray* labels  = this->labels();
		Int size 			= labels->value(subseq_num);

		return insertSymbol(subseq_num, size, symbol);
	}

	static Int block_size(Int client_area)
	{
		return Base::block_size(client_area, 2);
	}

	void dump(ostream& out = cout, bool multi = true) const
	{
		Base::dump(out);

		out<<"Sequence Labels: "<<endl;
		labels()->dump(out);
		out<<endl;

		if (multi)
		{
			sequence()->index()->dump(out);

			const auto* values = sequence()->symbols();

			const auto labels = this->labels();

			Int offset = 0;

			for (Int c = 0; c <labels->size(); c++)
			{
				Int size = labels->value(c);

				out<<"offset: "<<offset<<" size: "<<size<<endl;

				dumpSymbols<typename Sequence::Value>(out, size, 8, [values, offset](Int idx) {
					return values[idx + offset];
				});

				offset += size;

				out<<endl<<endl;
			}
		}
		else {
			out<<"Sequence: "<<endl;
			sequence()->dump(out);
			out<<endl;
		}
	}
};

}


#endif
