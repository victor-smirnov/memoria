
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_MISC_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_MISC_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::MiscName)
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

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using CtrSizesT = Position;

    static const Int Streams = Types::Streams;

    using PageUpdateMgt     = typename Types::PageUpdateMgr;

public:
    auto begin() {
        return self().template seek_stream<0>(0);
    }
//
//    auto End() {
//      auto& self = this->self();
//      return self.template seek_stream<0>(self.sizes()[0]);
//    }
//
//    auto begin() {
//      return self().template seek_stream<0>(0);
//    }
//
    auto end() {
        auto& self = this->self();
        return self.template seek_stream<0>(self.size());
    }
//
//
//
    CtrSizeT size() const {
        return self().sizes()[0];
    }
//
//    auto find(Key key)
//    {
//      auto iter = self().template find_ge<IntList<0>>(0, key);
//
//      iter.cache().data_size()[0] = self().size();
//      iter.cache().data_pos()[0]++;
//
//      return iter;
//    }
//
    auto seek(CtrSizeT pos)
    {
        return self().template seek_stream<0>(pos);
//
//      auto& cache = iter->cache();
//
//      cache.data_size()[0] = self().size();
//      cache.data_pos()[0]++;
//
//      return iter;
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


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::MiscName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
