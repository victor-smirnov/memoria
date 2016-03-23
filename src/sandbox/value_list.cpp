
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;


typedef IntList<1,2,3,4,5> SrcList;

typedef MergeValueLists<SrcList, IntList<6, 7, 8>>::Type List1;

typedef MergeValueLists<ValueList<Int, 6, 7, 8>>::Type List2;

//typedef MergeValueLists<>::Result List0;

typedef MergeValueLists<IntValue<5>>::Type List3;
typedef MergeValueLists<IntValue<5>, IntValue<6>>::Type List4;

typedef MergeValueLists<IntValue<5>, IntValue<6>, IntValue<7>>::Type List5;

typedef MergeValueLists<IntList<4>, IntValue<5>>::Type List6;
typedef MergeValueLists<IntList<4>, IntValue<5>, IntValue<6>>::Type List7;

typedef MergeValueLists<IntList<4>, IntValue<5>, IntList<6>>::Type List8;

typedef MergeValueLists<IntValue<5>, IntList<7, 8, 9>, IntValue<10>>::Type List9;

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

