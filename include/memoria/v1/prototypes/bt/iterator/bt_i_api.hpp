
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt/walkers/bt_misc_walkers.hpp>

#include <iostream>


namespace memoria {
namespace v1 {

MEMORIA_V1_ITERATOR_PART_BEGIN(bt::IteratorAPIName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::BranchNodeEntry                           BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Types::Pages::BranchDispatcher                  BranchDispatcher;
    typedef typename Container::Types::Pages::NodeDispatcher                    NodeDispatcher;
    typedef typename Types::Position                                            Position;

    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;


    static const int32_t Streams = Container::Types::Streams;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SizesFn, sizes, Position);

public:
    auto leaf_sizes() const {
        return LeafDispatcher::dispatch(self().leaf(), SizesFn());
    }

    void refresh()
    {
        Base::refresh();

        self().refreshBranchPrefixes();
        self().refreshLeafPrefixes();
    }
    
    void check(std::ostream& out, const char* source) 
    {
#ifndef NDEBUG
        auto cache1 = self().cache();

        auto tmp = self().clone();
        tmp->refresh();

        auto cache2 = tmp->cache();

        if (cache1 != cache2)
        {
            self().dump(out);
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid iterator cache. Iterator: {} Actual: {}", cache1, cache2));
        }
#endif        
    }


protected:


    bool nextLeaf();
    bool nextLeafMs(uint64_t streams);


    bool prevLeaf();

    bool IsFound() {
        auto& self = this->self();
        return (!self.isEnd()) && self.isNotEmpty();
    }

    void dumpKeys(std::ostream& out) const
    {
        Base::dumpKeys(out);
    }

    CtrSizeT skipStreamFw(int32_t stream, CtrSizeT distance);
    CtrSizeT skipStreamBw(int32_t stream, CtrSizeT distance);
    CtrSizeT skipStream(int32_t stream, CtrSizeT distance);

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SizeFn, size, int32_t);

    int32_t leafSize(int32_t stream) const
    {
        return self().leaf_size(stream);
    }

    int32_t leaf_size(int32_t stream) const
    {
        return LeafDispatcher::dispatch(self().leaf(), SizeFn(), stream);
    }

    int32_t leaf_size() const
    {
        return LeafDispatcher::dispatch(self().leaf(), SizeFn(), self().stream());
    }





    bool has_no_data() const
    {
        return leaf_sizes().eqAll(0);
    }

    bool is_leaf_empty() const
    {
        return self().model().isNodeEmpty(self().leaf());
    }

    int32_t leaf_capacity(int32_t stream) const
    {
        auto& self = this->self();
        return self.leaf_capacity(Position(), stream);
    }

    int32_t leaf_capacity(const Position& reserved, int32_t stream) const
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

        auto next = ctr.split_leaf_p(self.leaf(), self.leaf_sizes());

        self.leaf() = next;

        self.idx() = 0;
    }

    template <int32_t Stream, typename SubstreamsList, typename... Args>
    void _updateStream(Args&&... args)
    {
        auto& self = this->self();
        auto& ctr  = self.model();

        ctr.template update_stream_entry<Stream, SubstreamsList>(self, std::make_tuple(std::forward<Args>(args)...));
    }


    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto read_leaf_entry(Args&&... args) const
    {
         return self().ctr().template apply_substreams_fn<Stream, SubstreamsIdxList>(self().leaf(), bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }

    template <typename Walker>
    void walkUpForRefresh(NodeBaseG node, int32_t idx, Walker&& walker) const
    {
        if (!node->is_leaf())
        {
            BranchDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
        }

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().ctr().getNodeParent(node);

            NodeDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
        }
    }

    void refreshBranchPrefixes()
    {
        auto& self  = this->self();
        auto& cache = self.cache();

        bt::FindForwardWalker<bt::WalkerTypes<Types, IntList<0>>> walker(0, 0);

        cache.reset();

        self.walkUpForRefresh(self.leaf(), self.idx(), walker);

        walker.finish(self, self.idx(), WalkCmd::REFRESH);
    }

    template <int StreamIdx>
    void _refreshLeafPrefixes()
    {
        auto& self  = this->self();
        auto idx    = self.idx();

        bt::FindForwardWalker<bt::WalkerTypes<Types, IntList<StreamIdx>>> walker(0, 0);
        LeafDispatcher::dispatch(self.leaf(), walker, WalkCmd::LAST_LEAF, 0, idx);
    }


    struct RefreshLeafPrefixesFn
    {
        template <int32_t StreamIdx, typename Iter>
        bool process(Iter&& iter)
        {
            if (StreamIdx == iter.stream())
            {
                iter.template _refreshLeafPrefixes<StreamIdx>();
            }

            return true;
        }
    };


    void refreshLeafPrefixes()
    {
        auto& self  = this->self();
        ForEach<1, Streams>::process(RefreshLeafPrefixesFn(), self);
    }


    


MEMORIA_V1_ITERATOR_PART_END


#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(bt::IteratorAPIName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStreamFw(int32_t stream, CtrSizeT amount)
{
    typedef typename Types::template SkipForwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    int32_t idx = self.model().findFw(self.leaf(), stream, self.idx(    ), walker);

    return walker.finish(self, idx);
}

M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStreamBw(int32_t stream, CtrSizeT amount)
{
    typedef typename Types::template SkipBackwardWalker<Types> Walker;

    auto& self = this->self();

    Walker walker(stream, 0, amount);

    walker.prepare(self);

    int32_t idx = self.model().findBw(self.leaf(), stream, self.idx(), walker);

    return walker.finish(self, idx);
}




M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipStream(int32_t stream, CtrSizeT amount)
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
bool M_TYPE::nextLeafMs(uint64_t streams)
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
    int32_t stream = self.stream();

    Walker walker(stream, 0);

    return self.findPrevLeaf(walker);
}



M_PARAMS
template <typename Walker>
bool M_TYPE::findNextLeaf(Walker&& walker)
{
    auto& self = this->self();

    NodeBaseG&  leaf    = self.leaf();
    int32_t stream      = self.stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().getNodeParent(leaf);

        int32_t idx = self.ctr().findFw(parent, stream, leaf->parent_idx() + 1, walker);

        int32_t size = self.ctr().getNodeSize(parent, stream);

        MEMORIA_V1_ASSERT_TRUE(size > 0);

        int32_t child_idx;

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
    int32_t stream      = self.stream();

    if (!leaf->is_root())
    {
        walker.prepare(self);

        NodeBaseG parent = self.ctr().getNodeParent(leaf);

        int32_t idx = self.model().findBw(parent, stream, leaf->parent_idx() - 1, walker);

        int32_t size = self.model().getNodeSize(parent, stream);

        MEMORIA_V1_ASSERT_TRUE(size > 0);

        int32_t child_idx;

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


}}
