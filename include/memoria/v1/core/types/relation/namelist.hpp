
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

#include <memoria/v1/core/types/relation/expression.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {
namespace types     {
namespace relation  {

using namespace tools::types::algo;
using namespace tools::types::typelist;

template <typename Item, typename List>
class AddIfNotcontains {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

public:
    static const int32_t Idx = IndexOfTool<Item, List>::Value;
public:
    typedef IfThenElse<Idx == -1, TL<Item, List>, List> Result;
};


template <typename Expr, typename List = NullType> class NameListBuilder;

template <typename Type1, typename Type2, typename List>
class NameListBuilder<And<Type1, Type2>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};


template <typename Type1, typename Type2, typename List>
class NameListBuilder<Or<Type1, Type2>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};

template <typename Type1, typename Type2, typename List>
class NameListBuilder<Xor<Type1, Type2>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};

template <typename Type, typename List>
class NameListBuilder<Not<Type>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename NameListBuilder<Type, List>::Result                        Result;
};


template <int32_t Name, CompareOps Op, typename Type, Type ExValue, typename List>
class NameListBuilder<ValueOp<Name, Op, Type, ExValue>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename AddIfNotcontains<Name, List>::Result                       Result;
};

template <int32_t Name, typename Type, typename List>
class NameListBuilder<TypeOp<Name, Type>, List> {
    MEMORIA_V1_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename AddIfNotcontains<Name, List>::Result                       Result;
};


}
}
}}
