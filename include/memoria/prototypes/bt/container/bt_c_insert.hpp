
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::InsertName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    static const Int Streams                                                    = Types::Streams;

    static const Int ActiveStreams                                              = 3;



    template <typename EntryData>
    void insertEntry(Iterator& iter, const EntryData& data);

    template <typename EntryData>
    void insertEntry2(Iterator& iter, const EntryData& data);


    template <typename EntryData>
    void updateEntry(Iterator& iter, const EntryData&);


    void insertEntries(Iterator& iter, const Position& pos, ISource* src, std::function<void ()> split_fn = [](){});

    MEMORIA_DECLARE_NODE_FN(InsertIntoLeafFn, insert);
    bool insertIntoLeaf(Iterator& iter, const Position& pos, ISource* src)
    {
        auto& self = this->self();
        PageUpdateMgr mgr(self);

        try {
            Position sizes = self.getRemainder(src);

            LeafDispatcher::dispatch(iter.leaf(), InsertIntoLeafFn(), pos, sizes);

            return true;
        }
        catch (PackedOOMException ex)
        {
            mgr.rollback();
            return false;
        }
    }


    struct AppendToLeafFn {
        template <typename Node>
        void treeNode(const Node* node, ISource* src)
        {
            LayoutManager<Node> layout_manager(node);

            Position sizes;

            src->newNode(layout_manager, sizes.values());

            node->append(src, sizes);
        }
    };


    void appendToLeaf(Iterator& iter, ISource* src)
    {
        auto& self = this->self();

        LeafDispatcher::dispatch(iter.leaf(), AppendToLeafFn(), src);
    }


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::InsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename EntryData>
void M_TYPE::updateEntry(Iterator& iter, const EntryData& entry)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);
    mgr.add(iter.leaf());

    Accumulator delta;

    try {
        delta = self.setLeafEntry(iter.leaf(), iter.stream(), iter.idx(), entry);
    }
    catch (PackedOOMException ex)
    {
        mgr.rollback();
        throw ex;
    }

    self.updateParent(iter.leaf(), delta);
}


M_PARAMS
template <typename EntryData>
void M_TYPE::insertEntry(Iterator &iter, const EntryData& entry)
{
    NodeBaseG&  leaf    = iter.leaf();
    Int&        idx     = iter.idx();
    Int         stream  = iter.stream();

    auto& ctr  = self();

    Position leaf_sizes = ctr.getNodeSizes(leaf);

    if (ctr.isNodeEmpty(leaf))
    {
        ctr.initLeaf(leaf);
    }

    if (ctr.getStreamCapacity(leaf, stream) > 0)
    {
        ctr.makeRoom(leaf, stream, idx, 1);
    }
    else if (idx == 0)
    {
        auto next = ctr.splitLeafP(leaf, leaf_sizes / 2);
        idx = 0;

        ctr.makeRoom(leaf, stream, idx, 1);
    }
    else
    {
        Position split_idx = leaf_sizes / 2;

        auto next = ctr.splitLeafP(leaf, split_idx);

        if (idx < split_idx[stream])
        {
            ctr.makeRoom(leaf, stream, idx, 1);
        }
        else {
            idx -= split_idx[stream];

            leaf = next;
            ctr.makeRoom(leaf, stream, idx, 1);
        }
    }

    ctr.updateEntry(iter, entry);

    ctr.addTotalKeyCount(Position::create(stream, 1));
}


M_PARAMS
template <typename EntryData>
void M_TYPE::insertEntry2(Iterator& iter, const EntryData& data)
{
    auto& self = this->self();

    NodeBaseG&  leaf    = iter.leaf();
    Int&        idx     = iter.idx();
    Int         stream  = iter.stream();

    Position leaf_sizes = self.getNodeSizes(leaf);

    if (self.isNodeEmpty(leaf))
    {
        self.initLeaf(leaf);
    }

    if (!self.insertLeafEntry(iter, data))
    {
        Position split_idx = leaf_sizes / 2;

        NodeBaseG next = self.splitLeafP(leaf, split_idx);

        if (idx >= split_idx[stream])
        {
            idx -= split_idx[stream];
            leaf = next;
        }

        iter.buildPath(leaf);

        MEMORIA_ASSERT_TRUE(self.insertLeafEntry(iter, data));
    }

    self.addTotalKeyCount(Position::create(stream, 1));
}





M_PARAMS
void M_TYPE::insertEntries(
        Iterator& iter,
        const Position& pos,
        ISource* src,
        std::function<void ()> split_fn)
{
    auto& self = this->self();

    if (self.insertIntoLeaf(iter, pos, src))
    {
        return;
    }

    split_fn();

    self.appendToLeaf(iter, src);

    while (self.getRemainder(src).gtAny(0))
    {
        iter.createEmptyLeaf();
        self.appendToLeaf(iter, src);
    }
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
