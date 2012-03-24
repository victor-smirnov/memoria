
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_ROOTS_HPP
#define	_MEMORIA_CONTAINERS_ROOTS_HPP

#include <memoria/core/container/init.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria {

MEMORIA_DECLARE_PROFILE(StreamProfile<>, 0);

MEMORIA_DECLARE_ROOT_CTR(Map1, 			10);
MEMORIA_DECLARE_ROOT_CTR(Root, 			20);
//MEMORIA_DECLARE_ROOT_CTR(VectorMap, 	20);

//MEMORIA_DECLARE_ROOT_CTR(Set1, 		30);
//MEMORIA_DECLARE_ROOT_CTR(SumMap1, 		40);
//MEMORIA_DECLARE_ROOT_CTR(Vector, 		50);


}

#endif


