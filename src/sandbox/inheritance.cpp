
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

using namespace std;

struct Base {
    int base_ = 1;
    virtual void base() = 0;

};


struct Base1: Base {

    int field_ = 100;

    virtual void boo1() = 0;
    virtual void foo() = 0;

    virtual void non_foo() {
        cout<<"non_foo 1"<<endl;
    }
};

struct Base2: Base {
    int field_ = 200;

    virtual void boo2() = 0;
    virtual void foo() = 0;

    virtual void non_foo() {
        cout<<"non_foo 1"<<endl;
    }
};


struct Derived: public Base1, public Base2 {
    virtual void boo1()
    {
        cout<<"boo1"<<endl;
    }

    virtual void boo2()
    {
        cout<<"boo2"<<endl;
    }

    virtual void foo()
    {
        cout<<"foo"<<endl;
    }

    virtual void base()
    {
        Base1::base_ = 2;
        Base2::base_ = 3;

        cout<<"base "<<Base1::base_<<" "<<Base2::base_<<endl;
    }
};

void doWork1(Base1& base) {
    base.boo1();
    base.foo();
    base.base();
}

void doWork2(Base2& base) {
    base.boo2();
    base.foo();
    base.base();
}

int main()
{
    Derived obj;

    doWork1(obj);
    cout<<endl;
    doWork2(obj);

    obj.base();
//  obj.non_foo();
}
