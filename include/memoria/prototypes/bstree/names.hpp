
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BSTREE_NAMES_HPP
#define _MEMORIA_PROTOTYPES_BSTREE_NAMES_HPP

#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {
namespace bstree      {

using namespace memoria::btree;

class FindName      {};
class ToolsName     {};

class ItrApiName {};



template <Int Indexes>
class IndexPagePrefixName   {};

}

template <typename Types>
struct BSTreeIterTypesT: IterTypesT<Types>      {};

template <typename Types>
struct BSTreeCtrTypesT: CtrTypesT<Types>        {};

template <typename Types>
using BSTreeCtrTypes = BTreeCtrTypes<BSTreeCtrTypesT<Types>>;

template <typename Types>
using BSTreeIterTypes = BTreeIterTypes<BSTreeIterTypesT<Types>>;

}


#endif  // _MEMORIA_PROTOTYPES_BTREE_NAMES_HPP

