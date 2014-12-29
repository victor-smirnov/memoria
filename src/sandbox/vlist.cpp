
// Copyright Victor Smirnov 2014.
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
using namespace memoria::vapi;






int main(void) {

    typedef TypeList<int, float, bool> List0;

    cout<<TypeNameFactory<List0>::name()<<endl;
    cout<<TypeNameFactory<ListHead<List0>::Type>::name()<<endl;

    cout<<TypeNameFactory<AppendTool<double, List0>::Result>::name()<<endl;
    cout<<TypeNameFactory<AppendTool<List0, double>::Result>::name()<<endl;
    cout<<TypeNameFactory<AppendTool<List0, TypeList<double, unsigned> >::Result>::name()<<endl;
    cout<<TypeNameFactory<AppendTool<TypeList<double, unsigned>, List0 >::Result>::name()<<endl;

    cout<<TypeNameFactory<RemoveTool<int, List0>::Result>::name()<<endl;

    cout<<TypeNameFactory<RemoveTool<int, AppendTool<int, List0>::Result, true>::Result>::name()<<endl;

    cout<<IndexOfTool<int, List0>::Value<<endl;
    cout<<IndexOfTool<bool, AppendTool<List0, double>::Result>::Value<<endl;
    cout<<IndexOfTool<double, AppendTool<List0, double>::Result>::Value<<endl;
    cout<<IndexOfTool<unsigned, AppendTool<List0, double>::Result>::Value<<endl;

    cout<<TypeNameFactory<RemoveDuplicatesTool<AppendTool<AppendTool<TypeList<int, bool>, List0>::Result, double>::Result>::Result>::name()<<endl;


    cout<<TypeNameFactory<RevertList<AppendTool<List0, double>::Result>::Type>::name()<<endl;

    cout<<TypeNameFactory<AppendTool<TypeList<int, float>, TypeList<bool, double>>::Result>::name()<<endl;


    typedef MergeLists<
            TypeList<
                int,
                float
            >,
            double,
            TypeList <
                bool,
                void,
                Byte[1234]
            >
    >::Result Merge0;

    cout<<endl;
    cout<<TypeNameFactory<Merge0>::name()<<endl;
    return 0;
}
