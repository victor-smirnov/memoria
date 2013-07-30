
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED2_CXMULTISEQUENCE_HPP_
#define MEMORIA_CORE_PACKED2_CXMULTISEQUENCE_HPP_

#include <memoria/core/packed/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/packed_vle_tree.hpp>
#include <memoria/core/packed/packed_fse_tree.hpp>
#include <memoria/core/tools/elias_codec.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <
	Int BitsPerSymbol_ = 8,

	template <typename>	class IndexType 	= PackedVLETree,
	template <typename>	class CodecType 	= UBigIntEliasCodec,

	template <typename>	class ReindexFnType = VLEReindexFn,
	template <typename>	class SelectFnType	= Sequence8SelectFn,
	template <typename>	class RankFnType	= Sequence8RankFn,
	template <typename>	class ToolsFnType	= Sequence8ToolsFn
>
struct PackedCxMultiSequenceTypes {
	static const Int BitsPerSymbol = BitsPerSymbol_;

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

template <typename Types>
class PackedCxMultiSequence: public PackedAllocator {

	typedef PackedAllocator 								Base;
	typedef PackedCxMultiSequence<Types> 					MyType;

public:

	typedef Packed2TreeTypes<Int>							LabelArrayTypes;
	typedef PackedFSETree<LabelArrayTypes>					LabelArray;
	typedef typename LabelArray::Values						LabelArrayValues;

	typedef PackedFSESeachableSeqTypes<
			Types::BitsPerSymbol,
			PackedTreeBranchingFactor,
			512,
			Types::template Index,
			Types::template Codec,
			Types::template ReindexFn,
			Types::template SelectFn,
			Types::template RankFn,
			Types::template ToolsFn
	>														SequenceTypes;
	typedef PackedFSESearchableSeq<SequenceTypes>			Sequence;

	typedef typename Sequence::SymbolAccessor				SymbolAccessor;
	typedef typename Sequence::ConstSymbolAccessor			ConstSymbolAccessor;

	enum {
		LABELS, SYMBOLS
	};


public:
	PackedCxMultiSequence() {}

	LabelArray* labels() {
		return Base::template get<LabelArray>(LABELS);
	}

	const LabelArray* labels() const {
		return Base::template get<LabelArray>(LABELS);
	}

	Sequence* sequence() {
		return Base::template get<Sequence>(SYMBOLS);
	}

	const Sequence* sequence() const {
		return Base::template get<Sequence>(SYMBOLS);
	}

	static Int empty_size()
	{
		Int labels_block_size 	= LabelArray::empty_size();
		Int symbols_block_size 	= Sequence::empty_size();

		Int block_size = Base::block_size(labels_block_size + symbols_block_size, 2);

		return block_size;
	}

	void init()
	{
		Int block_size = MyType::empty_size();

		Base::init(block_size, 2);

		Base::template allocateEmpty<LabelArray>(LABELS);
		Base::template allocateEmpty<Sequence>(SYMBOLS);
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

		SelectResult result = seq->selectFw(symbol, rank_prefix + rank);
		if (result.idx() - seq_prefix < seq_size)
		{
			return SelectResult(result.idx() - seq_prefix, rank, true);
		}
		else {
			return SelectResult(seq_prefix + seq_size, seq->rank(seq_prefix, seq_prefix + seq_size), false);
		}
	}

	void insertSubsequence(Int idx)
	{
		labels()->insert(idx, LabelArrayValues());
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

	void removeSymbol(Int subseq_num, Int idx)
	{
		Sequence* seq 		= sequence();
		LabelArray* labels 	= this->labels();

		Int seq_prefix	= labels->sum(0, subseq_num);

		MEMORIA_ASSERT(idx, <=, labels->value(subseq_num));

		seq->removeSymbol(seq_prefix + idx);

		labels->value(subseq_num)--;

		labels->reindex();

		seq->reindex();
	}

	void appendSymbol(Int subseq_num, Int symbol)
	{
		LabelArray* labels  = this->labels();
		Int size 			= labels->value(subseq_num);

		insertSymbol(subseq_num, size, symbol);
	}

	void remove(Int subseq_num)
	{
		LabelArray* labels  = this->labels();
		MEMORIA_ASSERT(labels->value(0, subseq_num), ==, 0);

		labels->removeSpace(subseq_num, subseq_num + 1);
	}

	Int subseq_size(Int seq_num) const
	{
		return labels()->value(seq_num);
	}

	Int length(Int seq_num) const
	{
		return subseq_size(seq_num);
	}

	static Int block_size(Int client_area)
	{
		return Base::block_size(client_area, 2);
	}


	ConstSymbolAccessor
	symbol(Int seq_num, Int idx) const
	{
		Int seq_prefix	= labels()->sum(0, seq_num);
		Int size 		= labels()->value(seq_num);

		MEMORIA_ASSERT(idx, <, size);

		return sequence()->symbol(seq_prefix + idx);
	}

	SymbolAccessor
	sumbol(Int seq_num, Int idx)
	{
		Int seq_prefix	= labels()->sum(0, seq_num);
		Int size 		= labels()->value(seq_num);

		MEMORIA_ASSERT(idx, <, size);

		return sequence()->symbol(seq_prefix + idx);
	}

	void dump(ostream& out = cout, bool multi = true, bool dump_index = true) const
	{
//		if (dump_index) {
//			Base::dump(out);
//		}

		out<<"Sequence Labels: "<<endl;
		labels()->dump(out, dump_index);
		out<<endl;

		if (multi)
		{
			if (dump_index && sequence()->has_index())
			{
				sequence()->index()->dump(out);
			}

			auto values = sequence()->symbols();

			auto labels = this->labels();

			Int offset = 0;

			for (Int c = 0; c <labels->size(); c++)
			{
				Int size = labels->value(c);

				out<<"seq: "<<c<<" offset: "<<offset<<" size: "<<size<<endl;

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
