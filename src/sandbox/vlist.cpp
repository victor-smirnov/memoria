
// Copyright 2016 Victor Smirnov
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



#include <typeinfo>
#include <iostream>

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types/list/typelist.hpp>
#include <memoria/v1/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;


template <typename StructList> struct BuildEmptyRangeList;

template <typename Head, typename... Tail>
struct BuildEmptyRangeList<TL<Head, Tail...>> {
    using Type = MergeLists<
        TL<TL<>>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >;
};


template <typename... List, typename... Tail>
struct BuildEmptyRangeList<TL<TL<List...>, Tail...>> {
    static_assert(
        sizeof...(List) > 0,
        "PackedStruct sub-list must not be empty"
    );

    using Type = MergeLists<
        TL<typename BuildEmptyRangeList<TL<List...>>::Type>,
        typename BuildEmptyRangeList<TL<Tail...>>::Type
    >;
};

template <>
struct BuildEmptyRangeList<TL<>> {
    using Type = TL<>;
};


template <int32_t I>
class Struct {};

using StructList =
TL<
    TL<
        TL<
            TL<
                Struct<0>
            >
        >,
        TL<
            Struct<1>
        >
    >,
    TL<
        Struct<2>
    >
>;


//using StructList = TL<Struct<0>, Struct<1>, Struct<2>>;

using RangeList = BuildEmptyRangeList<StructList>::Type;

int main(void) {

    ListPrinter<RangeList>::print(cout)<<endl;

    return 0;
}
