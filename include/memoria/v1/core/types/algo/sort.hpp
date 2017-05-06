
// Copyright 2011 Victor Smirnov
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


#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/algo/minmax.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>


namespace memoria {
namespace v1 {

namespace detail02 {    
    
    template <
            typename List,
            template <typename Item> class ValueProvider,
            bool Asc,
            typename ValueType = int64_t,
            typename Result = TypeList<> >
    class SortH;



    template <
            typename SrcList,
            template <typename Item> class ValueProvider,
            bool Asc,
            typename ValueType,
            typename Result>
    class SortH {

        MEMORIA_V1_STATIC_ASSERT(IsList<SrcList>::Value);

        using Element = IfThenElse<
                    Asc,
                    typename MinElement<SrcList, ValueProvider, ValueType>::Result,
                    typename MaxElement<SrcList, ValueProvider, ValueType>::Result
        >;

        using List_ = MergeLists<TL<Element>, Result>;

    public:
        using Type = typename SortH<
                    typename RemoveTool<Element, SrcList>::Result,
                    ValueProvider,
                    Asc,
                    ValueType,
                    List_
        >::Type;
    };


    template <
            template <typename Item> class ValueProvider,
            bool Asc,
            typename ValueType,
            typename Result
    >
    class SortH<TypeList<>, ValueProvider, Asc, ValueType, Result>: public HasType<Result> {};
}


template <
            typename List,
            template <typename Item> class ValueProvider,
            bool Asc,
            typename ValueType = int64_t,
            typename Result = TypeList<> 
>
using Sort = typename detail02::SortH<List, ValueProvider, Asc, ValueType, Result>::Type;

}}
