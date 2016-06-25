
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


#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>

#include <memoria/v1/core/tools/object_pool.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::btfl::MiscName)
public:
    using typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::PageUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    static const Int Streams                = Types::Streams;
    static const Int DataStreams            = Types::DataStreams;
    static const Int StructureStreamIdx     = Types::StructureStreamIdx;

public:
    auto begin() {
        return self().template seek_stream<StructureStreamIdx>(0);
    }

    auto end() {
        auto& self = this->self();
        return self.template seek_stream<StructureStreamIdx>(self.size());
    }

    CtrSizeT size() const {
        return self().sizes()[StructureStreamIdx];
    }

    auto seek(CtrSizeT pos)
    {
        return self().template seek_stream<StructureStreamIdx>(pos);
    }

    auto seekL0(CtrSizeT pos)
    {
    	return self().select(pos + 1, 0);
    }


    auto seek(const CtrSizesT& pos, Int level)
    {
        auto& self = this->self();
        auto iter  = self.template seek_stream<StructureStreamIdx>(pos[0]);

        for (Int l = 1; l <= level; l++)
        {
            if (iter->is_data())
            {
                iter->toData(pos[l]);
            }
            else if (iter->isSEnd() && iter->size() > 0)
            {
                iter->skipBw(1);
                iter->toData(pos[l]);
            }
            else {
                break;
            }
        }

        return iter;
    }


    auto select(CtrSizeT rank, Int stream)
    {
        return self().template select_<IntList<StructureStreamIdx, 1>>(stream, rank);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::btfl::MiscName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
