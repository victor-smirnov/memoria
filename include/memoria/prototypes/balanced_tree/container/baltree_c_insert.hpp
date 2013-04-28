
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_INSERT_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::balanced_tree;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::InsertName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                         TreeNodePage;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position 											Position;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    static const Int ActiveStreams                                              = 1;


    template <typename Element>
    void insertEntry(Iterator& iter, Int stream, const Element&);


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::InsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
template <typename Element>
void M_TYPE::insertEntry(Iterator &iter, Int stream, const Element& element)
{
    TreePath&   path    = iter.path();
    NodeBaseG&  node    = path.leaf().node();
    Int&        idx     = iter.key_idx();

    auto& ctr  = self();

    if (ctr.isNodeEmpty(node))
    {
    	ctr.layoutNode(node, 1);
    }

    Int node_size = ctr.getNodeSize(node, 0);

    if (ctr.getCapacity(node) > 0)
    {
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else if (idx == 0)
    {
        TreePath next = path;
        ctr.splitPath(path, next, 0, Position(node_size / 2), ActiveStreams);
        idx = 0;

        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else if (idx < node_size)
    {
        //FIXME: does it necessary to split the page at the middle ???
        TreePath next = path;
        ctr.splitPath(path, next, 0, Position(idx), ActiveStreams);
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else {
        TreePath next = path;

        ctr.splitPath(path, next, 0, Position(node_size / 2), ActiveStreams);

        path = next;

        idx = ctr.getNodeSize(node, 0);
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }

    ctr.setLeafDataAndReindex(node, idx, element);

    ctr.updateParentIfExists(path, 0, element.first);

    ctr.addTotalKeyCount(1);
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
