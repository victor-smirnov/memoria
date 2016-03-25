
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


#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bttl::MiscName)
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

    static const Int Streams = Types::Streams;

    using PageUpdateMgt     = typename Types::PageUpdateMgr;

public:
    auto begin() {
        return self().template seek_stream<0>(0);
    }

    auto end() {
        auto& self = this->self();
        return self.template seek_stream<0>(self.size());
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    auto seek(CtrSizeT pos)
    {
        return self().template seek_stream<0>(pos);
    }

    auto seek(const CtrSizesT& pos, Int level)
    {
        auto& self = this->self();
        auto iter  = self.template seek_stream<0>(pos[0]);

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


    CtrSizesT compute_extent(const NodeBaseG& leaf)
    {
        auto& self = this->self();

        auto i = self.seek(0);

        CtrSizesT extent;

        while (i.leaf() != leaf)
        {
            extent += self.node_extents(i.leaf());

            if (!i.nextLeaf())
            {
                throw Exception(MA_SRC, "Premature end of tree");
            }
        }

        return extent;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bttl::MiscName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}