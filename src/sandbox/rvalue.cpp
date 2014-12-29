
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/md5.hpp>
#include <memoria/core/types/typehash.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


template<typename = void>
struct Object {

    Object() {}

    Object(Object&&) {}
    Object(const Object&) {}

    Object& operator=(const Object& other) {

        Assign(*this, other);

        return *this;
    }

    Object& operator=(Object&& other) {

        Assign(*this, std::forward<Object<>>(other));

        return *this;
    }
};

template <typename T>
void Assign(Object<>& obj, const T& other) {}

//template <typename T>
//void Assign(Object<>& obj, T&& other) {}

template <typename T>
void Assign(Object<>& obj, T& other) {

}

Object<> boo() {
    return Object<>();
}

int main(void) {

    Object<> o1, o2;

    o1 = o2;

    o1 = boo();

    return 0;
}

