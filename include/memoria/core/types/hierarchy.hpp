
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria    {

using memoria::TypeList;

template <
    typename TChain,
    template <typename, typename> class Element,
    typename Base = EmptyType
>
struct SimpleHierarchy;

template <
    typename T1,
    typename ... T2,
    template <typename, typename> class Element,
    typename Base
>
struct SimpleHierarchy<TypeList<T1, T2...>, Element, Base>:
        public Element<T1, SimpleHierarchy<TypeList <T2...>, Element, Base> > {};

template <
    typename T,
    template <typename, typename> class Element,
    typename Base
>
struct SimpleHierarchy<TypeList<T>, Element, Base>: public Element<T, Base> {};

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_HIERARCHY_HPP */

