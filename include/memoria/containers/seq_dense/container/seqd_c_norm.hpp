
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_NORM_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_NORM_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrNormName)

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

    typedef typename Types::DataSource                                          DataSource;
    typedef typename Types::DataTarget                                          DataTarget;


    typedef typename Types::CtrSizeT                                            CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    static const Int MAIN_STREAM                                                = Types::MAIN_STREAM;


    struct InsertBlockIntoLeafFn {

        template <Int Idx, typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int length, DataSource& data, Accumulator& accum)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

            seq->insert(&data, idx, length);

            std::get<Idx>(accum).assignUp(seq->sums(idx, idx + length));
            std::get<Idx>(accum)[0] = length;
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, Int length, DataSource& data, Accumulator& accum)
        {
            node->layout(1);
            node->process(0, *this, idx, length, data, accum);
        }
    };

    struct UpdateBlockFn {

        template <Int Idx, typename SeqTypes>
        void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int length, DataSource& data, Accumulator& accum)
        {
            MEMORIA_ASSERT_TRUE(seq != nullptr);

            Accumulator accum1;
            std::get<Idx>(accum1).assignUp(seq->sums(idx, idx + length));

            seq->update(&data, idx, length);

            std::get<Idx>(accum).assignUp(seq->sums(idx, idx + length));

            VectorSub(accum, accum1);
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, Int length, DataSource& data, Accumulator& accum)
        {
            node->layout(1);
            node->process(0, *this, idx, length, data, accum);
        }
    };

    void insertBlockIntoLeaf(NodeBaseG& leaf, Int idx, Int length, DataSource& data)
    {
        self().updatePageG(leaf);

        Accumulator accum;
        LeafDispatcher::dispatch(leaf, InsertBlockIntoLeafFn(), idx, length, data, accum);

        self().updateParent(leaf, accum);
    }

    void updateBlockLeaf(NodeBaseG& leaf, Int idx, Int length, DataSource& data)
    {
        self().updatePageG(leaf);

        Accumulator accum;
        LeafDispatcher::dispatch(leaf, UpdateBlockFn(), idx, length, data, accum);

        self().updateParent(leaf, accum);
    }

    SizeT insertBlockIntoLeaf(Iterator& iter, DataSource& data)
    {
        UInt leaf_capacity = self().getStreamCapacity(iter.leaf(), Position::create(0, 0), 0);

        SizeT length;

        if (data.getRemainder() > leaf_capacity)
        {
            length = leaf_capacity;
        }
        else {
            length = data.getRemainder();
        }

        insertBlockIntoLeaf(iter.leaf(), iter.idx(), length, data);

        iter.idx() += length;

        return length;
    }


    void insertBlock(Iterator& iter, DataSource& data);




    CtrSizeT updateBlock(Iterator& iter, DataSource& data);



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrNormName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::insertBlock(Iterator& iter, DataSource& data)
{
    auto& self = this->self();
    auto& ctr  = self;

    if (ctr.isNodeEmpty(iter.leaf()))
    {
        ctr.layoutLeafNode(iter.leaf(), Position::create(0, 0));
    }

    UInt leaf_capacity = self.getStreamCapacity(iter.leaf(), Position::create(0, 0), 0);

    SizeT remainder = data.getRemainder();

    if (leaf_capacity >= remainder)
    {
        self.insertBlockIntoLeaf(iter, data);
    }
    else {
        NodeBaseG next;
        UInt next_capacity = 0;

        if (iter.isEnd())
        {
            if (leaf_capacity > 0)
            {
                // insert into this leaf
                self.insertBlockIntoLeaf(iter, data);
            }
        }
        else {
            // split leaf
            next            = self.splitLeafP(iter.leaf(), Position::create(0, iter.idx()));
            next_capacity   = self.getStreamCapacity(iter.leaf(), Position::create(0, 0), 0);

            self.insertBlockIntoLeaf(iter, data);
        }

        while(data.getRemainder() > 0)
        {
            iter.createEmptyLeaf();

            self.insertBlockIntoLeaf(iter, data);

            SizeT remainder = data.getRemainder();
            if (next.isSet() && remainder > 0 && remainder <= next_capacity)
            {
                self.insertBlockIntoLeaf(next, 0, remainder, data);

                iter.leaf() = next;
                iter.idx()  = remainder;

                break;
            }
        }
    }

    self.addTotalKeyCount(Position::create(0, remainder));
}



M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::updateBlock(Iterator& iter, DataSource& data)
{
    CtrSizeT sum = 0;
    CtrSizeT len = data.getRemainder();

    while (len > 0)
    {
        Int to_update = iter.leaf_size() - iter.idx();

        if (to_update > len) to_update = len;

        self().updateBlockLeaf(iter.leaf(), iter.idx(), to_update, data);

        len     -= to_update;
        sum     += to_update;

        iter.skipFw(to_update);

        if (iter.isEof())
        {
            break;
        }
    }

    return sum;

}


#undef M_PARAMS
#undef M_TYPE

}


#endif
