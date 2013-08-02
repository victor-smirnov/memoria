
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_
#define MEMORIA_TESTS_SEQUENCE_SEQUENCE_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "sequence_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <Int BitsPerSymbol, bool Dense = true>
class SequenceCreateTest: public SequenceTestBase<BitsPerSymbol, Dense> {

	typedef SequenceCreateTest<BitsPerSymbol, Dense> 							MyType;
	typedef SequenceTestBase<BitsPerSymbol, Dense> 								Base;

	typedef typename Base::Allocator											Allocator;
	typedef typename Base::Iterator												Iterator;
	typedef typename Base::Ctr													Ctr;

	static const Int Symbols													= Base::Symbols;

public:

	SequenceCreateTest(StringRef name): Base(name)
	{
		MEMORIA_ADD_TEST(testCreateRemoveRandom);
		MEMORIA_ADD_TEST(testAppend);
	}

	void testCreateRemoveRandom()
	{
		Allocator allocator;

		Ctr ctr(&allocator);

		allocator.commit();

		try {
			for (Int c = 0; c < this->size_; c++)
			{
				Int bit1 = getRandom(Symbols);
				Int idx  = getRandom(c + 1);

				this->out()<<c<<" Insert: "<<bit1<<" at "<<idx<<endl;

				ctr.insert(idx , bit1);

				auto iter = ctr.seek(idx);

				Int bit2 = iter.symbol();

				AssertEQ(MA_SRC, iter.pos(), idx);
				AssertEQ(MA_SRC, bit1, bit2);
			}

			AssertEQ(MA_SRC, ctr.size(), this->size_);

			allocator.commit();

			this->StoreAllocator(allocator, this->getResourcePath("create.dump"));

			BigInt size = ctr.size();

			for (Int c = 0; c < size; c++)
			{
				Int idx = getRandom(size - c);

				ctr.remove(idx);

				AssertEQ(MA_SRC, ctr.size(), size - c - 1);
			}

			allocator.commit();

			this->StoreAllocator(allocator, this->getResourcePath("remove.dump"));
		}
		catch (...) {
			Base::dump_name_ = Base::Store(allocator);
			throw;
		}
	}

	void testAppend()
	{
		Allocator allocator;

		Ctr ctr(&allocator);

		allocator.commit();

		try {

			auto seq = Base::fillRandom(ctr, this->size_);

			allocator.commit();

			this->StoreAllocator(allocator, this->getResourcePath("append.dump"));

			Int cnt = 0;
			for (auto i = ctr.Begin(); !i.isEof(); i++, cnt++)
			{
				Int symbol1 = i.symbol();
				Int symbol2 = seq[cnt];

				AssertEQ(MA_SRC, symbol1, symbol2);
			}

			AssertEQ(MA_SRC, cnt, this->size_);

			for (Int c = 0; c < this->size_; c++)
			{
				Int symbol1 = ctr.seek(c).symbol();
				Int symbol2 = seq[c];

				AssertEQ(MA_SRC, symbol1, symbol2);
			}
		}
		catch (...) {
			Base::dump_name_ = Base::Store(allocator);
			throw;
		}
	}
};



}


#endif
