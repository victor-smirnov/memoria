
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

class Iterator {

    Int i = 0;

public:

    bool operator!=(const Iterator& other) const {
        return i < 10;
    }

    void operator++() {
        i++;
    }

    const Iterator& operator*() const {
        cout<<"A"<<endl;
        return *this;
    }

    Iterator& operator*() {
        cout<<"B"<<endl;
        return *this;
    }

    int cnt() {
        return i;
    }
};

class Container {
public:

    Iterator begin() {
        return Iterator();
    }

    Iterator end() {
        return Iterator();
    }

};


int main(void) {

    Container ctr;

    for (auto& var: ctr) {
        cout<<var.cnt()<<endl;
    }

    return 0;
}

