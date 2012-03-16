
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

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;
    typedef typename Base::Types::DataPathItem                                  DataPathItem;
    

    DataPathItem GetDataPage(const NodeBase *node, Int idx, Int flags) const
    {
        Value id = me()->GetLeafData(node, idx);

        DataPathItem item;

        item.node() 		= me()->allocator().GetPage(id, flags);
        item.parent_idx() 	= idx;

        return item;
    }

    bool IsDynarray() const {
    	return true;
    }


//	bool GetNextData(TreePath& path, Int& idx) const;
//	bool GetPrevData(TreePath& path, Int& idx) const;

	Iterator FindStart();
	Iterator FindEnd();


    void FinishPathStep(TreePath& path, Int key_idx) const;

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::ToolsName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS



//M_PARAMS
//bool M_TYPE::GetNextData(TreePath& path, Int& idx) const
//{
//	if (idx < path[0]->children_count() - 1)
//	{
//		idx++;
//	}
//	else if (me()->GetNextNode(path))
//	{
//		idx = 0;
//	}
//	else {
//		return false;
//	}
//
//	path.data() = GetDataPage(path[0].node(), idx, Allocator::READ);
//
//	return true;
//}



//M_PARAMS
//bool M_TYPE::GetPrevData(TreePath& path, Int& idx) const
//{
//	if (idx > 0)
//	{
//		idx--;
//	}
//	else if (me()->GetPrevNode(path))
//	{
//		idx = path[0].node()->children_count() - 1;
//	}
//	else {
//		return false;
//	}
//
//	path.data() = me()->GetDataPage(path[0].node(), idx, Allocator::READ);
//
//	return true;
//}


M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindStart()
{
	Iterator i = Base::FindStart();

	if (i.path()[0]->children_count() > 0)
	{
		me()->FinishPathStep(i.path(), i.key_idx());

		i.data_pos() 	= 0;
	}

	return i;
}


M_PARAMS
typename M_TYPE::Iterator M_TYPE::FindEnd()
{
	Iterator i = Base::FindEnd();

	if (i.PrevKey())
	{
		i.data_pos() = i.data()->data().size();
	}

	return i;
}


M_PARAMS
void M_TYPE::FinishPathStep(TreePath& path, Int key_idx) const
{
	if (key_idx >= 0 && key_idx < path[0]->children_count())
	{
		path.data().node() 			= me()->GetDataPage(path[0].node(), key_idx, Allocator::READ);

		path.data().parent_idx()	= key_idx;
	}
	else
	{
		path.data().Clear();
	}
}

#undef M_PARAMS
#undef M_TYPE




}


#endif
