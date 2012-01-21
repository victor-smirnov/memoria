
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_HPP
#define	_MEMORIA_HPP

#include <memoria/vapi.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/allocators/stream/factory.hpp>

namespace memoria {

MEMORIA_DECLARE_PROFILE(StreamProfile<>, 0);


MEMORIA_DECLARE_ROOT_CTR(Root, 			10);
MEMORIA_DECLARE_ROOT_CTR(VectorMap, 	20);

MEMORIA_DECLARE_ROOT_CTR(DefKVMap, 		30);
MEMORIA_DECLARE_ROOT_CTR(SumSet1, 		40);
MEMORIA_DECLARE_ROOT_CTR(SumMap1, 		50);
MEMORIA_DECLARE_ROOT_CTR(Vector, 		60);


}

#endif


