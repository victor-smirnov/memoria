
// Copyright 2017 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.





#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/allocators/inmem/common/container_collection_cfg.hpp>

#include "container/btfl_test_factory.hpp"

#include "btfl_ctr_impl.hpp"

namespace memoria {

using Profile = DefaultProfile<>;    

using Ctr1Name = BTFLTestCtr<2>;
using Ctr2Name = BTFLTestCtr<4>;

MMA_INSTANTIATE_CTR_BTFL(Ctr1Name, Profile, 1)
MMA_INSTANTIATE_CTR_BTFL(Ctr2Name, Profile, 2)
    
}

