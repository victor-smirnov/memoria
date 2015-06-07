
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <iostream>
#include <typeinfo>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/list/reverse.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <iostream>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

template <typename T>
struct IsInt {
    static const bool Value = false;
};

template <>
struct IsInt<Int> {
    static const bool Value = true;
};

template <typename T, typename = void> struct TCtr;

template <bool V>
using Predicate = typename std::enable_if<V>::type;

template <typename T>
using IsIntCtr = Predicate<IsInt<T>::Value>;

template <typename T>
struct TCtr<T, IsIntCtr<T> > {

};


template <typename T, typename = IsIntCtr<T>>
void boo(T value) {
    cout<<value<<endl;
}

int main()
{
    TCtr<Int> c1;
//  TCtr<Long> c2; // will not compile

    boo(1);

    return sizeof(c1);
}


