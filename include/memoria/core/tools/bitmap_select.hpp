
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_BITMAP_SELECT_H
#define _MEMORIA_CORE_TOOLS_BITMAP_SELECT_H

#include <memoria/core/tools/bitmap.hpp>

namespace memoria    {

using namespace std;

using namespace memoria::vapi;






namespace intrnl2 {

template <typename T>
bool SelectFW(T arg, size_t& total, size_t count, size_t& stop)
{
	size_t popcnt = PopCnt(arg & MakeMask<T>(0, stop));

	if (total + popcnt < count)
	{
		total += popcnt;
		return false;
	}
	else
	{
		T mask0 = 1;

		for (size_t c = 0; c < stop; mask0 <<= 1, c++)
		{
			total += ((arg & mask0) != 0);

			if (total == count)
			{
				stop = c;
				return true;
			}
		}

		return false;
	}
}


template <typename T>
bool SelectBW(T arg, size_t& total, size_t count, size_t start, size_t& delta)
{
	size_t popcnt = PopCnt(arg & MakeMask<T>(0, start + 1));

	if (total + popcnt < count)
	{
		total += popcnt;
		return false;
	}
	else
	{
		T mask0 = static_cast<T>(1) << start;

		for (size_t c = start; c > 0; mask0 >>= 1, c--)
		{
			total += ((arg & mask0) != 0);

			if (total == count)
			{
				delta = start - c;
				return true;
			}
		}

		total += ((arg & mask0) != 0);

		if (total == count)
		{
			delta = start;
			return true;
		}

		return false;
	}
}


}

class SelectResult {
	size_t idx_;
	size_t count_;
	bool found_;
public:
	SelectResult(size_t idx, size_t count, bool found): idx_(idx), count_(count), found_(found) {}

	size_t idx() const   {return idx_;}
	size_t count() const {return count_;}
	bool is_found() const {return found_;}
};


template <typename T>
SelectResult Select1FW(const T* buffer, size_t start, size_t stop, size_t count)
{
	size_t bitsize 	= TypeBitsize<T>();
	size_t mask 	= TypeBitmask<T>();
	size_t divisor 	= TypeBitmaskPopCount(mask);

	size_t prefix 	= bitsize - (start & mask);

	size_t suffix;
	size_t start_cell;
	size_t stop_cell;

	if (start + prefix >= stop)
	{
		prefix 		= stop - start;
		suffix 		= 0;
		start_cell  = 0;
		stop_cell 	= 0;
	}
	else
	{
		suffix 		= stop & mask;
		start_cell	= (start + prefix) >> divisor;
		stop_cell	= stop >> divisor;
	}

	size_t total = 0;

	if (intrnl2::SelectFW(GetBits0(buffer, start, prefix), total, count, prefix))
	{
		return SelectResult(start + prefix, total, true);
	}

	for (size_t cell = start_cell; cell < stop_cell; cell++)
	{
		if (intrnl2::SelectFW(buffer[cell], total, count, bitsize))
		{
			return SelectResult((cell << divisor) + bitsize, total, true);
		}
	}

	if (suffix > 0)
	{
		size_t start0 = stop_cell << divisor;
		intrnl2::SelectFW(GetBits0(buffer, start0, suffix), total, count, suffix);
		return SelectResult(suffix + start0, total, true);
	}
	else {
		return SelectResult(stop, total, false);
	}
}

template <typename T>
SelectResult Select0FW(const T* buffer, size_t start, size_t stop, size_t count)
{
	size_t bitsize 	= TypeBitsize<T>();
	size_t mask 	= TypeBitmask<T>();
	size_t divisor 	= TypeBitmaskPopCount(mask);

	size_t prefix 	= bitsize - (start & mask);

	size_t suffix;
	size_t start_cell;
	size_t stop_cell;

	if (start + prefix >= stop)
	{
		prefix 		= stop - start;
		suffix 		= 0;
		start_cell  = 0;
		stop_cell 	= 0;
	}
	else
	{
		suffix 		= stop & mask;
		start_cell	= (start + prefix) >> divisor;
		stop_cell	= stop >> divisor;
	}

	size_t total = 0;

	if (intrnl2::SelectFW(GetBitsNeg0(buffer, start, prefix), total, count, prefix))
	{
		return SelectResult(start + prefix, total, true);
	}

	for (size_t cell = start_cell; cell < stop_cell; cell++)
	{
		if (intrnl2::SelectFW(~buffer[cell], total, count, bitsize))
		{
			return SelectResult((cell << divisor) + bitsize, total, true);
		}
	}

	if (suffix > 0)
	{
		size_t start0 = stop_cell << divisor;
		intrnl2::SelectFW(GetBitsNeg0(buffer, start0, suffix), total, count, suffix);
		return SelectResult(suffix + start0, total, true);
	}
	else {
		return SelectResult(stop, total, false);
	}
}




template <typename T>
SelectResult Select1BW(const T* buffer, size_t start, size_t stop, size_t count)
{
	size_t bitsize 	= TypeBitsize<T>();
	size_t mask 	= TypeBitmask<T>();
	size_t divisor 	= TypeBitmaskPopCount(mask);

	size_t prefix 	= start & mask;

	size_t suffix;
	size_t start_cell;
	size_t stop_cell;

	if (start - prefix <= stop)
	{
		prefix 		= start - stop;
		suffix 		= 0;
		start_cell  = 0;
		stop_cell 	= 0;
	}
	else
	{
		size_t padding  = stop & mask;

		suffix 		= bitsize - padding;
		stop_cell	= stop >> divisor;
		start_cell	= ((start - prefix) >> divisor) - 1;
	}

	size_t total = 0;
	size_t delta;

	if (intrnl2::SelectBW(GetBits0(buffer, start, prefix), total, count, prefix, delta))
	{
		return SelectResult(start - delta, total, true);
	}

	for (size_t cell = start_cell; cell > stop_cell; cell--)
	{
		if (intrnl2::SelectBW(buffer[cell], total, count, bitsize - 1, delta))
		{
			return SelectResult((cell << divisor) + bitsize - delta, total, true);
		}
	}

	if (suffix > 0)
	{
		intrnl2::SelectBW(GetBits0(buffer, stop, suffix), total, count, suffix, delta);
		return SelectResult(stop + (bitsize - delta), total);
	}
	else {
		return SelectResult(stop, total, false);
	}
}



} //memoria

#endif
