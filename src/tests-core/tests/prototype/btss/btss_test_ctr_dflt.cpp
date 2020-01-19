
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



#include <memoria/core/container/ctr_impl_btss.hpp>

#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/allocators/inmem/common/container_collection_cfg.hpp>

#include "btss_ctr_impl.hpp"

namespace memoria {

using Profile = DefaultProfile<>;    


using Ctr1Name = BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::FIXED>;
using Ctr2Name = BTSSTestCtr<PackedSizeType::FIXED, PackedSizeType::VARIABLE>;
using Ctr3Name = BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::FIXED>;
using Ctr4Name = BTSSTestCtr<PackedSizeType::VARIABLE, PackedSizeType::VARIABLE>;

MMA1_INSTANTIATE_CTR_BTSS(Ctr1Name, Profile, 1)
MMA1_INSTANTIATE_CTR_BTSS(Ctr2Name, Profile, 2)
//MMA1_INSTANTIATE_CTR_BTSS(Ctr3Name, Profile, 3)
//MMA1_INSTANTIATE_CTR_BTSS(Ctr4Name, Profile, 4)
    
}

