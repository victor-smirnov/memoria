
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MODEL_INSERT_HPP

#include <memoria/vapi/models/logs.hpp>
#include <memoria/prototypes/btree/pages/tools.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::InsertName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;
    

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::TreeNodePage                                     	TreeNodePage;
    typedef typename Base::Counters                                             Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::NodeFactory                                          NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Base::Indexes;

    typedef typename Base::Metadata                                             Metadata;



    NodeBaseG CreateNode(Short level, bool root, bool leaf);

    void InsertEntry(Iterator &iter, const Key *keys, const Value &value) {}
    void InsertEntry(Iterator &iter, Key key, const Value &value) {}


MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::btree::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::CreateNode(Short level, bool root, bool leaf)
{
	NodeBaseG node = NodeFactory::Create(me()->allocator(), level, root, leaf);

	if (root) {
		Metadata meta = me()->GetRootMetadata(node);
		meta.model_name() = me()->name();
		me()->SetRootMetadata(node, meta);
	}

	node->model_hash() = me()->hash();

	return node;
}



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
