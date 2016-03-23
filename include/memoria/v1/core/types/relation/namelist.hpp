
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/relation/expression.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace types     {
namespace relation  {

using namespace memoria::tools::types::algo;
using namespace memoria::tools::types::typelist;

template <typename Item, typename List>
class AddIfNotcontains {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

public:
    static const Int Idx = IndexOfTool<Item, List>::Value;
public:
    typedef IfThenElse<Idx == -1, TL<Item, List>, List> Result;
};


template <typename Expr, typename List = NullType> class NameListBuilder;

template <typename Type1, typename Type2, typename List>
class NameListBuilder<And<Type1, Type2>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};


template <typename Type1, typename Type2, typename List>
class NameListBuilder<Or<Type1, Type2>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};

template <typename Type1, typename Type2, typename List>
class NameListBuilder<Xor<Type1, Type2>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);

    typedef typename NameListBuilder<Type1, List>::Result                       Result1;
    typedef typename NameListBuilder<Type2, Result1>::Result                    Result2;
public:
    typedef Result2                                                             Result;
};

template <typename Type, typename List>
class NameListBuilder<Not<Type>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename NameListBuilder<Type, List>::Result                        Result;
};


template <Int Name, CompareOps Op, typename Type, Type ExValue, typename List>
class NameListBuilder<ValueOp<Name, Op, Type, ExValue>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename AddIfNotcontains<Name, List>::Result                       Result;
};

template <Int Name, typename Type, typename List>
class NameListBuilder<TypeOp<Name, Type>, List> {
    MEMORIA_STATIC_ASSERT(IsList<List>::Value);
public:
    typedef typename AddIfNotcontains<Name, List>::Result                       Result;
};


}
}
}
