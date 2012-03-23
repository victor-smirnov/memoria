
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/allocators/stream/factory.hpp>

#include <iostream>

using namespace std;
using namespace memoria;


namespace memoria {

template class ::memoria::StreamAllocator<StreamProfile<>, BasicContainerCollectionCfg<StreamProfile<> >::Page, EmptyType>;
template class Checker<StreamContainerTypesCollection, DefaultStreamAllocator>;

}

