
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

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btfl::ChecksName)

public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

public:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    static const int32_t Streams = Types::Streams;

    using BlockUpdateMgr = typename Types::BlockUpdateMgr;


    bool checkContent(const NodeBaseG& node) const
    {
    	auto& self = this->self();
    	if (!Base::checkContent(node))
    	{
    		if (node->is_leaf())
    		{
    			auto sizes = self.getLeafStreamSizes(node);

    			CtrSizeT data_streams_size = 0;
    			for (int32_t c = 0; c < CtrSizesT::Indexes - 1; c++)
    			{
    				data_streams_size += sizes[c];
    			}

    			if (data_streams_size != sizes[Streams - 1])
    			{
    				MMA1_ERROR(self, "Leaf streams sizes check failed", data_streams_size, sizes[Streams - 1]);
    				return true;
    			}
    		}

    		return false;
    	}
    	else {
    		return true;
    	}
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::ChecksName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
