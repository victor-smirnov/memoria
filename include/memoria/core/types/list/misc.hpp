
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP
#define	_MEMORIA_CORE_TOOLS_TYPES_LIST_MISC_HPP

#include <memoria/core/types/list/append.hpp>

namespace memoria    {

template <
        typename T1     = NullType,
        typename T2     = NullType,
        typename T3     = NullType,
        typename T4     = NullType,
        typename T5     = NullType,
        typename T6     = NullType,
        typename T7     = NullType,
        typename T8     = NullType,
        typename T9     = NullType,
        typename T10    = NullType,
        typename T11    = NullType,
        typename T12    = NullType,
        typename T13    = NullType,
        typename T14    = NullType,
        typename T15    = NullType,
        typename T16    = NullType,
        typename T17    = NullType,
        typename T18    = NullType,
        typename T19    = NullType,
        typename T20    = NullType
>
class TLTool {
    typedef typename appendTool<T1, NullType>::Result    L1;
    typedef typename appendTool<T2, L1>::Result          L2;
    typedef typename appendTool<T3, L2>::Result          L3;
    typedef typename appendTool<T4, L3>::Result          L4;
    typedef typename appendTool<T5, L4>::Result          L5;
    typedef typename appendTool<T6, L5>::Result          L6;
    typedef typename appendTool<T7, L6>::Result          L7;
    typedef typename appendTool<T8, L7>::Result          L8;
    typedef typename appendTool<T9, L8>::Result          L9;
    typedef typename appendTool<T10, L9>::Result         L10;
    typedef typename appendTool<T11, L10>::Result        L11;
    typedef typename appendTool<T12, L11>::Result        L12;
    typedef typename appendTool<T13, L12>::Result        L13;
    typedef typename appendTool<T14, L13>::Result        L14;
    typedef typename appendTool<T15, L14>::Result        L15;
    typedef typename appendTool<T16, L15>::Result        L16;
    typedef typename appendTool<T17, L16>::Result        L17;
    typedef typename appendTool<T18, L17>::Result        L18;
    typedef typename appendTool<T19, L18>::Result        L19;
    typedef typename appendTool<T20, L19>::Result        L20;

public:
    typedef L20                                          List;
};


template <typename List, typename Default> struct SelectHeadIfNotEmpty;

template <typename Head, typename Tail, typename Default>
struct SelectHeadIfNotEmpty<TL<Head, Tail>, Default> {
    typedef Head                                                                Result;
};

template <typename Default>
struct SelectHeadIfNotEmpty<NullType, Default> {
    typedef Default                                                             Result;
};


template <typename List>
struct ListSize {
    static const Int Value = ListSize<typename List::Tail>::Value + 1;
};

template <>
struct ListSize<NullType> {
    static const Int Value = 0;
};

template <typename Item>
struct AsListTool {
    typedef TL<Item>                                                      List;
};

template <typename Head, typename Tail>
struct AsListTool<TL<Head, Tail> > {
    typedef TL<Head, Tail>                                                List;
};


}

#endif	/* _MEMORIA_CORE_TOOLS_TYPES_TYPECHAIN_HPP */
