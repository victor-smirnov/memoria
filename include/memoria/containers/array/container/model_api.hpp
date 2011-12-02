
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_MODEL_MODEL_API_HPP
#define _MEMORIA_MODELS_ARRAY_MODEL_MODEL_API_HPP

#include <memoria/containers/array/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::array::ContainerApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                              Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
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

    typedef typename Base::ApiKeyType                                           ApiKeyType;
    typedef typename Base::ApiValueType                                         ApiValueType;

    static const Int Indexes                                                    = Base::Indexes;



    struct AddKeysToMapFn {
    	Key *keys_;

    	AddKeysToMapFn(Key* keys): keys_(keys) {}

    	template <typename Node>
    	void operator()(Node *node)
    	{
    		for (Int c = 0; c < Indexes; c++) {
    			node->map().key(c, node->map().size()) = keys_[c];
    		}
    		node->map().size()++;
    		node->map().Reindex();
    	}
    };

    void AddKeysToMap(NodeBase *node, Key* keys)
    {
    	AddKeysToMapFn fn(keys);
    	NodeDispatcher::Dispatch(node, fn);
    }

    Iterator Seek(BigInt pos);
    virtual BigInt Size();

    virtual BigInt GetBlobSize(BigInt BlobId) {return 0;}
    virtual BigInt GetBlobsCount()            {return 0;}
    virtual bool RemoveBlob(BigInt BlobId)    {return 0;}

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::models::array::ContainerApiName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Iterator M_TYPE::Seek(BigInt pos)
{
	return me_.Find(pos, 0);
}

M_PARAMS
BigInt M_TYPE::Size()
{
	NodeBaseG node = me_.GetRoot();

	if (node != NULL)
	{
		return me_.GetMaxKey(node, 0);
	}
	else {
		return 0;
	}
}



#undef M_TYPE
#undef M_PARAMS

}


#endif
