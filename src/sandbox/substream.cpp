


#include <memoria/prototypes/bt/bt_packed_struct_list_builder.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;
using namespace memoria::bt::internal;

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


int main(void) {

	cout<<TypeNameFactory<List>::name()<<endl;

	cout<<SubstreamsTreeSize<List>::Size<<endl;

	cout<<LeafOffsetCount<List, IntList<3>>::Size<<endl;

    return 0;
}

