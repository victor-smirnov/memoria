
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::ChecksName)

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

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    static const Int Streams = Types::Streams;

    using PageUpdateMgt = typename Types::PageUpdateMgr;

public:
    bool check(void *data)
    {
        auto& self = this->self();
        return self.checkTree() || self.checkExtents();
    }

protected:
    bool checkExtents()
    {
        auto& self = this->self();

        auto sizes = self.sizes();
        auto ctr_totals = self.total_counts();

        if (ctr_totals != sizes)
        {
            MEMORIA_ERROR(self, "ctr_totals != sizes", (SBuf()<<ctr_totals).str(), (SBuf()<<sizes).str());
            return true;
        }

        auto i = self.begin();

        CtrSizesT extent;

        do
        {
            auto current_extent = i->leaf_extent();

            if (current_extent != extent)
            {
                MEMORIA_ERROR(self, "current_extent != extent", (SBuf()<<current_extent).str(), (SBuf()<<extent).str(), i->leaf()->id());

                return true;
            }

            for (Int c = 0; c < Streams; c++)
            {
                if (extent[c] < 0)
                {
                    MEMORIA_ERROR(self, "extent[c] < 0", (SBuf()<<extent).str(), i->leaf()->id());
                    return true;
                }
            }

            auto ex = self.node_extents(i->leaf());

            extent += ex;
        }
        while(i->nextLeaf());

        return false;
    }


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::ChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
