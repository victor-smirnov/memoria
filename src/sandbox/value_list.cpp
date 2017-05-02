
// Copyright 2014 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;


typedef IntList<1,2,3,4,5> SrcList;

typedef MergeValueLists<SrcList, IntList<6, 7, 8>>::Type List1;

typedef MergeValueLists<ValueList<int32_t, 6, 7, 8>>::Type List2;

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
