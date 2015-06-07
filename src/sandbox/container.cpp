
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/types/typelist.hpp>

#include <iostream>

//#if defined(MIN)
//#include <cxxabi.h>

using namespace std;
using namespace memoria;


template <typename Name, typename Base, typename Types, template <typename> class Type> class CtrPart;

template <typename Types> class Ctr;
template <typename Types> class Iter;

template <int Idx, typename Types, template <typename> class Type>
class CtrHelper: public CtrPart<
                            typename SelectByIndexTool<Idx, typename Types::List>::Result, CtrHelper<Idx - 1, Types, Type>,
                            Types,
                            Type
                        >
{
    typedef Type<Types> MyType;
    typedef CtrHelper<Idx, Types, Type> ThisType;
    typedef CtrPart<
                typename SelectByIndexTool<Idx, typename Types::List>::Result, CtrHelper<Idx - 1, Types, Type>,
                Types,
                Type
            >                                                                       BaseType;

public:
    CtrHelper(MyType& me): BaseType(me) {}
};

template <typename Types, template <typename> class Type>
class CtrHelper<-1, Types, Type>: public Types::template BaseFactory<Types, Type>::Type {
    typedef Type<Types> MyType;
    typedef CtrHelper<-1, Types, Type> ThisType;

    typedef typename Types::template BaseFactory<Types, Type>::Type BaseType;

public:
    CtrHelper(MyType& me): BaseType(me) {}
};


template <typename Types, template <typename> class Type>
class CtrStart: public CtrHelper<ListSize<typename Types::List>::Value - 1, Types, Type> {
    typedef Type<Types> MyType;

    typedef CtrHelper<ListSize<typename Types::List>::Value - 1, Types, Type> Base;
public:
    CtrStart(MyType& me): Base(me) {}
};


template <typename Types>
class Ctr: public CtrStart<Types, Ctr> {
    typedef CtrStart<Types, Ctr>            Base;
public:
    Ctr(): Base(*this) {}
};


template <typename Types>
class Iter: public CtrStart<Types, Iter> {
    typedef Iter<Types>                     MyType;
    typedef CtrStart<Types, Iter>           Base;
public:
    Iter(Ctr<typename Types::CtrTypes>& ctr): Base(*this) {}
};



template <typename Types>  struct CtrTypesBridge: Types {

    typedef typename Types::CtrList List;

    template <typename Types_, template<typename> class Type_> struct BaseFactory {
        typedef typename Types::template CtrBaseFactory<Types_, Type_>::Type Type;
    };
};

template <typename Types>  struct IterTypesBridge: Types {

    typedef typename Types::IterList List;

    template <typename Types_, template<typename> class Type_> struct BaseFactory {
        typedef typename Types::template IterBaseFactory<Types_, Type_>::Type Type;
    };
};







class Name1{};
class Name2{};
class Name3{};
class Name4{};

typedef TLTool<Name3, Name2, Name1>::List CtrListType;
typedef TLTool<Name4>::List                 IterListType;



template <typename Base, typename Types, template <typename> class Type>
class CtrPart<Name1, Base, Types, Type>: public Base {
    typedef Type<Types> MyType;
    typedef Iter<typename Types::IterTypes> IterType;

    MyType& me_;
public:
    CtrPart(MyType& me): Base(me), me_(me) {}

    IterType iter() {
        return IterType(me_);
    }

    void boo1() {
        cout<<"Boo1"<<endl;
        me()->boo2();
        me()->boo3();

        me()->boo4();
    }
};


template <typename Base, typename Types, template <typename> class Type>
class CtrPart<Name2, Base, Types, Type>: public Base {
    typedef Type<Types> MyType;
    MyType& me_;
public:
    CtrPart(MyType& me): Base(me), me_(me) {}

    void boo2() {
        cout<<"Boo2 "<<endl;
    }
};

template <typename Base, typename Types, template <typename> class Type>
class CtrPart<Name3, Base, Types, Type>: public Base {
    typedef Type<Types> MyType;
    MyType& me_;
public:
    CtrPart(MyType& me): Base(me), me_(me) {}

    void boo3() {
        cout<<"Boo3 "<<endl;
    }
};


template <typename Base, typename Types, template <typename> class Type>
class CtrPart<Name4, Base, Types, Type>: public Base {
    typedef Type<Types> MyType;
    MyType& me_;
public:
    CtrPart(MyType& me): Base(me), me_(me) {}

    void boo1() {
        cout<<"Boo1 "<<endl;
    }
};


template <typename Types, template <typename> class Type>
class CtrBase {
    typedef Type<Types> MyType;
    MyType& me_;

public:
    CtrBase(MyType& me): me_(me) {}

    void boo4() {
        cout<<"Boo4"<<endl;
    }
};


template <typename Types, template <typename> class Type>
class IterBase {
    typedef Type<Types> MyType;
    MyType& me_;

public:
    IterBase(MyType& me): me_(me) {}

    void boo4() {
        cout<<"Iter4"<<endl;
    }
};


struct Types {
    typedef CtrListType     CtrList;
    typedef IterListType    IterList;

    template <typename Types_, template<typename> class Type_> struct CtrBaseFactory {
        typedef CtrBase<Types_, Type_> Type;
    };

    template <typename Types_, template<typename> class Type_> struct IterBaseFactory {
        typedef IterBase<Types_, Type_> Type;
    };

    typedef CtrTypesBridge<Types>   CtrTypes;
    typedef IterTypesBridge<Types>  IterTypes;
};


int main(void) {

    Ctr<Types::CtrTypes> ctr;
    ctr.boo1();

//  auto iter = ctr.iter();
//  iter.boo1();

    const int BufferSize = 40960;

//  char buf[BufferSize];
//  size_t len = sizeof(buf);
//  abi::__cxa_demangle("_ZN7CtrPartI5Name19CtrHelperILi1E14CtrTypesBridgeI5TypesE3CtrES4_S5_EC2ERS5_IS4_E", buf, &len, NULL);
//  cout<<buf<<endl;

    return 0;
}
