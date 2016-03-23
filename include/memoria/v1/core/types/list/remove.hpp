
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/list/typelist.hpp>
#include <memoria/v1/core/types/list/append.hpp>

namespace memoria {
namespace v1 {

template <typename Item, typename List, bool All = false> struct RemoveTool;

template <typename Item, typename Head, typename ... Tail, bool All>
struct RemoveTool<Item, TypeList<Head, Tail...>, All> {
    typedef typename AppendTool<
                        Head,
                        typename RemoveTool<
                                    Item,
                                    TypeList<Tail...>,
                                    All
                        >::Result
                    >::Result                                                   Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, TypeList<Item, Tail...>, false> {
    typedef TypeList<Tail...>                                                   Result;
};

template <typename Item, typename ... Tail>
struct RemoveTool<Item, TypeList<Item, Tail... >, true> {
    typedef typename RemoveTool<
                        Item,
                        TypeList<Tail...>,
                        true
                    >::Result                                                   Result;
};


template <typename Item, bool All>
struct RemoveTool<Item, TypeList<>, All> {
    typedef TypeList<>                                                          Result;
};




template <typename List> struct RemoveDuplicatesTool;


template<>
struct RemoveDuplicatesTool<TypeList<>> {
    typedef TypeList<>                                                          Result;
};



template <typename Head, typename ... Tail>
struct RemoveDuplicatesTool<TypeList<Head, Tail...> > {
private:
    typedef typename RemoveDuplicatesTool<TypeList<Tail...>>::Result    TailResult;
    typedef typename RemoveTool<Head, TailResult>::Result       HeadResult;
public:
    typedef typename AppendTool<Head, HeadResult>::Result                       Result;
};

}}