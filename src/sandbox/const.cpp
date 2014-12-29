// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>
#include <unordered_map>


using namespace std;
using namespace memoria::vapi;

struct Foo {
    void m() {
        cout<<"m() "<<endl;
    };
    void m() const {
        cout<<"m() const"<<endl;
    };
};

struct ConstFoo {
    void m() const {
        cout<<"m() const"<<endl;
    };
};


struct Bar {
    mutable int m_ = 0;

    Bar() {}

    void m() {
        cout<<"m()"<<endl;
    }
    void m() const {
        m_ = 2;

        cout<<"m() const"<<endl;
    }

    const ConstFoo foo() const {
        return ConstFoo();
    }

    Foo foo() {
        return Foo();
    }
};


void constFn(const Bar& bar) {
    cout<<TypeNameFactory<decltype(bar.foo())>::name()<<endl;
}



int main(void) {

    Bar b1;
    const Bar b2;

    b1.m();
    b1.m();

    b2.m();
    b2.m();

    ConstFoo foo = b2.foo();

    foo.m();

    typedef unordered_map<int, int> int_map;

    int_map m;

    constFn(b2);

    //cout<<TypeNameFactory<decltype(b2.foo())>::name()<<endl;
    cout<<TypeNameFactory<decltype(begin(m))>::name()<<endl;

    return 0;
}

