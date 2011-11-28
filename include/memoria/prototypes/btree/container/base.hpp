
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

#include <iostream>

namespace memoria    	{
namespace btree			{

MEMORIA_BTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBase)

    typedef typename Base::Name                                                 Name;
    typedef typename Base::Types                                                Types;
    typedef typename Types::KeysList                                        	KeysList;
    typedef typename Types::Value                                               Value;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Base::Iterator                                             Iterator;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef PageGuard<NodeBase>                                            		NodeBaseG;
    typedef typename Types::NodeBase::BasePageType                              TreeNodePage;
    typedef typename Types::Counters                                            Counters;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;


    typedef typename MaxElement<KeysList, TypeSizeValueProvider>::Result        Key;

    typedef Key                													ApiKeyType;

    typedef Value                                                               ApiValueType;

    typedef typename Types::Metadata 											Metadata;

    static const Int  Indexes                                                   = Types::Indexes;
    static const bool MapType                                                   = Types::MapType;

    bool IsDynarray() {
    	return false;
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
