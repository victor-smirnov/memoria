/*
 * vlist.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */


#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;










int main(void) {

	typedef VTL<int, float, bool> List0;

	cout<<TypeNameFactory<List0>::name()<<endl;
	cout<<TypeNameFactory<ListHead<List0>::Type>::name()<<endl;

	cout<<TypeNameFactory<AppendTool<double, List0>::Result>::name()<<endl;
	cout<<TypeNameFactory<AppendTool<List0, double>::Result>::name()<<endl;
	cout<<TypeNameFactory<AppendTool<List0, VTL<double, unsigned> >::Result>::name()<<endl;
	cout<<TypeNameFactory<AppendTool<VTL<double, unsigned>, List0 >::Result>::name()<<endl;

	cout<<TypeNameFactory<RemoveTool<int, List0>::Result>::name()<<endl;

	cout<<TypeNameFactory<RemoveTool<int, AppendTool<int, List0>::Result, true>::Result>::name()<<endl;

	cout<<IndexOfTool<int, List0>::Value<<endl;
	cout<<IndexOfTool<bool, AppendTool<List0, double>::Result>::Value<<endl;
	cout<<IndexOfTool<double, AppendTool<List0, double>::Result>::Value<<endl;
	cout<<IndexOfTool<unsigned, AppendTool<List0, double>::Result>::Value<<endl;

	cout<<TypeNameFactory<RemoveDuplicatesTool<AppendTool<AppendTool<VTL<int, bool>, List0>::Result, double>::Result>::Result>::name()<<endl;


	cout<<TypeNameFactory<RevertList<AppendTool<List0, double>::Result>::Type>::name()<<endl;

	cout<<TypeNameFactory<AppendTool<VTL<int, float>, VTL<bool, double>>::Result>::name()<<endl;

	return 0;
}
