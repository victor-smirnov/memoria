
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/types/list/linearize.hpp>
#include <memoria/prototypes/bt/tools/bt_accumulators.hpp>
#include <memoria/prototypes/bt/tools/bt_tools.hpp>



#include <iostream>

using namespace memoria;
using namespace memoria::bt;
using namespace std;

class T{};

template <typename T, Int Indexes_>
struct PkdStruct {
	static const Int Indexes = Indexes_;
};

using BranchStructList = TypeList<
		PkdStruct<Int, 9>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>,

		PkdStruct<Int, 4>
>;


using LeafStructList = TypeList<
		TL<PkdStruct<Int, 4>, PkdStruct<Int, 4>>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>,

		PkdStruct<Int, 4>
>;

using IdxList = TypeList<
		TL<TL<IndexRange<0, 1>, IndexRange<1, 3>>, TL<IndexRange<0, 3>>>,
		TL<IndexRange<0, 3>, IndexRange<3, 6>, IndexRange<6, 12>, IndexRange<13, 16>>,
		TL<>,
		TL<IndexRange<0, 1>, IndexRange<1, 2>>
>;


using RangeListType = BranchNodeRangeListBuilder<
		BranchStructList,
		LeafStructList,
		IdxList
>::Type;


using AccType = IteratorAccumulatorBuilder<
		BranchStructList,
		RangeListType
>::Type;


using AccumTuple = TupleBuilder<AccType>::Type;

int main() {
	ListPrinter<RangeListType>::print(cout);
	cout<<"Accum:"<<endl;
	ListPrinter<AccType>::print(cout);
	cout<<"AccumTuple:"<<endl;
	ListPrinter<TL<AccumTuple>>::print(cout);

	AccumTuple accum;

	try {
		AccumItem<LeafStructList, IdxList, IntList<0>, AccumTuple>::value(9, accum) = 12345;
		cout<<"AccumItem = "<<AccumItem<LeafStructList, IdxList, IntList<0>, AccumTuple>::value(0, accum)<<endl;
	}
	catch (BoundsException& ex) {
		cout<<ex.message()<<endl;
	}
}


