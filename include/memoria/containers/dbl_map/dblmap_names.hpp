
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_DBLMAP_NAMES_HPP
#define _MEMORIA_CONTAINERS_DBLMAP_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace dblmap     {

class CtrApiName    {};
class CtrApi2Name    {};

class OuterCtrApiName    {};
class OuterCtrInsertName {};

class InnerCtrApiName    {};
class InnerCtrInsertName {};

class CtrInsertName {};
class CtrToolsName  {};
class CtrRemoveName {};
class CtrChecksName {};
class CtrUpdateName {};

class ItrCRUDName 	{};

class ItrApi2Name 	{};

class OuterItrApiName 	{};

class InnerItrApiName 	{};
class InnerItrNavName 	{};

}

template <typename Types>
struct DblMapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct DblMapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using DblMapCtrTypes  = DblMapCtrTypesT<Types>;

template <typename Types>
using DblMapIterTypes = DblMapIterTypesT<Types>;








template <typename Types>
struct DblMap2CtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct DblMap2IterTypesT: IterTypesT<Types> {};



template <typename Types>
using DblMap2CtrTypes  = DblMap2CtrTypesT<Types>;

template <typename Types>
using DblMap2IterTypes = DblMap2IterTypesT<Types>;


}

#endif
