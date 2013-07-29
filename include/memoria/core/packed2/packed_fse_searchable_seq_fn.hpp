
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_SEARCHABLESEQ_FN_HPP_
#define MEMORIA_CORE_PACKED_FSE_SEARCHABLESEQ_FN_HPP_

#include <memoria/core/tools/bitmap_select.hpp>


namespace memoria {


template <typename Seq>
class BitmapSelectFn {

	static const Int BitsPerSymbol 			= Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 1, "BitmapSelectFn<> can only be used with 1-bit sequences");

	const Seq& seq_;
public:
	BitmapSelectFn(const Seq& seq): seq_(seq) {}

	SelectResult operator()(Int start, Int end, Int symbol, BigInt rank)
	{
		if (symbol)
		{
			return Select1FW(seq_.symbols(), start, end, rank);
		}
		else {
			return Select0FW(seq_.symbols(), start, end, rank);
		}
	}
};


template <typename Seq>
class BitmapRankFn {

	static const Int BitsPerSymbol 			= Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 1, "BitmapRankFn<> can only be used with 1-bit sequences");


	const Seq& seq_;
public:
	BitmapRankFn(const Seq& seq): seq_(seq) {}

	Int operator()(Int start, Int end, Int symbol)
	{
		if (symbol)
		{
			return PopCount(seq_.symbols(), start, end);
		}
		else {
			return (end - start) - PopCount(seq_.symbols(), start, end);
		}
	}
};


template <typename Seq>
class BitmapSumFn {
	typedef typename Seq::Values 			Values;

	static const Int BitsPerSymbol 			= Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 1, "BitmapSumFn<> can only be used with 1-bit sequences");

	const Seq& seq_;
public:
	BitmapSumFn(const Seq& seq): seq_(seq) {}

	Values operator()(Int start, Int end)
	{
		Values values;

		BigInt rank1 = PopCount(seq_.symbols(), start, end);

		values[2] = rank1;
		values[0] = end - start;
		values[1] = end - start - rank1;

		return values;
	}
};

template <typename Seq>
class SequenceSumFn {
	typedef typename Seq::Values 			Values;
	static const Int BitsPerSymbol 			= Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
				"SequenceSumFn<> can only be used with 2-7-bit sequences");

	const Seq& seq_;
public:
	SequenceSumFn(const Seq& seq): seq_(seq) {}

	Values operator()(Int start, Int end)
	{
		Values values;

		values[0] = end - start;

		auto symbols = seq_.symbols();

		for (Int idx = start * BitsPerSymbol; idx < end * BitsPerSymbol; idx += BitsPerSymbol)
		{
			Int symbol = GetBits(symbols, idx, BitsPerSymbol) + 1;
			values[symbol]++;
		}

		return values;
	}
};

template <typename Seq>
class Sequence8SumFn {
	typedef typename Seq::Values 			Values;
	static const Int BitsPerSymbol 			= Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 8, "Sequence8SumFn<> can only be used with 8-bit sequences");

	const Seq& seq_;
public:
	Sequence8SumFn(const Seq& seq): seq_(seq) {}

	Values operator()(Int start, Int end)
	{
		Values values;

		values[0] = end - start;

		auto symbols = seq_.symbols();

		for (Int idx = start; idx < end; idx++)
		{
			Int symbol = symbols[idx] + 1;
			values[symbol]++;
		}

		return values;
	}
};

template <typename Seq>
class SequenceSelectFn {
	typedef typename Seq::Value Value;

	static const Int BitsPerSymbol = Seq::BitsPerSymbol;

	const Seq& seq_;

	static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
			"SequenceSelectFn<> can only be used with 2-7-bit sequences");

public:
	SequenceSelectFn(const Seq& seq): seq_(seq) {}

	SelectResult operator()(Int start, Int end, Value symbol, Int rank)
	{
		Int cnt = 0;

		auto buf = seq_.symbols();

		for (Int c = start * BitsPerSymbol; c < end * BitsPerSymbol; c += BitsPerSymbol)
		{
			if (TestBits(buf, c, symbol, BitsPerSymbol))
			{
				cnt++;

				if (cnt == rank)
				{
					return SelectResult(c / BitsPerSymbol, rank, true);
				}
			}
		}

		return SelectResult(end, cnt, false);
	}
};


template <typename Seq>
class SequenceRankFn {
	typedef typename Seq::Value Value;

	static const Int BitsPerSymbol = Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol > 1 && BitsPerSymbol < 8,
			"SequenceRankFn<> can only be used with 2-7-bit sequences");

	const Seq& seq_;

public:
	SequenceRankFn(const Seq& seq): seq_(seq) {}

	Int operator()(Int start, Int end, Value symbol)
	{
		Int cnt = 0;

		auto buf = seq_.symbols();

		for (Int c = start * BitsPerSymbol; c < end * BitsPerSymbol; c += BitsPerSymbol)
		{
			if (TestBits(buf, c, symbol, BitsPerSymbol))
			{
				cnt++;
			}
		}

		return cnt;
	}
};





template <typename Seq>
class Sequence8SelectFn {

	static const Int BitsPerSymbol = Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 8,
			"Sequence8SelectFn<> can only be used with 8-bit sequences");

	const Seq& seq_;

public:
	Sequence8SelectFn(const Seq& seq): seq_(seq) {}

	SelectResult operator()(Int start, Int end, Int symbol, Int rank)
	{
		Int cnt = 0;

		auto buf = seq_.symbols();

		for (Int c = start; c < end; c++)
		{
			if (buf[c] == symbol)
			{
				cnt++;
				if (cnt == rank)
				{
					return SelectResult(c, rank, true);
				}
			}
		}

		return SelectResult(end, cnt, false);
	}
};


