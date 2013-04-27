
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

		Int label_array_size = roundUpBytesToAlignmentBlocks(client_area / 5);
		Int sequence_size	 = client_area - label_array_size;

		Base::template allocate<LabelArray>(0, label_array_size);
		Base::template allocate<Sequence>(1, sequence_size);
	}

	Int rank(Int subseq_num, Int to, Int symbol) const
	{
		const Sequence* 	seq 	= sequence();
		const LabelArray*	labels 	= this->labels();

		MEMORIA_ASSERT(to, <=, labels->value(subseq_num));

		Int seq_pefix	= labels->sum(0, subseq_num);

		return seq->rank(seq_pefix, seq_pefix + to, symbol);
	}

	SelectResult select(Int subseq_num, Int rank, Int symbol) const
	{
		const Sequence* 	seq 	= sequence();
		const LabelArray*	labels 	= this->labels();

		Int seq_size	= labels->value(subseq_num);
		Int seq_prefix	= labels->sum(0, subseq_num);
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

	void insertSubsequence(Int idx)
	{
		labels()->insert(idx, 0);
		labels()->reindex();
	}

	void appendSubsequence()
	{
		insertSubsequence(labels()->size());
	}

	void insertSymbol(Int subseq_num, Int idx, Int symbol)
	{
		Sequence* seq 		= sequence();
		LabelArray* labels 	= this->labels();

		Int seq_prefix	= labels->sum(0, subseq_num);

		MEMORIA_ASSERT(idx, <=, labels->value(subseq_num));

		seq->insert(seq_prefix + idx, symbol);

		labels->value(subseq_num)++;

		labels->reindex();

		seq->reindex();
	}

	void appendSymbol(Int subseq_num, Int symbol)
	{
		LabelArray* labels  = this->labels();
		Int size 			= labels->value(subseq_num);

		insertSymbol(subseq_num, size, symbol);
	}

	Int subseq_size(Int seq_num) const
	{
		return labels()->value(seq_num);
	}

	static Int block_size(Int client_area)
	{
		return Base::block_size(client_area, 2);
	}

	const UByte& value(Int seq_num, Int idx) const
	{
		Int seq_prefix	= labels()->sum(0, seq_num);
		Int size 		= labels()->value(seq_num);

		MEMORIA_ASSERT(idx, <, size);

		return sequence()->value(seq_prefix + idx);
	}

	UByte& value(Int seq_num, Int idx)
	{
		Int seq_prefix	= labels()->sum(0, seq_num);
		Int size 		= labels()->value(seq_num);

		MEMORIA_ASSERT(idx, <, size);

		return sequence()->value(seq_prefix + idx);
	}

	void dump(ostream& out = cout, bool multi = true, bool dump_index = true) const
	{
		if (dump_index) {
			Base::dump(out);
		}

		out<<"Sequence Labels: "<<endl;
		labels()->dump(out, dump_index);
		out<<endl;

		if (multi)
		{
			if (dump_index) {
				sequence()->index()->dump(out);
			}

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
			sequence()->dump(out, dump_index);
			out<<endl;
		}
	}
};

}


#endif
