
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_TOOLS_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>






namespace memoria    {


using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::ToolsName)

public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
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

    typedef typename Base::Types::DataPage                                  	DataPage;
    typedef typename Base::Types::DataPageG                                  	DataPageG;

    static const Int Indexes                                                    = Types::Indexes;

    

    DataPage* GetDataPage(NodeBase *node, Int idx)
    {
        Value id = me_.GetLeafData(node, idx);
        return me_.allocator().GetPage(id);
    }

    NodeBase* GetDataParent(DataPage *node)
    {
    	return me_.allocator().GetPage(node->parent_id());
    }

    DataPage* InsertDataPage(NodeBase* node, Int key_idx)
    {
    	me_.InsertSpace(node, key_idx, 1);
    	return me_.create_datapage(node, key_idx);
    }

    bool IsDynarray() {
    	return true;
    }

MEMORIA_CONTAINER_PART_END



}


#endif
