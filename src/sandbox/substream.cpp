
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_substreamgroup_dispatcher.hpp>

#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/list_tree.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>


using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;
using namespace memoria::list_tree;

template <int I>
struct S {
	static const Int Value = I;
};

struct T{};


namespace memoria {
namespace bt {

template <Int V>
struct StructSizeProvider<S<V>> {
    static const Int Value = V;
};


}
}


using BranchList = TypeList<
		TL<
			TL<
				S<1>,
				S<2>
			>,
			S<3>
		>,
		TL<
			S<4>,
			S<5>,
			S<6>
		>
>;


using LeafList = TL<
		TL<
			TL<
				S<5>,
				TL<
					S<2>,
					S<3>,
					S<7>
				>
			>,
			S<4>
		>,
		TL<
			TL<
				S<8>,
				S<10>
			>,
			TL<
				TL<
					S<10>,
					S<15>,
					S<7>,
					S<19>
				>,
				S<2>
			>
		>
>;




int main(void)
{
	static_assert(ListSize<Linearize<BranchList>>::Value == ListSize<Linearize<LeafList, 2>>::Value, "");

	TypePrinter<Linearize<BranchList>>::println(cout);
	TypePrinter<Linearize<LeafList, 2>>::println(cout);

	using Leafs = Linearize<LeafList, 2>;

	cout<<"Leaf Offsets:"<<endl;
	using LeafOffsets 	= typename LeafOffsetListBuilder<LeafList>::Type;

	using LeafPath = IntList<0, 1>;

	constexpr Int LeafIdx = LeafCount<LeafList, LeafPath>::Value;

	constexpr Int LocalOffset 	= FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;
	using LocalLeafGroup 		= FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

	cout<<"LocalOffset: "<<LocalOffset<<endl;
	ListPrinter<TL<LocalLeafGroup>>::print(cout);

	cout<<"LeafPrefix: "<<GetLeafPrefix<LocalLeafGroup, LocalOffset>::Value<<endl;


	TypePrinter<BuildTreePath<LeafList, 11>::Type>::println(cout);


	const Int BranchOffset = LeafCountInf<LeafList, LeafPath, 2>::Value - LocalOffset;

	cout<<"BranchOffset "<<BranchOffset<<endl;

	TypePrinter<BuildTreePath<BranchList, BranchOffset>::Type>::println(cout);

//	ListPrinter<TL<S<FindTopLevelIdx<List, 3>::Value>>>::print(cout);

    return 0;
}

