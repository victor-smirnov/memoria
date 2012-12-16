
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_NAMES_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_NAMES_HPP

#include <memoria/prototypes/bstree/names.hpp>

namespace memoria    {
namespace dynvector  {

using namespace memoria::bstree;

template <typename ContainerTypes>
struct DynVector {
    typedef ContainerTypes                                                          Types;
};

template <typename ContainerName>
class DataPagePartsListFactory      {};

class InsertName            {};
class RemoveName            {};
class ToolsName             {};
class SeekName              {};
class ReadName              {};
class checksName            {};

class IteratorContainerAPIName  {};
class IteratorAPIName       {};
class IteratorToolsName     {};
class IteratorWalkName      {};

template <Int Indexes>
class IndexPagePrefixName   {};

}

template <typename Types>
struct DynVectorCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct DynVectorIterTypesT: IterTypesT<Types> {};

template <typename Types>
using DynVectorCtrTypes = BSTreeCtrTypes<DynVectorCtrTypesT<Types>>;

template <typename Types>
using DynVectorIterTypes = BSTreeIterTypes<DynVectorIterTypesT<Types>>;



}


#endif
