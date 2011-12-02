
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_CHECKS_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_CHECKS_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::ChecksName)
private:

public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Types::DataPage                                        	DataPage;
    typedef typename Types::DataPageG                                        	DataPageG;
    typedef typename Types::Buffer                                         	 	Buffer;
    typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
    typedef typename Types::CountData                                       	CountData;


    static const Int Indexes                                                    = Types::Indexes;


    bool check_leaf_value(NodeBase* leaf, Int idx);


MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::ChecksName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::check_leaf_value(NodeBase* leaf, Int idx)
{
	Int key			= me_.GetKey(leaf, 0, idx);
	DataPageG data 	= me_.GetDataPage(leaf, idx);

	if (data != NULL)
	{
		bool error = false;

		if (key != data->data().size())
		{
			MEMORIA_TRACE(me_, "Invalid data page size", leaf->id(), idx, key, data->data().size());
			error = true;
		}

		if (key == 0)
		{
			MEMORIA_TRACE(me_, "Zero data page size", leaf->id(), idx, key, data->data().size());
			error = true;
		}

		return error;
	}
	else {
		MEMORIA_TRACE(me_, "No DataPage exists", leaf->id(), idx, key);
		return true;
	}
}




#undef M_TYPE
#undef M_PARAMS


}


#endif
