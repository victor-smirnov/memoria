
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_TOOLS_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/sequence/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::sequence::CtrToolsName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Allocator::Page                                            Page;
	typedef typename Page::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
	typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
	typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
	typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
	typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;

	typedef typename Types::DataPage                                            DataPage;
	typedef typename Types::DataPageG                                           DataPageG;

	typedef typename Types::IDataSourceType                                     IDataSourceType;
	typedef typename Types::IDataTargetType                                     IDataTargetType;

	typedef typename Types::TreePath                                            TreePath;
	typedef typename Types::TreePathItem                                        TreePathItem;
	typedef typename Types::DataPathItem                                        DataPathItem;

	typedef typename Base::LeafNodeKeyValuePair                                 LeafNodeKeyValuePair;


	static const Int Indexes                                                    = Types::Indexes;
	typedef typename Types::Accumulator                                         Accumulator;

	typedef typename Types::ElementType                                         ElementType;

    DataPathItem getValuePage(const NodeBaseG& node, Int idx, Int flags) const
    {
        Value id = me()->getLeafData(node, idx);

        DataPathItem item;

        item.node()         = me()->allocator().getPage(id, flags);
        item.parent_idx()   = idx;

        return item;
    }

    bool IsDynarray() const {
        return true;
    }

    void finishPathStep(TreePath& path, Int key_idx) const;

    Int getDataPageCapacity(const DataPageG& node) const
    {
        return node->getCapacity();
    }

    Int getMaxDataPageCapacity() const
    {
        const Metadata& metadata = me()->getRootMetadata();
        return DataPage::getMaxPageCapacity(metadata.page_size());
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::sequence::CtrToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



//M_PARAMS
//typename M_TYPE::Iterator M_TYPE::findStart(bool reverse)
//{
//    Iterator i = Base::findStart(false);
//
//    if (i.leaf()->children_count() > 0)
//    {
//        me()->finishPathStep(i.path(), i.key_idx());
//
//        i.dataPos() = reverse ? -1 : 0;
//    }
//
//    return i;
//}
//
//
//
//M_PARAMS
//typename M_TYPE::Iterator M_TYPE::findEnd(bool reverse)
//{
//    Iterator i = Base::findEnd(false);
//
//    if (i.leaf()->children_count() > 0 && i.prevKey())
//    {
//        i.dataPos() = i.data()->size() + (reverse ? -1 : 0);
//    }
//
//    return i;
//}




M_PARAMS
void M_TYPE::finishPathStep(TreePath& path, Int key_idx) const
{
    if (key_idx >= 0 && key_idx < path.leaf()->children_count())
    {
        path.data().node()          = me()->getValuePage(path.leaf().node(), key_idx, Allocator::READ);

        path.data().parent_idx()    = key_idx;
    }
    else
    {
        path.data().clear();
    }
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
