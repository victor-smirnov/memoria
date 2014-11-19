/*
 * sublist.cpp
 *
 *  Created on: Nov 16, 2014
 *      Author: victor
 */

#include <memoria/core/types/list/sublist.hpp>
#include <memoria/core/types/typelist.hpp>

#include <iostream>

using namespace memoria;
using namespace std;

class T{};

typedef TypeList<
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
> List;

int main() {
    ListPrinter<List>::print(cout);
}


