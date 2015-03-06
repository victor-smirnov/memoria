
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/prototypes/bt/tools/bt_accumulators.hpp>


#include <iostream>

using namespace memoria;
using namespace memoria::bt;
using namespace std;

class T{};

template <typename T, Int Indexes_>
struct PkdStruct {
	static const Int Indexes = Indexes_;
};

using StructList = TypeList<
		PkdStruct<Int, 8>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>
>;

using IdxList = TypeList<
		TL<IndexRange<0, 3>, IndexRange<4, 6>, IndexRange<7>>,
		TL<IndexRange<0, 3>, IndexRange<4, 6>, IndexRange<16>>,
		TL<IndexRange<0, 3>>
>;

using Type = AccumListBuilderH<StructList, IdxList>::Type;


int main() {
	ListPrinter<Type>::print(cout);
}


