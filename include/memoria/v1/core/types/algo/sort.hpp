
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

template <
        typename List,
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType = int64_t,
        typename Result = TypeList<> >
class Sort;



template <
        typename SrcList,
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType,
        typename Result>
class Sort {

    MEMORIA_V1_STATIC_ASSERT(IsList<SrcList>::Value);

    typedef IfThenElse<
                Asc,
                typename MinElement<SrcList, ValueProvider, ValueType>::Result,
                typename MaxElement<SrcList, ValueProvider, ValueType>::Result
    >                                                                           Element;

    typedef typename AppendTool<Element, Result>::Result                        List_;

public:
    typedef typename Sort<
                typename RemoveTool<Element, SrcList>::Result,
                ValueProvider,
                Asc,
                ValueType,
                List_
    >::List                                                                     List;
};


template <
        template <typename Item> class ValueProvider,
        bool Asc,
        typename ValueType,
        typename Result
>
class Sort<TypeList<>, ValueProvider, Asc, ValueType, Result> {
public:
    typedef Result                                                              List;
};


}}