
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_KVMAP_MODEL_CHECKS_HPP
#define	_MEMORIA_MODELS_KVMAP_MODEL_CHECKS_HPP

#include <memoria/containers/kvmap/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::kvmap::ChecksName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;
    typedef typename Allocator::Transaction                                     Transaction;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                            NodeBaseG;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Types::Pages::NodeFactory                                  NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    static const Int Indexes                                                    = Types::Indexes;

    

    bool check_keys()
    {
    	Iterator i = me()->FindStart();

        Key k0 = 0;
        bool first = true;

        while (!i.IsEnd())
        {
            Key key = i.GetKey(0);
            if (!first)
            {
                if (k0 >= key)
                {
                	cout<<TypeNameFactory<typename Base::Name>::name()<<endl;
                    MEMORIA_ERROR(me(), "check_keys: key0 >= key1", k0, key, "page.id =", i.page()->id(), "page.idx =", i.key_idx());
                }
            }
            else {
                first = false;
            }

            k0 = key;

            i.Next();
        }

        return false;
    }

    bool check_node_content(NodeBaseG& node)
    {
    	bool errors = Base::check_node_content(node);

        Int children = node->children_count();

        Key k0;
        for (Int c = 0; c < children; c++)
        {
            Key key = me()->GetKey(node, 0, c);
            if (c > 0)
            {
                if (k0 >= key)
                {
                    MEMORIA_ERROR(me(), "check_node_content: key0 >= key1", k0, key, "page.id =", node->id());
                    errors = true;
                }
            }

            k0 = key;
        }

        return errors;
    }

MEMORIA_CONTAINER_PART_END

}

#endif
