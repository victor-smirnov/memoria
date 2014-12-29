
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
struct S {};

struct T{};

using List = TL<
		TL<
			TL<T>,
			TL<T>
		>,
		TL<
			TL<T>,
			TL<T>
		>
>;

int main(void)
{
	ListPrinter<List>::print(cout);

//	ListPrinter<TL<
//		MakeValueList<Int, 100, 200>
//	>>::print(cout);

//	ListPrinter<BuildTopLevelLeafSubsets<List>>::print(cout);

	ListPrinter<TL<S<FindTopLevelIdx<List, 4>::Value>>>::print(cout);

    return 0;
}

