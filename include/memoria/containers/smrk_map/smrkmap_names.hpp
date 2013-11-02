
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP_NAMES_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_NAMES_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {
namespace smrk_map   {

class CtrApiName    {};
class CtrInsertName {};
class CtrRemoveName {};

class ItrApiName {};
class ItrNavName {};

}

template <typename Types>
struct SMrkMapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct SMrkMapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using SMrkMapCtrTypes  = SMrkMapCtrTypesT<Types>;

template <typename Types>
using SMrkMapIterTypes = SMrkMapIterTypesT<Types>;


}

#endif
