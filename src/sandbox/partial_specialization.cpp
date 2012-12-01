/*
 * partial_specialization.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */

#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


template <int Value>
struct Declarator {
    typedef NotDefined Type;
};

template <>
struct Declarator<0> {
    typedef int Type;
};

template <>
struct Declarator<1> {
    typedef long Type;
};


template <
    template <int> class Decl,
    int Value = 100,
    typename List = TypeList<>
>
class Builder {
    typedef typename Decl<Value>::Type                          DeclType;

    typedef typename IfThenElse<
        IfTypesEqual<DeclType, NotDefined>::Value,
        List,
        typename AppendTool<DeclType, List>::Result
    >::Result                                                   NewList;

public:
    typedef typename Builder<Decl, Value - 1, NewList>::Type    Type;
};

template <
    template <int> class Decl,
    typename List
>
class Builder<Decl, -1, List> {
public:
    typedef List Type;
};


typedef Builder<Declarator> DeclBuilder;

//template <typename T>
//class Boo: public DeclBuilder {
//
//};

template <>
struct Declarator<2> {
    typedef bool Type;
};



int main(void) {

    cout<<"Type: "<<TypeNameFactory<DeclBuilder::Type>::name()<<endl;

    return 0;
}

