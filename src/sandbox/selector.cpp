#include <iostream>
#include <typeinfo>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/list/reverse.hpp>

#include <memoria/core/types/list_printer.hpp>

using namespace memoria;

using namespace memoria::vapi;

//using namespace std;
//
//template<bool C, typename T = void>
//struct enable_if {
//  typedef T type;
//};
//
//template <typename T>
//struct enable_if<false, T> { };
//
//template<typename, typename>
//struct is_same {
//    static bool const value = false;
//};
//
//template<typename A>
//struct is_same<A, A> {
//    static bool const value = true;
//};
//
//template<typename B, typename D>
//struct is_base_of {
//    static D * create_d();
//    static char (& chk(B *))[1];
//    static char (& chk(...))[2];
//    static bool const value = sizeof chk(create_d()) == 1 && !is_same<B volatile const, void volatile const>::value;
//};
//
//struct SomeTag { };
//struct InheritSomeTag : SomeTag { };
//
//struct InheritSomeTag2: InheritSomeTag {};
//
//template<typename T, typename S = void>
//struct MyClass { /* T not derived from SomeTag */ };
//
//template<typename T>
//struct MyClass<T, typename enable_if<is_base_of<SomeTag, T>::value>::type> {
//    typedef int isSpecialized;
//};
//
////template<typename T>
////struct MyClass<T, typename enable_if<is_base_of<InheritSomeTag, T>::value>::type> {
////    typedef float isSpecialized;
////};
////
////template<typename T>
////struct MyClass<T, typename enable_if<is_base_of<InheritSomeTag2, T>::value>::type> {
////    typedef double isSpecialized;
////};
//
//int main() {
//    MyClass<SomeTag>::isSpecialized test1;        /* ok */
//    MyClass<InheritSomeTag>::isSpecialized test2; /* ok */
//    MyClass<InheritSomeTag2>::isSpecialized test3;
//
////    std::cout<<"IsBaseOf "<<is_base_of<InheritSomeTag, SomeTag>::value<<std::endl;
//
//    std::cout<<typeid(test1).name()<<std::endl;
//    std::cout<<typeid(test2).name()<<std::endl;
//    std::cout<<typeid(test3).name()<<std::endl;
//
//}

template <typename TSpec = void>
class SomeTag { };

// Type tag, NOT part of the inheritance chain
template <typename TSpec = void>
struct InheritSomeTag { };

template <typename TSpec = void>
struct InheritSomeTag2 { };

// Derived class, uses type tag
template <typename TSpec>
class SomeTag<InheritSomeTag<TSpec> > { };

// Derived class, uses type tag
template <typename TSpec>
class SomeTag<InheritSomeTag<InheritSomeTag2<TSpec> > > { };

template <class T, class Tag=T  >
struct MyClass { };

template <class T, typename TSpec>
struct MyClass<T, SomeTag<TSpec> >
{
    typedef int isSpecialized;
};

template <typename T>
struct Factory {
    typedef SomeTag<InheritSomeTag<T> > Type;
};

//typename Factory<TSpec>::Type

template <class T, typename TSpec>
struct MyClass<T, SomeTag<InheritSomeTag<TSpec> > >
{
    typedef double isSpecialized;
};

//template <class T, typename TSpec>
//struct MyClass<T, SomeTag<InheritSomeTag<InheritSomeTag2<TSpec> > > >
//{
//    typedef float isSpecialized;
//};

//template <typename T = void>
//using I2Tag = SomeTag<InheritSomeTag<InheritSomeTag2<T> > >;

class Name1{};
class Name2{};
class Name3{};

class Name4{};
class Name5{};
class Name6{};

typedef VTL<Name1, Name2, Name3, Name4, Name5, Name6> List;

int main()
{
    MyClass<SomeTag<> >::isSpecialized test1; //ok
    MyClass<SomeTag<InheritSomeTag<> > >::isSpecialized test2; //ok
    MyClass<SomeTag<InheritSomeTag<InheritSomeTag2<> > > >::isSpecialized test3; //ok

    std::cout<<typeid(test1).name()<<std::endl;
    std::cout<<typeid(test2).name()<<std::endl;
    std::cout<<typeid(test3).name()<<std::endl;

    std::cout<<TypeNameFactory<RevertList<List>::Type>::name()<<std::endl;
}


