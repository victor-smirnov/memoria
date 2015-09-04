
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_TABLE_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_TABLE_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria {
namespace table   {



template <Int StreamIdx> struct InputTupleSizeH;

template <>
struct InputTupleSizeH<0> {
	template <typename Tuple>
	static auto get(Tuple&& buffer, Int idx) -> decltype(std::get<0>(buffer[idx])[1]) {
		return std::get<0>(buffer[idx])[1];
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{
		std::get<0>(buffer[idx])[1] += value;
	}
};

template <>
struct InputTupleSizeH<1> {
	template <typename Tuple>
	static auto get(Tuple&& buffer, Int idx) -> decltype(std::get<0>(buffer[idx])[1]) {
		return std::get<0>(buffer[idx])[1];
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{
		std::get<0>(buffer[idx])[1] += value;
	}
};


template <>
struct InputTupleSizeH<2> {
	template <typename Tuple>
	static Int get(Tuple&& buffer, Int idx) {
		return 0;
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{}
};





template <typename CtrT, typename Rng>
class RandomDataInputProvider: public memoria::bttl::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {

	using Base = memoria::bttl::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;

public:

	using Position 		= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;
	using Buffer 		= typename Base::Buffer;

	using RunDescr 		= typename Base::RunDescr;

	using NodeBaseG		= typename Base::NodeBaseG;

	template <Int StreamIdx>
	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<StreamIdx>;

	template <Int StreamIdx>
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

private:
	CtrSizeT keys_;
	CtrSizeT columns_;
	CtrSizeT mean_data_size_;
	CtrSizeT data_size_cnt_ 	= 0;
	CtrSizeT data_size_limit_ 	= 0;

	CtrSizeT key_ = 0;
	CtrSizeT col_ = 0;

	Position sizes_;
	Position pos_;

	Int level_ = 0;

	Rng rng_;
public:
	RandomDataInputProvider(CtrT& ctr, CtrSizeT keys, CtrSizeT columns, CtrSizeT mean_data_size, const Rng& rng):
		Base(ctr, Position({500, 500, 500})),
		keys_(keys), columns_(columns), mean_data_size_(mean_data_size),
		rng_(rng)
	{}


	virtual RunDescr populate(const Position& pos)
	{
		if (key_ >= 10) {
			DebugCounter = 1;
		}

		if (key_ < keys_)
		{
			if (level_ == 0)
			{
				std::get<0>(this->buffer_)[pos[0]] = InputTupleAdapter<0>::convert(memoria::core::StaticVector<BigInt, 2>({1, 0}));

				col_ = 0;

				data_size_limit_ = mean_data_size_;

				key_++;
				level_ = 1;
				return RunDescr(0, 1);
			}
			else if (level_ == 1)
			{
				std::get<1>(this->buffer_)[pos[1]] = InputTupleAdapter<1>::convert(memoria::core::StaticVector<BigInt, 2>({1, 0}));

				col_++;

				if (data_size_limit_ > 0)
				{
					data_size_cnt_ = 0;
					level_ = 2;
				}
				else if (col_ >= columns_)
				{
					level_ = 0;
				}

				return RunDescr(1, 1);
			}
			else {
				CtrSizeT rest = data_size_limit_ - data_size_cnt_;

				CtrSizeT capacity 	= this->capacity_[2];
				CtrSizeT ppos 		= pos[2];

				CtrSizeT limit;

				if (ppos + rest > capacity) {
					limit = capacity - ppos;
				}
				else {
					limit = rest;
				}

				for (CtrSizeT c = ppos; c < ppos + limit; c++) {
					std::get<2>(this->buffer_)[c] = InputTupleAdapter<2>::convert(col_ % 256);
				}

				data_size_cnt_ += limit;

				if (data_size_cnt_ >= data_size_limit_)
				{
					if (col_ < columns_)
					{
						level_ = 1;
					}
					else {
						level_ = 0;
					}
				}

				return RunDescr(2, limit);
			}
		}
		else {
			return RunDescr(-1);
		}
	}

};



}
}

#endif
