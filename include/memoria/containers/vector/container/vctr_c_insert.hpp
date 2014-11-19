
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_vctr_C_INSERT_HPP
#define _MEMORIA_CONTAINER_vctr_C_INSERT_HPP


#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::CtrInsertName)

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
    typedef typename Base::DefaultDispatcher                                    DefaultDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::DataSource                                          DataSource;
    typedef typename Types::DataTarget                                          DataTarget;

    typedef typename Types::Source                                              Source;
    typedef typename Types::CtrSizeT                                            CtrSizeT;
    typedef typename Types::Value                                               Value;

    static const Int Streams                                                    = Types::Streams;

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

        return Accumulator(sizes);
    }

    Accumulator appendToLeaf(NodeBaseG& leaf, const Position& idx, Source& source)
    {
        auto& self = this->self();

        Position remainders = self.getRemainderSize(source);
        Position sizes      = self.getNodeSizes(leaf);
        Int capacity        = self.getStreamCapacity(leaf, sizes, 0);

        if (remainders[0] > capacity)
        {
            auto length = Position::create(0, capacity);

            LeafDispatcher::dispatch(leaf, InsertSourceFn(), source, idx, length);

            return Accumulator(length);
        }
        else {
            LeafDispatcher::dispatch(leaf, InsertSourceFn(), source, idx, remainders);

            return Accumulator(remainders);
        }
    }

    void fillNewLeaf(NodeBaseG& leaf, Source& source)
    {
        appendToLeaf(leaf, Position(0), source);
    }

    void insert(Iterator& iter, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::insert(Iterator& iter, DataSource& data)
{
    auto& self = this->self();
    auto& ctr  = self;

    Position idx(iter.idx());

    if (ctr.isNodeEmpty(iter.leaf()))
    {
        ctr.layoutLeafNode(iter.leaf(), 0);
    }

    Source source(&data);

    CtrSizeT inserted = data.getRemainder();

    ctr.insertSource(iter.leaf(), idx, source);

    ctr.addTotalKeyCount(Position(inserted));

    if (iter.isEof())
    {
        iter.nextLeaf();
    }

    iter.skipFw(inserted);
}







#undef M_PARAMS
#undef M_TYPE

}


#endif
