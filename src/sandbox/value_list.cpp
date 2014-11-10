


#include <memoria/core/types/list/append.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


typedef IntList<1,2,3,4,5> SrcList;

typedef MergeValueLists<SrcList, IntList<6, 7, 8>>::Result List1;

typedef MergeValueLists<ValueList<Int, 6, 7, 8>>::Result List2;

//typedef MergeValueLists<>::Result List0;

typedef MergeValueLists<IntValue<5>>::Result List3;
typedef MergeValueLists<IntValue<5>, IntValue<6>>::Result List4;

typedef MergeValueLists<IntValue<5>, IntValue<6>, IntValue<7>>::Result List5;

typedef MergeValueLists<IntList<4>, IntValue<5>>::Result List6;
typedef MergeValueLists<IntList<4>, IntValue<5>, IntValue<6>>::Result List7;

typedef MergeValueLists<IntList<4>, IntValue<5>, IntList<6>>::Result List8;

typedef MergeValueLists<IntValue<5>, IntList<7, 8, 9>, IntValue<10>>::Result List9;

int main(void) {

	cout<<TypeNameFactory<SrcList>::name()<<endl;

	cout<<TypeNameFactory<List1>::name()<<endl;
	cout<<TypeNameFactory<List2>::name()<<endl;

	cout<<TypeNameFactory<List3>::name()<<endl;
	cout<<TypeNameFactory<List4>::name()<<endl;
	cout<<TypeNameFactory<List5>::name()<<endl;

	cout<<TypeNameFactory<List6>::name()<<endl;
	cout<<TypeNameFactory<List7>::name()<<endl;
	cout<<TypeNameFactory<List8>::name()<<endl;
	cout<<TypeNameFactory<List9>::name()<<endl;

    return 0;
}

