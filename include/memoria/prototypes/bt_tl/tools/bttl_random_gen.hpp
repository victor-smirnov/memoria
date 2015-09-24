
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_RANDOMGEN_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TL_TOOLS_RANDOMGEN_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt_tl/bttl_tools.hpp>
#include <memoria/prototypes/bt_tl/bttl_input.hpp>
#include <memoria/core/container/container.hpp>

namespace memoria {
namespace bttl    {



template <
	typename CtrT,
	typename RngT
>
class RandomDataInputProvider: public StreamingCtrInputProvider<CtrT, RandomDataInputProvider<CtrT, RngT>> {

	using Base = StreamingCtrInputProvider<CtrT, RandomDataInputProvider<CtrT, RngT>>;

public:

	using CtrSizesT 	= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;

	using Rng 			= RngT;

	using RunDescr 		= typename Base::RunDescr;


	template <Int StreamIdx>
	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<StreamIdx>;

	template <Int StreamIdx>
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

	static constexpr Int Streams = CtrT::Types::Streams;

private:
	CtrSizesT counts_;
	CtrSizesT limits_;
	CtrSizesT current_limits_;
	CtrSizesT totals_;

	Int level_ = 0;

	Rng& rng_;
public:
	RandomDataInputProvider(
			CtrT& ctr,
			const CtrSizesT& limits,
			Int level,
			Rng& rng,
			const CtrSizesT& buffer_size = CtrSizesT(500)
	):
		Base(ctr, buffer_size),
		limits_(limits),
		level_(level),
		rng_(rng)
	{
		current_limits_[0] = limits_[0];

		for (Int c = 1; c < Streams; c++)
		{
			current_limits_[c] = getRandom(limits_[c]);
		}
	}

	auto query()
	{
		if (counts_[level_] < current_limits_[level_])
		{
			if (level_ < Streams - 1)
			{
				return RunDescr(level_++, 1);
			}
			else {
				return RunDescr(level_, current_limits_[level_] - counts_[level_]);
			}
		}
		else if (level_ > 0)
		{
			counts_[level_] = 0;
			current_limits_[level_] = getRandom(limits_[level_]);

			level_--;

			return this->query();
		}
		else {
			return RunDescr(-1);
		}
	}

	template <typename Buffer>
	void populate(StreamTag<0>, Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		buffer[start] = InputTupleAdapter<0>::convert(IL<BigInt>({2}));

		counts_[0] += length;
		totals_[0] += length;
	}

	template <Int StreamIdx, typename Buffer>
	void populate(StreamTag<StreamIdx>, Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		buffer[start] = InputTupleAdapter<StreamIdx>::convert(IL<BigInt>({0}));

		counts_[StreamIdx] += length;
		totals_[StreamIdx] += length;
	}

	template <typename Buffer>
	void populateLastStream(Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		auto col = counts_[Streams - 2];

		for (auto c = start; c < start + length; c++) {
			buffer[c] = InputTupleAdapter<Streams - 1>::convert(col % 256);
		}

		counts_[Streams - 1] += length;
		totals_[Streams - 1] += length;
	}

	const CtrSizesT& consumed(){
		return totals_;
	}
private:
	CtrSizeT getRandom(CtrSizeT limit)
	{
		return 1 + rng_() % (2 * limit - 1);
	}
};







template <
	typename CtrT
>
class DeterministicDataInputProvider: public StreamingCtrInputProvider<CtrT, DeterministicDataInputProvider<CtrT>> {

	using Base = StreamingCtrInputProvider<CtrT, DeterministicDataInputProvider<CtrT>>;

public:

	using CtrSizesT 	= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;

	using RunDescr 		= typename Base::RunDescr;

	template <Int StreamIdx>
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

	static constexpr Int Streams = CtrT::Types::Streams;

private:
	CtrSizesT counts_;
	CtrSizesT limits_;

	CtrSizesT totals_;

	Int level_;

public:
	DeterministicDataInputProvider(
			CtrT& ctr,
			const CtrSizesT& limits,
			Int level = 0,
			const CtrSizesT& buffer_size = CtrSizesT(500)
	):
		Base(ctr, buffer_size),
		limits_(limits),
		level_(level)
	{}

	auto query()
	{
		if (counts_[level_] < limits_[level_])
		{
			if (level_ < Streams - 1)
			{
				return RunDescr(level_++, 1);
			}
			else {
				return RunDescr(level_, limits_[level_] - counts_[level_]);
			}
		}
		else if (level_ > 0)
		{
			counts_[level_] = 0;
			level_--;

			return this->query();
		}
		else {
			return RunDescr(-1);
		}
	}

	template <typename Buffer>
	void populate(StreamTag<0>, Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		buffer[start] = InputTupleAdapter<0>::convert(IL<BigInt>({2}));

		counts_[0] += length;
		totals_[0] += length;
	}

	template <Int StreamIdx, typename Buffer>
	void populate(StreamTag<StreamIdx>, Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		buffer[start] = InputTupleAdapter<StreamIdx>::convert(IL<BigInt>({0}));

		counts_[StreamIdx] += length;
		totals_[StreamIdx] += length;
	}

	template <typename Buffer>
	void populateLastStream(Buffer&& buffer, CtrSizeT start, CtrSizeT length)
	{
		auto col = counts_[Streams - 2];

		for (auto c = start; c < start + length; c++) {
			buffer[c] = InputTupleAdapter<Streams - 1>::convert(col % 256);
		}

		counts_[Streams - 1] += length;
		totals_[Streams - 1] += length;
	}

	const CtrSizesT& consumed(){
		return totals_;
	}
};





}
}

#endif
