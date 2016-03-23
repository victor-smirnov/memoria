
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



#include <memoria/v1/core/types/list/sublist.hpp>
#include <memoria/v1/core/types/typelist.hpp>

#include <iostream>

using namespace memoria;
using namespace std;

class T{};

using List = TypeList<
        SublistFromStart<TypeList<>, 0>::Type,
        SublistFromStart<TypeList<T>, 0>::Type,
        SublistFromStart<TypeList<T, T, T, T>, 0>::Type,
        SublistFromStart<TypeList<T, T, T, T>, 1>::Type,
        SublistFromStart<TypeList<T, T, T, T>, 2>::Type,
        SublistFromStart<TypeList<T, T, T, T>, 3>::Type,
        SublistFromStart<TypeList<T, T, T, T>, 4>::Type,

        SublistToEnd<TypeList<>, 0>::Type,
        SublistToEnd<TypeList<T>, 0>::Type,
        SublistToEnd<TypeList<T>, 1>::Type,
        SublistToEnd<TypeList<T, T, T, T>, 0>::Type,
        SublistToEnd<TypeList<T, T, T, T>, 1>::Type,
        SublistToEnd<TypeList<T, T, T, T>, 2>::Type,
        SublistToEnd<TypeList<T, T, T, T>, 3>::Type,
        SublistToEnd<TypeList<T, T, T, T>, 4>::Type,

        Sublist<TypeList<>, 0, 0>::Type,
        Sublist<TypeList<T>, 0, 0>::Type,
        Sublist<TypeList<T>, 0, 1>::Type,
        Sublist<TypeList<T>, 1, 1>::Type,


        Sublist<TypeList<T, T, T, T>, 0, 4>::Type,
        Sublist<TypeList<T, T, T, T>, 1, 1>::Type,
        Sublist<TypeList<T, T, T, T>, 2, 2>::Type,
        Sublist<TypeList<T, T, T, T>, 3, 4>::Type,
        Sublist<TypeList<T, T, T, T>, 4, 4>::Type
>;


using List2 = TypeList<
        IntValue<ListDepth<T>::Value>,
        IntValue<ListDepth<TypeList<>>::Value>,
        IntValue<ListDepth<TypeList<T, T, T>>::Value>,
        IntValue<ListDepth<TypeList<TypeList<T>, T, T>>::Value>,
        IntValue<ListDepth<TypeList<TypeList<T>, T, TypeList<T>>>::Value>,
        IntValue<ListDepth<TypeList<TypeList<T>, T, TypeList<TypeList<T>>>>::Value>,
        IntValue<ListDepth<TypeList<TypeList<T>, TypeList<TypeList<TypeList<T>>>, TypeList<TypeList<T>>>>::Value>
>;

using Tests = IntList<
        ListDepth<TypeList<TypeList<T>>>::Value
>;


using List3 = TypeList<
        Linearize<TL<T, TL<TL<T>>, T>, 1>,
        Linearize<TL<T, T, TL<TL<T>>>, 1>,
        Linearize<TL<TL<TL<TL<T>>>, T, TL<TL<T>>>, 3>,

        Linearize<TL<TL<T>, T, TL<T>>, 1>
>;


int main() {
    ListPrinter<Tests>::print(cout);

    ListPrinter<List3>::print(cout);
}
