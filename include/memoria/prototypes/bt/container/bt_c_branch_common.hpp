
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_BRANCH_COMMON_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_BRANCH_COMMON_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::BranchCommonName)

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

    typedef std::function<Accumulator (NodeBaseG&, NodeBaseG&)>                 SplitFn;
    typedef std::function<void (const Position&, Int)>                          MergeFn;

    typedef typename Types::Source                                              Source;


    static const Int Streams                                                    = Types::Streams;

    void newRootP(NodeBaseG& root);

    MEMORIA_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, Accumulator);
    Accumulator splitBranchNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at);

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::BranchCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::splitBranchNode(NodeBaseG& src, NodeBaseG& tgt, Int split_at)
{
    auto& self = this->self();

    Accumulator accum = NonLeafDispatcher::dispatch(src, tgt, SplitNodeFn(), split_at);

    self.updateChildren(tgt);

    return accum;
}


M_PARAMS
void M_TYPE::newRootP(NodeBaseG& root)
{
    auto& self = this->self();

    self.updatePageG(root);

    NodeBaseG new_root = self.createNode1(root->level() + 1, true, false, root->page_size());

    UBigInt root_active_streams = self.getActiveStreams(root);
    self.layoutBranchNode(new_root, root_active_streams);

    self.copyRootMetadata(root, new_root);

    self.root2Node(root);

    Accumulator keys = self.sums(root);

    self.insertToBranchNodeP(new_root, 0, keys, root->id());

    root->parent_id()  = new_root->id();
    root->parent_idx() = 0;

    self.set_root(new_root->id());
}



#undef M_TYPE
#undef M_PARAMS

}



#endif
