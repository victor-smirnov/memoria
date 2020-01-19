
// Copyright 2016 Victor Smirnov
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



#pragma once

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include "../btfl_test_names.hpp"

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(btfl_test::CtrApiName)

public:
    using Types             = typename Base::Types;
    using Iterator          = typename Base::Iterator;


protected:
    using NodeBaseG         = typename Types::NodeBaseG;
    using Key               = typename Types::Key;
    using Value             = typename Types::Value;
    using CtrSizeT          = typename Types::CtrSizeT;
    using CtrSizesT         = typename Types::Position;
    using BranchNodeEntry   = typename Types::BranchNodeEntry;


    static const int32_t Streams = Types::Streams;

    using BlockUpdateMgr     = typename Types::BlockUpdateMgr;



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl_test::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
