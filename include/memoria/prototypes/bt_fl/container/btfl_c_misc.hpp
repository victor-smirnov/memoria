
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
    using typename Base::IteratorPtr;

protected:
    using typename Base::TreeNodePtr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    static const int32_t DataStreams            = Types::DataStreams;
    static const int32_t StructureStreamIdx     = Types::StructureStreamIdx;

public:

    void dump_leafs(CtrSizeT leafs)
    {
        auto ii = self().ctr_seq_begin();

        CtrSizeT lim = leafs >= 0? leafs : std::numeric_limits<CtrSizeT>::max();

        for (CtrSizeT cc = 0; cc < lim && !ii->is_end(); cc++) {
        ii->dump();
            if (!ii->iter_next_leaf()) {
                break;
            }
        }
    }

    auto ctr_seq_begin() {
        return self().template ctr_seek_stream<StructureStreamIdx>(0);
    }

    auto ctr_seq_end() {
        auto& self = this->self();
        return self.template ctr_seek_stream<StructureStreamIdx>(self.ctr_seq_size());
    }

    CtrSizeT ctr_seq_size() const {
        return self().sizes()[StructureStreamIdx];
    }

    auto ctr_seq_seek(CtrSizeT pos)
    {
        return self().template ctr_seek_stream<StructureStreamIdx>(pos);
    }

    auto ctr_seq_seekL0(CtrSizeT pos)
    {
    	return self().select(pos + 1, 0);
    }


    auto ctr_seq_seek(const CtrSizesT& pos, int32_t level)
    {
        auto& self = this->self();
        auto iter  = self.template ctr_seek_stream<StructureStreamIdx>(pos[0]);

        for (int32_t l = 1; l <= level; l++)
        {
            if (iter->is_data())
            {
                iter->toData(pos[l]);
            }
            else if (iter->isSEnd() && iter->size() > 0)
            {
                iter->iter_skip_bw(1);
                iter->toData(pos[l]);
            }
            else {
                break;
            }
        }

        return iter;
    }


    auto ctr_btfl_select(CtrSizeT rank, int32_t stream)
    {
        return self().template ctr_select<IntList<StructureStreamIdx, 1>>(stream, rank);
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btfl::MiscName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
