
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
#include <memoria/core/tools/checks.hpp>

#include <memoria/prototypes/bt_fl/btfl_tools.hpp>


#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::ChecksName)

public:
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Base::Streams;


    void ctr_check_content(const TreeNodeConstPtr& node, const CheckResultConsumerFn& fn) const
    {
    	auto& self = this->self();

        Base::ctr_check_content(node, fn);

        if (node->is_leaf())
        {
            auto sizes = self.ctr_get_leaf_stream_sizes(node);

            CtrSizeT data_streams_size = 0;
            for (int32_t c = 0; c < CtrSizesT::Indexes - 1; c++)
            {
                data_streams_size += sizes[c];
            }

            if (data_streams_size != sizes[Streams - 1])
            {
                fn(CheckSeverity::ERROR, make_string_document("Leaf streams sizes check failed: {} {}", data_streams_size, sizes[Streams - 1]));
            }
        }
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
