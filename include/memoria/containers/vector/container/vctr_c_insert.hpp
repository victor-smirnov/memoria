
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


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::DataSource                                          DataSource;
    typedef typename Types::DataTarget                                          DataTarget;

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

    mvector::VectorSource source(&data);

    typename Base::DefaultSubtreeProvider provider(self, Position(data.getSize()), source);

    ctr.insertSubtree(iter.leaf(), idx, provider);

    ctr.addTotalKeyCount(Position(data.getSize()));

    if (iter.isEof())
    {
        iter.nextLeaf();
    }

    iter.skipFw(data.getSize());
}





#undef M_PARAMS
#undef M_TYPE

}


#endif
