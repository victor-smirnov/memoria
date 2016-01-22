
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_FIND_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_FIND_H

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorFindName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <typename LeafPath>
    using TargetType = typename Container::Types::template TargetType<LeafPath>;

    template <typename Walker>
    auto find_fw(Walker&& walker)
    {
    	auto& self = this->self();

    	walker.prepare(self);

    	typename Container::NodeChain node_chain(self.leaf(), self.key_idx());

    	auto result = self.ctr().find_fw(node_chain, walker);

    	self.leaf() = result.node;
    	self.idx()  = result.idx;

    	walker.finish(self, result.idx, result.cmd);

    	return walker.result();
    }

    template <typename Walker>
    auto find_bw(Walker&& walker)
    {
        auto& self = this->self();

        walker.prepare(self);

        typename Container::NodeChain node_chain(self.leaf(), self.key_idx());

        auto result = self.ctr().find_bw(node_chain, walker);

        self.leaf() = result.node;
        self.idx()  = result.idx;

        walker.finish(self, result.idx, result.cmd);

        return walker.result();
    }



    template <typename LeafPath>
    auto find_fw_gt(Int index, TargetType<LeafPath> key)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(key, >=, 0);

    	typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);

    	return self().find_fw(walker);
    }

    template <typename LeafPath>
    auto find_fw_ge(Int index, TargetType<LeafPath> key)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(key, >=, 0);

    	typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);

    	return self().find_fw(walker);
    }


    template <typename LeafPath>
    auto find_bw_gt(Int index, TargetType<LeafPath> key)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(key, >=, 0);

    	typename Types::template FindGTBackwardWalker<Types, LeafPath> walker(index, key);

    	return self().find_bw(walker);
    }

    template <typename LeafPath>
    auto find_bw_ge(Int index, TargetType<LeafPath> key)
    {
    	MEMORIA_ASSERT(index, >=, 0);
    	MEMORIA_ASSERT(key, >=, 0);

    	typename Types::template FindGEBackwardWalker<Types, LeafPath> walker(index, key);

    	return self().find_bw(walker);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorFindName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}


#endif