template <typename Seq>
class Sequence8RankFn {

	static const Int BitsPerSymbol = Seq::BitsPerSymbol;

	static_assert(BitsPerSymbol == 8,
			"Sequence8RankFn<> can only be used with 2-7-bit sequences");

	const Seq& seq_;
public:
	Sequence8RankFn(const Seq& seq): seq_(seq) {}

	Int operator()(Int start, Int end, Int symbol)
	{
		Int cnt = 0;

		auto buf = seq_.symbols();

		for (Int c = start; c < end; c++)
		{
			if (buf[c] == symbol)
			{
				cnt++;
			}
		}

		return cnt;
	}
};


template <typename Seq>
class BitmapReindexFn {
	typedef typename Seq::Index 		Index;
	typedef typename Index::Values 		Values;

	static const Int BitsPerSymbol 				= Seq::BitsPerSymbol;
	static const Int ValuesPerBranch 			= Seq::ValuesPerBranch;
	static const Int Blocks						= Index::Blocks;
	static const bool FixedSizeElementIndex		= Index::FixedSizeElement;

	static_assert(BitsPerSymbol == 1,
			"BitmapReindexFn<> can only be used with 1-bit sequences");

	static_assert(FixedSizeElementIndex,
			"BitmapReindexFn<> can only be used with PackedFSETree<>-indexed sequences ");

public:
	void operator()(Seq& seq)
	{
		Int size = seq.size();

		if (size > ValuesPerBranch)
		{
			Int index_size 	= size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
			seq.createIndex(index_size);

			Index* index = seq.index();

			Int pos = 0;

			auto symbols = seq.symbols();

			index->insert(0, index_size, [&]() -> Values
			{
				Int next = pos + ValuesPerBranch;
				Int max = next <= size ? next : size;

				Values values;

				values[1] = PopCount(symbols, pos, max);
				values[0] = (max - pos) - values[1];

				pos = next;

				return values;
			});
		}
		else {
			seq.removeIndex();
		}
	}
};


template <typename Seq>
class ReindexFn {
	typedef typename Seq::Index 		Index;
	typedef typename Index::Values 		Values;

	static const Int BitsPerSymbol 				= Seq::BitsPerSymbol;
	static const Int ValuesPerBranch 			= Seq::ValuesPerBranch;
	static const Int Blocks						= Index::Blocks;
	static const bool FixedSizeElementIndex		= Index::FixedSizeElement;

	static_assert(BitsPerSymbol > 2,
				"ReindexFn<> can only be used with 2-8-bit sequences");

	static_assert(FixedSizeElementIndex,
					"ReindexFn<> can only be used with PackedFSETree<>-indexed sequences ");

public:
	void operator()(Seq& seq)
	{
		Int size = seq.size();

		if (size > ValuesPerBranch)
		{
			Int index_size 	= size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);
			seq.createIndex(index_size);

			Index* index = seq.index();

			Int pos = 0;

			index->insert(0, index_size, [&]() -> Values
			{
				auto symbols = seq.symbols();

				Int next = pos + ValuesPerBranch;
				Int max = next <= size ? next : size;

				Values values;

				for (; pos < max; pos++)
				{
					Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
					values[symbol]++;
				}

				return values;
			});
		}
		else {
			seq.removeIndex();
		}
	}
};


template <typename Seq>
class VLEReindexFn {
	typedef typename Seq::Index 		Index;
	typedef typename Index::Values 		Values;
	typedef typename Index::Codec 		Codec;

	static const Int BitsPerSymbol 				= Seq::BitsPerSymbol;
	static const Int ValuesPerBranch 			= Seq::ValuesPerBranch;
	static const Int Blocks						= Index::Blocks;
	static const bool FixedSizeElementIndex		= Index::FixedSizeElement;

	static_assert(BitsPerSymbol > 2,
				"ReindexFn<> can only be used with 2-8-bit sequences");

	static_assert(!FixedSizeElementIndex,
				"ReindexFn<> can only be used with PackedVLETree<>-indexed sequences ");

public:
	void operator()(Seq& seq)
	{
		Int size = seq.size();

		if (size > ValuesPerBranch)
		{
			Codec codec;

			Int length = 0;

			auto symbols = seq.symbols();

			for (Int b = 0; b < size; b += ValuesPerBranch)
			{
				Int next = b + ValuesPerBranch;
				Int max = next <= size ? next : size;

				Values values;

				for (Int pos = b; pos < max; pos++)
				{
					Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
					values[symbol]++;
				}

				for (Int c = 0; c < Blocks; c++)
				{
					length += codec.length(values[c]);
				}
			}

			seq.createIndex(length);

			Index* index = seq.index();

			Int pos = 0;

			symbols = seq.symbols();

			Int index_size 	= size / ValuesPerBranch + (size % ValuesPerBranch == 0 ? 0 : 1);

			index->insert(0, index_size, [&]() -> Values
			{
				Int next = pos + ValuesPerBranch;
				Int max = next <= size ? next : size;

				Values values;

				for (; pos < max; pos++)
				{
					Int symbol = GetBits(symbols, pos * BitsPerSymbol, BitsPerSymbol);
					values[symbol]++;
				}

				return values;
			});
		}
		else {
			seq.removeIndex();
		}
	}
};




}


#endif
