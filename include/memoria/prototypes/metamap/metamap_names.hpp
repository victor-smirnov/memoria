
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_MAP_NAMES_HPP
#define _MEMORIA_PROTOTYPES_MAP_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria   {
namespace metamap	{

class CtrFindName   {};
class CtrInsertName {};
class CtrRemoveName {};
class CtrNavName 	{};

class ItrApiName 	{};
class ItrNavName 	{};
class ItrValueName 	{};

}

template <typename Types>
struct MetaMapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct MetaMapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using MetaMapCtrTypes  = MetaMapCtrTypesT<Types>;

template <typename Types>
using MetaMapIterTypes = MetaMapIterTypesT<Types>;


}

#endif
