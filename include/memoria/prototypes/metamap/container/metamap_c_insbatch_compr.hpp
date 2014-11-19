
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_METAMAP_CTR_INSBATCH_COMPR_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_CTR_INSBATCH_COMPR_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/prototypes/metamap/metamap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {



MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrInsBatchComprName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::DataSource                                          DataSource;

    typedef typename Types::Source                                              Source;
    typedef typename Types::EntrySizes                                          EntrySizes;
    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Entry                                               Entry;


    MEMORIA_DECLARE_NODE_FN(ComputeDataLengths, computeDataLengths);
    bool checkLeafCapacity(const NodeBaseG& node, const EntrySizes& sizes, SizeT total)
    {
        return false;
    }


    template <typename Entry>
    void computeDataLengths(const Entry& entry, EntrySizes& lengths)
    {
        LeafDispatcher::template dispatchStatic<LeafNode>(true, ComputeDataLengths(), entry, lengths);
    }

    std::pair<EntrySizes, SizeT> computeDataLengths(SizeT total, Source& source)
    {
        EntrySizes lengths;
        Clear(lengths);

        auto pos        = std::get<0>(source)->getStart();
        auto remainder  = std::get<0>(source)->getRemainder();

        if (remainder < total) total = remainder;

        for (SizeT c = 0; c < total; c++)
        {
            Entry entry = std::get<0>(source)->get();

            computeDataLengths(entry, lengths);
        }

        std::get<0>(source)->reset(pos);
        return std::pair<EntrySizes, SizeT>(lengths, total);
    }

    struct InsertSourceFn
    {
        template <typename Node>
        void treeNode(Node* node, Source& source, const Position& idx, const Position& sizes)
        {
            node->insert(source, idx, sizes);
        }
    };

    Accumulator insertSourceToLeaf(NodeBaseG& leaf, const Position& idx, Source& source)
    {
        auto& self = this->self();

        Position sizes = self.getRemainderSize(source);

        LeafDispatcher::dispatch(leaf, InsertSourceFn(), source, idx, sizes);

        return self.sums(leaf, idx, idx + sizes);
    }



    Accumulator appendToLeaf(NodeBaseG& leaf, const Position& idx, Source& source)
    {
        auto& self = this->self();

        self.computeDataLengths(10, source);

        Position remainders = self.getRemainderSize(source);
        Position sizes      = self.getNodeSizes(leaf);
        Int capacity        = self.getStreamCapacity(leaf, sizes, 0);

        if (remainders[0] > capacity)
        {
            auto length = Position::create(0, capacity);

            LeafDispatcher::dispatch(leaf, InsertSourceFn(), source, idx, length);

            return self.sums(leaf, idx, idx + length);
        }
        else {
            LeafDispatcher::dispatch(leaf, InsertSourceFn(), source, idx, remainders);

            return self.sums(leaf, idx, idx + remainders);
        }
    }

    void fillNewLeaf(NodeBaseG& leaf, Source& source)
    {
        appendToLeaf(leaf, Position(0), source);
    }

    Accumulator insert(Iterator& iter, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::metamap::CtrInsBatchComprName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insert(Iterator& iter, DataSource& data)
{
    auto& self = this->self();

    Position idx(iter.idx());

    if (self.isNodeEmpty(iter.leaf()))
    {
        self.layoutLeafNode(iter.leaf(), Position(0));
    }

    Source source(&data);

    CtrSizeT inserted = data.getRemainder();

    Accumulator sums = self.insertSource(iter.leaf(), idx, source);

    self.addTotalKeyCount(Position(inserted));

    if (iter.isEnd())
    {
        iter.nextLeaf();
    }

    iter.skipFw(inserted);

    return sums;
}







#undef M_PARAMS
#undef M_TYPE

}


#endif
