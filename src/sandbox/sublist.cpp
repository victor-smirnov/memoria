
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/types/list/sublist.hpp>
#include <memoria/core/types/typelist.hpp>

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


