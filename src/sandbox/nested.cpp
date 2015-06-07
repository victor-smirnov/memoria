
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>

using namespace std;

template <typename Selector>
struct Main {
    typedef int BooType;

    template <int a> struct A {
        static void dump() {
            BooType b;
            cout<<"first "<<b<<endl;
        }
    };
};


template <>
struct Main<int> {
    template <typename BooType> struct A {
        static void dump() {
            BooType b;
            cout<<"third "<<b<<endl;
        }
    };
};


template <>
template <>
struct Main<int>::A<int> {
    static void dump() {
        int b = 0;
        cout<<"second "<<b<<endl;
    }
};


int main(void) {
    Main<int>::A<int>::dump();
}
