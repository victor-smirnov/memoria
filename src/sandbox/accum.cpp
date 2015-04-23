
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
using namespace memoria::list_tree;
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
		TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>, PkdStruct<Int, 2>>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>,

		PkdStruct<Int, 4>
>;

using IdxList = TypeList<
		TL<
			TL<IndexRange<0, 1>, IndexRange<2, 4>>,
			TL<IndexRange<0, 2>>,
			TL<IndexRange<0, 2>>
		>,
		TL<IndexRange<0, 3>, IndexRange<3, 5>, IndexRange<6, 12>, IndexRange<13, 17>>,
		TL<>,
		TL<IndexRange<0, 1>, IndexRange<1, 2>>
>;


using RangeListType = BranchNodeRangeListBuilder<
		BranchStructList,
		LeafStructList,
		IdxList
>::Type;

using RangeOffsetListType = BranchNodeRangeListBuilder<
		BranchStructList,
		LeafStructList,
		IdxList
>::OffsetList;


using AccType = IteratorAccumulatorBuilder<
		BranchStructList,
		RangeListType
>::Type;


using AccumTuple = TupleBuilder<Linearize<AccType>>::Type;

int main() {
	ListPrinter<RangeListType>::print(cout);
	cout<<"Accum:"<<endl;
	ListPrinter<AccType>::print(cout);
	cout<<"AccumTuple:"<<endl;
	ListPrinter<TL<AccumTuple>>::print(cout);

	cout<<"LeafRangeList:"<<endl;
	ListPrinter<TL<IdxList>>::print(cout);


	using AccumItemH = AccumItem<TL<LeafStructList>, IntList<0, 0>, AccumTuple>;

	cout<<"RangeOffsetList:"<<endl;
	TypesPrinter<
		RangeOffsetListType,
		IntValue<AccumItemH::BranchIndex>
	>::print(cout);



	AccumTuple accum;

	try {
		AccumItemH::template item<6>(accum)[0] = 123;

		TypesPrinter<decltype(AccumItemH::template item<6>(accum))>::print(cout);

		Int index = 13;
		AccumItemH::value(index, accum) = 12345;
		cout<<"AccumItem = "<<AccumItemH::value(index, accum)<<endl;
	}
	catch (BoundsException& ex) {
		cout<<ex.message()<<endl;
	}

	using Tuple = std::tuple<int, int, int>;

	cout<<StreamTupleHelper<Tuple>::convert()<<endl;

	cout<<StreamTupleHelper<Tuple>::convertTupleAll(std::make_tuple(1, 5, 6.6))<<endl;



	using LeafStructList2 = TL<

			TypeList<
				TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>>,

				PkdStruct<Int, 10>,

				TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>>,

				PkdStruct<Int, 11>,

				TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>, PkdStruct<Int, 2>>,

				PkdStruct<Int, 17>,

				PkdStruct<Int, 4>,

				PkdStruct<Int, 4>
			>,

			TypeList<
				PkdStruct<Int, 11>, //12 //br:8

				TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>>,

				PkdStruct<Int, 11>,

				TL<PkdStruct<Int, 4>, PkdStruct<Int, 2>, PkdStruct<Int, 2>>,

				PkdStruct<Int, 17>
			>
	>;

	const Int LeafIdx = 13;

	cout<<"BranchIndex: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::BranchStructIdx<<endl;
	cout<<"Offset: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::LeafOffset<<endl;
	cout<<"ISStart: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::IsStreamStart<<endl;
}


