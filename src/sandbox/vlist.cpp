
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/types/algo/select.hpp>

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


template <Int I>
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
