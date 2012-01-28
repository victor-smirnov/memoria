
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_KV_MAP_PARAMS_HPP_
#define MEMORIA_TESTS_KV_MAP_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/params.hpp"

namespace memoria {


struct KVMapReplay: public BTreeReplayParams {
	KVMapReplay(): BTreeReplayParams(){}
};


struct KVMapParams: public TestTaskParams {

	KVMapParams(): TestTaskParams("KVMap") {}
};


}


#endif
