
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

    static const Int Streams                                                    = Types::Streams;

    static const Int MAIN_STREAM                                                = Types::MAIN_STREAM;

    struct RemoveFromLeafFn {

        template <Int Idx, typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Accumulator& delta)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

            typedef PkdFSSeq<SeqTypes>  Seq;
            typedef typename Seq::Values2                Values;

            Int sym = seq->symbol(idx);

            seq->remove(idx, idx + 1);

            Values indexes;

            indexes[0]       = -1;
            indexes[sym + 1] = -1;

            std::get<Idx>(delta) = indexes;
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int stream, Int idx, Accumulator& delta)
        {
            node->layout(1);
            node->process(stream, *this, idx, delta);
        }
    };

    struct RemoveBlockFromLeafFn {

        template <Int Idx, typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int length, Accumulator& delta)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

            std::get<Idx>(delta).assignUp(seq->sums(idx, idx + length));
            std::get<Idx>(delta)[0] = length;

            seq->remove(idx, idx + length);
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int stream, Int idx, Int length, Accumulator& delta)
        {
            node->layout(1);
            node->process(stream, *this, idx, length, delta);
        }
    };



    void removeFromLeaf(NodeBaseG& leaf, Int idx, Accumulator& indexes)
    {
        self().updatePageG(leaf);
        LeafDispatcher::dispatch(leaf, RemoveFromLeafFn(), 0, idx, indexes);
    }

    void removeBlockFromNode(NodeBaseG& leaf, Int idx, Int length)
    {
        self().updatePageG(leaf);

        Accumulator indexes;
        LeafDispatcher::dispatch(leaf, RemoveBlockFromLeafFn(), 0, idx, length, indexes);

        self().updateParent(leaf, -indexes);
    }

    void remove(CtrSizeT idx)
    {
        auto& self  = this->self();
        auto iter   = self.seek(idx);

        self.remove(iter);
    }

    void remove(Iterator& iter)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();
        Int stream  = iter.stream();

        Accumulator sums;

        self.removeFromLeaf(leaf, idx, sums);

        self.updateParent(leaf, sums);

        self.addTotalKeyCount(Position::create(stream, -1));

        self.mergeWithSiblings(leaf);
    }


    void removeBlock(Iterator& iter, CtrSizeT length)
    {
        auto& self  = this->self();

        auto merge_fn = [&iter](const Position& size, Int level){
            if (level == 0)
            {
                iter.idx() += size[0];
            }
        };

        if (iter.idx() + length <= iter.leaf_size())
        {
            self.removeBlockFromNode(iter.leaf(), iter.idx(), length);

            if (iter.isEnd())
            {
                iter.skipFw(0);
            }

            self.mergeWithSiblings(iter.leaf(), merge_fn);
        }
        else {
            auto tmp = iter;
            tmp.skipFw(length);

            Int from_length = iter.leaf_size() - iter.idx();
            self.removeBlockFromNode(iter.leaf(), iter.idx(), from_length);

            auto bkp_iter = iter;

            while(iter.nextLeaf() && iter.leaf() != tmp.leaf())
            {
            	self.removeNode(iter.leaf());
                iter = bkp_iter;
            }

            self.removeBlockFromNode(tmp.leaf(), 0, tmp.idx());

            tmp.idx() = 0;

            iter = tmp;

            self.mergeWithSiblings(iter.leaf(), merge_fn);
        }

        self.addTotalKeyCount(Position::create(0, -length));

        iter.refreshCache();
    }


    void removeBlock2(Iterator& iter, CtrSizeT length)
    {
        auto& self  = this->self();

        auto merge_fn = [&iter](const Position& size, Int level){
            if (level == 0)
            {
                iter.idx() += size[0];
            }
        };

        if (iter.idx() + length <= iter.leaf_size())
        {
            self.removeBlockFromNode(iter.leaf(), iter.idx(), length);

            if (iter.isEnd())
            {
                iter.skipFw(0);
            }

            self.mergeWithSiblings(iter.leaf(), merge_fn);
        }
        else {
            auto tmp = iter;
            tmp.skipFw(length);

            Int from_length = iter.leaf_size() - iter.idx();
            self.removeBlockFromNode(iter.leaf(), iter.idx(), from_length);

            auto bkp_iter = iter;

            while(iter.nextLeaf() && iter.leaf() != tmp.leaf())
            {
            	self.removeNode(iter.leaf());
                iter = bkp_iter;
            }

            self.removeBlockFromNode(tmp.leaf(), 0, tmp.idx());

            tmp.idx() = 0;

            iter = tmp;

            self.mergeWithSiblings(iter.leaf(), merge_fn);
        }

        self.addTotalKeyCount(Position::create(0, -length));

        iter.refreshCache();
    }

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
