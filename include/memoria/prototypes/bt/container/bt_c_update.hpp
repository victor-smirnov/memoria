
// Copyright 2011 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {


MEMORIA_V1_CONTAINER_PART_BEGIN(bt::UpdateName)

    using typename Base::TreeNodePtr;
    using typename Base::Iterator;

    template <typename SubstreamsList, typename Buffer>
    void ctr_update_stream_entry(Iterator& iter, int32_t stream, int32_t idx, const Buffer& entry)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(iter.path(), 0);

        auto update_status0 = self.template ctr_try_update_stream_entry<SubstreamsList>(iter, idx, entry);

        if (!update_status0)
        {
            auto split_r = iter.iter_split_leaf(stream, idx);
            idx = split_r.stream_idx();

            auto update_status1 = self.template ctr_try_update_stream_entry<SubstreamsList>(iter, idx, entry);

            if (!update_status1)
            {
                MEMORIA_MAKE_GENERIC_ERROR("Second insertion attempt failed").do_throw();
            }
        }

        self.ctr_update_path(iter.path(), 0);
    }



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::UpdateName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
