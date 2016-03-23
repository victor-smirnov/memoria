
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memoria/v1/core/types/list/linearize.hpp>
#include <memoria/v1/core/types/list/typelist.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>



#include <iostream>

using namespace memoria;
using namespace v1::bt;
using namespace v1::list_tree;
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
