
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/core/types/list/typelist.hpp>

namespace memoria {

template <typename Item1, typename Item2> struct AppendTool {
	typedef TypeList<Item1, Item2>                                 	Result;
};

template <typename Item, typename ... List>
struct AppendTool<Item, TypeList<List...> > {
	typedef TypeList<Item, List...> 									Result;
};

template <typename ... List1, typename ... List2>
struct AppendTool<TypeList<List1...>, TypeList<List2...> > {
	typedef TypeList<List1..., List2...> 							Result;
};

template <typename ... List, typename Item>
struct AppendTool<TypeList<List...>, Item > {
	typedef TypeList<List..., Item> 									Result;
};


// Errors:
template <typename ... List>
struct AppendTool<TypeList<List...>, NullType>;

template <typename ... List>
struct AppendTool<NullType, TypeList<List...>>;

template <>
struct AppendTool<NullType, NullType>;

}

#endif  /* _MEMORIA_CORE_TOOLS_TYPES_LIST_APPEND_HPP */
