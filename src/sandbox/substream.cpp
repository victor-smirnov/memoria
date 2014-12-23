


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>

#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/list_tree.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

template <int I>
struct S {};

namespace memoria {
namespace bt {

template <Int I>
struct StructSizeProvider<S<I>> {
	static const Int Value = I;
};


}
}


using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;

typedef TypeList<
            IntValue<10>,
            IntValue<20>,
            TypeList<
                IntValue<1>,
                IntValue<2>,
                TypeList<
                    IntValue<2>,
                    TypeList<
                        IntValue<3>
                    >,
                    IntValue<4>
                >,
                IntValue<5>
            >,
            IntValue<5>
        >                                                                       List;


using namespace memoria::bt::detail;

class T {};

using SList = TypeList<
//        SubstreamSizeListBuilder<T>::Type,
//        SubstreamSizeListBuilder<TypeList<T, T, T, T>>::Type,
//        SubstreamSizeListBuilder<TypeList<TypeList<T>, T, T, T>>::Type,
//        SubstreamSizeListBuilder<TypeList<T, T, TypeList<T>, T, T, T, TypeList<T>, T, T>>::Type,
//        SubstreamSizeListBuilder<TypeList<T, TypeList<T>, T, T, TypeList<T>, T>>::Type,
//
//        SubstreamSizeListBuilder<TypeList<TypeList<T>>>::Type,
//
//        SubstreamSizeListBuilder<TypeList<TypeList<T>, T>>::Type,
//
//        SubstreamSizeListBuilder<TypeList<T, TypeList<T, TypeList<T>>>>::Type
>;

template <typename T, typename V = Int> struct PropValue;

template <typename T>
struct PropValue<T, decltype((void)T::VALUE, 0)> {
	static const Int Value = T::VALUE;
};

template <typename T>
struct PropValue<T, decltype((void)T::VALUE1, 0)> {
	static const Int Value = T::VALUE1;
};



struct VV {
	static const Int VALUE = 42;
};

struct VVV {
	static const Int VALUE1 = 42;
};


using List2 = IntList<
	LeafCount<TL<TL<T, TL<T, T, T>, T>, TL<T, T, T>, T>, IntList<2>, 3>::Value
>;


using List4 = TL<TL<S<2>, S<7>>, TL<S<3>, S<5>>>;
using List5 = Linearize<List4, 2>;

using List3 = TypeList<
		//detail::OffsetBuilder<List5>::Type,
		LeafOffsetListBuilder<List4>::Type

//		S<LeafCount<List4, IntList<1,1>, 1>::Value>
>;


int main(void) {

    cout<<TypeNameFactory<List>::name()<<endl;

//    cout<<SubstreamsTreeSize<List>::Size<<endl;

//    cout<<LeafOffsetCount<List, IntList<3>>::Value<<endl;

    ListPrinter<SList>::print(cout);

    cout<<PropValue<VV>::Value<<endl;

    cout<<TypeNameFactory<decltype(std::declval<int>())>::name()<<endl;


//    cout<<ListSize<T>::Value<<endl;
//
//    std::tuple<void> t;

    cout<<"LeafOffset: "<<endl;
    ListPrinter<List3>::print(cout);

    return 0;
}

