
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_LOUDSTREE_HPP
#define MEMORIA_CORE_TOOLS_LOUDSTREE_HPP

#include <memoria/core/tools/symbol_sequence.hpp>

namespace memoria {

class LoudsTree: public SymbolSequence<1> {

	typedef LoudsTree 															MyType;
	typedef SymbolSequence<1>													Base;

public:
	static const size_t END														= static_cast<size_t>(-1);

public:

	LoudsTree(): Base()
	{}

	LoudsTree(size_t capacity): Base(capacity)
	{}

	LoudsTree(const MyType& other): Base(other)
	{}

	LoudsTree(MyType&& other): Base(other)
	{}

	size_t parent(size_t idx)
	{
		size_t rank = rank0(idx);
		return select1(rank);
	}

	size_t firstChild(size_t idx)
	{
		size_t rank = rank1(idx);
		size_t idx1 = select0(rank) + 1;

		if ((*this)[idx1] == 1)
		{
			return idx1;
		}
		else {
			return END;
		}
	}

	size_t lastChild(size_t idx)
	{
		size_t rank = rank1(idx) + 1;
		size_t idx1 = select0(rank) - 1;

		if ((*this)[idx1] == 1)
		{
			return idx1;
		}
		else {
			return END;
		}
	}

	size_t nextSibling(size_t idx)
	{
		if ((*this)[idx + 1] == 1)
		{
			return idx + 1;
		}
		else {
			return END;
		}
	}

	size_t prevSibling(size_t idx)
	{
		if ((*this)[idx - 1] == 1)
		{
			return idx - 1;
		}
		else {
			return END;
		}
	}

	size_t appendUDS(size_t value)
	{
		size_t idx = Base::size();
		Base::enlarge(value + 1);

		writeUDS(idx, value);

		return value + 1;
	}

	size_t writeUDS(size_t idx, size_t value)
	{
		size_t max = idx + value;

		for (; idx < max; idx++)
		{
			(*this)[idx] = 1;
		}

		(*this)[idx++] = 0;

		return idx;
	}

	size_t select1(size_t rank)
	{
		return select(rank, 1);
	}

	size_t select0(size_t rank)
	{
		return select(rank, 0);
	}

	size_t select(size_t rank, Int symbol)
	{
		SelectResult result = Base::sequence_->selectFW(symbol, rank);
		return result.is_found() ? result.idx() : END;
	}

	size_t rank1(size_t idx)
	{
		return Base::sequence_->rank1(idx + 1, 1);
	}

	size_t rank0(size_t idx)
	{
		return Base::sequence_->rank1(idx + 1, 0);
	}
};


}


#endif
