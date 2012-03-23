
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_BASE_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>


#include <iostream>

namespace memoria    	{
namespace btree			{

MEMORIA_BTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBase)


    typedef typename Base::Types                                                Types;

    typedef typename Types::Key                                               	Key;
    typedef typename Types::Value                                               Value;
    typedef typename Types::Element                                             Element;
    typedef typename Types::Accumulator											Accumulator;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Types::NodeBase::BasePageType                              TreeNodePage;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

    typedef typename Types::Metadata 											Metadata;

    typedef typename Types::TreePathItem										TreePathItem;
    typedef typename Types::TreePath											TreePath;

    static const Int  Indexes                                                   = Types::Indexes;


    void operator=(ThisType&& other) {
    	Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other) {
    	Base::operator=(other);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
