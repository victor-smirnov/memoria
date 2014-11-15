


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_leaf_offset_count.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;

typedef TypeList<
			IntValue<10>,
			IntValue<20>,
			TypeList<
				IntValue<1>,
				IntValue<2>,
				TypeList<
					IntValue<2>,
					TypeList<
						IntValue<3>
					>,
					IntValue<4>
				>,
				IntValue<5>
			>,
			IntValue<5>
		> 																		List;


using namespace memoria::bt::internal;

class T {};

typedef TypeList<
		SubstreamSizeListBuilder<T>::Type,
		SubstreamSizeListBuilder<TypeList<T, T, T, T>>::Type,
		SubstreamSizeListBuilder<TypeList<TypeList<T>, T, T, T>>::Type,
		SubstreamSizeListBuilder<TypeList<T, T, TypeList<T>, T, T, T, TypeList<T>, T, T>>::Type,
		SubstreamSizeListBuilder<TypeList<T, TypeList<T>, T, T, TypeList<T>, T>>::Type,

		SubstreamSizeListBuilder<TypeList<TypeList<T>>>::Type,

		SubstreamSizeListBuilder<TypeList<TypeList<T>, T>>::Type,

		SubstreamSizeListBuilder<TypeList<T, TypeList<T, TypeList<T>>>>::Type
> SList;

int main(void) {

	cout<<TypeNameFactory<List>::name()<<endl;

	cout<<SubstreamsTreeSize<List>::Size<<endl;

	cout<<LeafOffsetCount<List, IntList<3>>::Value<<endl;

	ListPrinter<SList>::print(cout);

    return 0;
}

