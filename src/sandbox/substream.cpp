
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


using List = TL<
		TL<
			TL<
				S<5>,
				TL<
					S<2>,
					S<3>,
					S<7>
				>
			>,
			TL<S<4>>
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
	using Leafs = Linearize<List, 2>;

	cout<<"Leaf Offsets:"<<endl;
	using LeafOffsets 	= typename LeafOffsetListBuilder<List>::Type;

	using LeafPath = IntList<1, 0, 1>;

	constexpr Int LeafIdx = LeafCount<List, LeafPath>::Value;

	constexpr Int LocalOffset 	= FindLocalLeafOffsetV<Leafs, LeafIdx>::Value;
	using LocalLeafGroup 		= FindLocalLeafOffsetT<LeafOffsets, LeafIdx>::Type;

	cout<<"LocalOffset: "<<LocalOffset<<endl;
	ListPrinter<TL<LocalLeafGroup>>::print(cout);

	cout<<"LeafPrefix: "<<GetLeafPrefix<LocalLeafGroup, LocalOffset>::Value<<endl;

//	ListPrinter<TL<S<FindTopLevelIdx<List, 3>::Value>>>::print(cout);

    return 0;
}

