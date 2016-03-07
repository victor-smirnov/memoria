
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/types/list/linearize.hpp>
#include <memoria/core/types/list/typelist.hpp>
#include <memoria/prototypes/bt/tools/bt_tools.hpp>



#include <iostream>

using namespace memoria;
using namespace memoria::bt;
using namespace memoria::list_tree;
using namespace std;

template <Int I>
class T{};


template <Int I, Int J>
class BB{};



using List1 = TL<
    TL<
        T<1>,
        TL<T<2>, T<3>, TL<T<11>, T<22>>>
    >,
    TL<
        TL<TL<T<33>, T<44>>, T<4>, T<5>>,
        T<6>
    >
>;


using List2 = TL<
        TL<TL<BB<0, 2>>>,
        TL<TL<BB<0, 1>>>
>;

int main() {
    ListPrinter<
        Linearize<List2, 3>
    >::print();
}


