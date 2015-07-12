
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>


#include <iostream>


namespace memoria    {

using namespace memoria::bt;


MEMORIA_ITERATOR_PART_BEGIN(memoria::bt::IteratorAPIName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Types::Position                                            Position;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;





    bool nextLeaf();
    bool nextLeafMs(UBigInt streams);


    bool prevLeaf();

    bool IsFound() {
        auto& self = this->self();
        return (!self.isEnd()) && self.isNotEmpty();
    }

    void dumpKeys(ostream& out) const
    {
        Base::dumpKeys(out);
    }

    CtrSizeT skipStreamFw(Int stream, CtrSizeT distance);
    CtrSizeT skipStreamBw(Int stream, CtrSizeT distance);
    CtrSizeT skipStream(Int stream, CtrSizeT distance);

    MEMORIA_DECLARE_NODE_FN_RTN(SizeFn, size, Int);

    Int leafSize(Int stream) const
    {
        return self().leaf_size(stream);
    }

    Int leaf_size(Int stream) const
    {
        return LeafDispatcher::dispatch(self().leaf(), SizeFn(), stream);
    }

    Int leaf_size() const
    {
        return LeafDispatcher::dispatch(self().leaf(), SizeFn(), self().stream());
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SizesFn, sizes, Int);

    Position leaf_sizes() const {
        return LeafDispatcher::dispatch(self().leaf(), SizesFn());
    }

    bool has_no_data() const
    {
        return leaf_sizes().eqAll(0);
    }

    bool is_leaf_empty() const
    {
        return self().model().isNodeEmpty(self().leaf());
    }

    Int leaf_capacity(Int stream) const
    {
        auto& self = this->self();
        return self.leaf_capacity(Position(), stream);
    }

    Int leaf_capacity(const Position& reserved, Int stream) const
    {
        auto& self = this->self();
        auto& ctr = self.model();

        return ctr.getStreamCapacity(self.leaf(), reserved, stream);
    }

    template <typename Walker>
    bool findNextLeaf(Walker&& walker);

    template <typename Walker>
    bool findPrevLeaf(Walker&& walker);

    void createEmptyLeaf()
    {
        auto& self = this->self();
        auto& ctr  = self.model();

        auto next = ctr.splitLeafP(self.leaf(), self.leaf_sizes());

        self.leaf() = next;

        self.idx() = 0;
    }

    template <Int Stream, typename SubstreamsList, typename... Args>
    void _updateStream(Args&&... args)
    {
    	auto& self = this->self();
    	auto& ctr  = self.model();

    	ctr.template updateStreamEntry<Stream, SubstreamsList>(self, std::make_tuple(std::forward<Args>(args)...));
    }





    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafStreamEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _readLeafEntry(Args&&... args) const -> ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>
    {
    	 return self().ctr().template _readLeafEntry<Stream, SubstreamsIdxList>(self().leaf(), std::forward<Args>(args)...);
    }



    template <Int StreamIdx>
    void _refreshCache()
    {
    	auto& self = this->self();

    	FindForwardWalker<bt::WalkerTypes<Types, IntList<StreamIdx>>> walker(0, 0);

    	self.cache().reset();

    	self.ctr().walkUp2(self.leaf(), self.idx(), walker);

    	walker.finish(self, self.idx(), WalkCmd::NONE);
    }


MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bt::IteratorAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStreamFw(Int stream, CtrSizeT amount)
{
    typedef typename Types::template SkipForwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    Int idx = self.model().findFw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}

M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStreamBw(Int stream, CtrSizeT amount)
{
    typedef typename Types::template SkipBackwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    Int idx = self.model().findBw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}




M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStream(Int stream, CtrSizeT amount)
{
    auto& self = this->self();

    if (amount > 0)
    {
        return self.skipStreamFw(stream, amount);
    }
    else if (amount < 0) {
        return self.skipStreamBw(stream, -amount);
    }
    else {
        return 0;
    }
}

M_PARAMS
bool M_TYPE::nextLeafMs(UBigInt streams)
{
    typedef typename Types::template NextLeafMutistreamWalker<Types, IntList<0>, IntList<0>> Walker;

    auto& self = this->self();

    Walker walker(streams);

    return self.findNextLeaf(walker);
}


M_PARAMS
bool M_TYPE::nextLeaf()
{
    typedef typename Types::template NextLeafWalker<Types, IntList<0>> Walker;
    Walker walker(self().stream(), 0);

    return self().findNextLeaf(walker);
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
    typedef typename Types::template PrevLeafWalker<Types, IntList<0>> Walker;

    auto& self = this->self();
    Int stream = self.stream();

    Walker walker(stream, 0);

    return self.findPrevLeaf(walker);
}



M_PARAMS
template <typename Walker>
bool M_TYPE::findNextLeaf(Walker&& walker)
{
    auto& self = this->self();

    NodeBaseG&  leaf    = self.leaf();
    Int         stream  = self.stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().getNodeParent(leaf);

        Int idx = self.ctr().findFw(parent, stream, leaf->parent_idx() + 1, walker);

        Int size = self.ctr().getNodeSize(parent, stream);

        MEMORIA_ASSERT_TRUE(size > 0);

        Int child_idx;

        if (idx < size)
        {
            child_idx = idx;
        }
        else {
            child_idx = size - 1;
        }

        // Step down the tree
        leaf = self.ctr().getChild(parent, child_idx);

        walker.finish(self, idx < size);

        self.idx() = 0;

        return idx < size;
    }
    else {
        return false;
    }
}



M_PARAMS
template <typename Walker>
bool M_TYPE::findPrevLeaf(Walker&& walker)
{
    auto& self = this->self();

    NodeBaseG&  leaf    = self.leaf();
    Int         stream  = self.stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().getNodeParent(leaf);

        Int idx = self.model().findBw(parent, stream, leaf->parent_idx() - 1, walker);

        Int size = self.model().getNodeSize(parent, stream);

        MEMORIA_ASSERT_TRUE(size > 0);

        Int child_idx;

        if (idx >= 0)
        {
            child_idx = idx;
        }
        else {
            child_idx = 0;
        }

        // Step down the tree
        leaf = self.model().getChild(parent, child_idx);

        walker.finish(self, idx >= 0);

        self.idx() = idx >= 0 ? self.leafSize(stream) - 1 : -1;

        return idx >= 0;
    }
    else {
        return false;
    }
}


#undef M_TYPE
#undef M_PARAMS


}


#endif
