
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_REMOVE_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_REMOVE_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrRemoveName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    void remove(Iterator& from, Iterator& to);
    void remove(Iterator& from, CtrSizeT size);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::remove(Iterator& from, Iterator& to)
{
    auto& self = this->self();

    auto& from_path     = from.leaf();
    Position from_pos   = Position(from.key_idx());

    auto& to_path       = to.leaf();
    Position to_pos     = Position(to.key_idx());

    Accumulator keys;

    self.removeEntries(from_path, from_pos, to_path, to_pos, keys, true);

    from.idx() = to.idx() = to_pos.get();
}

M_PARAMS
void M_TYPE::remove(Iterator& from, CtrSizeT size)
{
    auto to = from;

    to.skip(size);

    auto& self = this->self();

    self.remove(from, to);

    from = to;

    from.refreshCache();
}

#undef M_PARAMS
#undef M_TYPE

}


#endif
