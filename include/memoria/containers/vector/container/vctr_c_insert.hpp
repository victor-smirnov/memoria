
// Copyright 2013 Victor Smirnov
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


#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrInsertName)
public:
    using Types = typename Base::Types;
    using typename Base::Iterator;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;


    typedef typename Types::CtrSizeT                                            CtrSizeT;
    typedef typename Types::Value                                               Value;

    static const int32_t Streams                                                    = Types::Streams;


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(mvector::CtrInsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}
