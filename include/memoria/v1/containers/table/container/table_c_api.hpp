
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/containers/table/table_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/containers/table/table_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(table::CtrApiName)

    using Types             = typename Base::Types;

    using NodeBaseG         = typename Types::NodeBaseG;
    using Iterator          = typename Base::Iterator;

    using Key                   = typename Types::Key;
    using Value                 = typename Types::Value;
    using CtrSizeT            = typename Types::CtrSizeT;

    using BranchNodeEntry       = typename Types::BranchNodeEntry;
    using Position            = typename Types::Position;

    static const int32_t Streams = Types::Streams;

    using BlockUpdateMgr     = typename Types::BlockUpdateMgr;

    Iterator Begin() {
        return self().template _seek<0>(0);
    }

    Iterator End() {
        auto& self = this->self();
        return self.template _seek<0>(self.sizes()[0]);
    }


    CtrSizeT size() const {
        return self().sizes()[0];
    }

    Iterator find(Key key)
    {
        return self().template ctr_find_ge<IntList<0>>(0, key);
    }




MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(table::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
