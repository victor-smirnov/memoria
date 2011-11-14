
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_NAMES_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_NAMES_HPP

namespace memoria    {
namespace dynvector  {

template <typename ContainerTypes>
struct DynVector {
    typedef ContainerTypes                                                          Types;
};

template <typename ContainerName>
class DataPagePartsListFactory      {};

class InsertName            {};
class RemoveName            {};
class ToolsName             {};
class SeekName             	{};
class ChecksName            {};

class IteratorContainerAPIName  {};
class IteratorAPIName       {};
class IteratorToolsName     {};
class IteratorWalkName      {};

template <Int Indexes>
class IndexPagePrefixName   {};

}
}


#endif
