
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_NAMES_HPP
#define _MEMORIA_MODELS_ARRAY_NAMES_HPP

#include <memoria/prototypes/bstree/names.hpp>


namespace memoria       {
namespace mvector       {


template <typename ContainerName>
class DataPagePartsListFactory      {};

class InsertName            {};
class RemoveName            {};
class ToolsName             {};
class SeekName              {};
class ReadName              {};
class ChecksName            {};

class IteratorContainerAPIName  {};
class IteratorAPIName       {};
class IteratorToolsName     {};
class IteratorWalkName      {};

template <Int Indexes>
class IndexPagePrefixName   {};


class ApiName                   {};
class ContainerApiName          {};


}

template <typename Types>
struct VectorCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct VectorIterTypesT: IterTypesT<Types> {};

template <typename Types>
using VectorCtrTypes  = BSTreeCtrTypes<VectorCtrTypesT<Types>>;

template <typename Types>
using VectorIterTypes = BSTreeIterTypes<VectorIterTypesT<Types>>;

}

#endif
