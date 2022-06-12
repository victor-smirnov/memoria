
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


#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>

#include <memoria/core/tools/object_pool.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::MiscName)
public:
    using typename Base::Types;

protected:
    using typename Base::TreeNodePtr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

//    static const int32_t DataStreams            = Types::DataStreams;

    // FIXME: some strange compiler behavior
    //static const int32_t StructureStreamIdx     = Types::StructureStreamIdx;

public:

    void dump_leafs(CtrSizeT leafs)
    {

    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::MiscName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
