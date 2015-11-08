
// Copyright Victor Smirnov 2012-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_CORE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_CORE_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/vector_tuple.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/core/tools/tuple_dispatcher.hpp>


#include <ostream>
#include <tuple>

namespace memoria   {
namespace bt        {


template <typename PkdStructList> struct MakeStreamEntryTL;

template <typename Head, typename... Tail>
struct MakeStreamEntryTL<TL<Head, Tail...>> {
	using Type = AppendItemToList<
			typename PkdStructInputType<Head>::Type,
			typename MakeStreamEntryTL<TL<Tail...>>::Type
	>;
};

template <>
struct MakeStreamEntryTL<TL<>> {
	using Type = TL<>;
};


template <typename List> struct TypeListToTupleH;

template <typename List>
using TypeListToTuple = typename TypeListToTupleH<List>::Type;

template <typename... List>
struct TypeListToTupleH<TL<List...>> {
	using Type = std::tuple<List...>;
};

}
}

#endif

