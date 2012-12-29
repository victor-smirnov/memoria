
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_CHECKS_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_CHECKS_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/vector/names.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::ChecksName)
private:

public:

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

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Types::DataPage                                            DataPage;
    typedef typename Types::DataPageG                                           DataPageG;

    static const Int Indexes                                                    = Types::Indexes;


    bool check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx);


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::ChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx)
{
    Int key         = me()->getKey(leaf, 0, idx);
    DataPageG data  = me()->getValuePage(leaf, idx, Allocator::READ);

    if (data.isSet())
    {
        bool error = false;

        if (key != data->size())
        {
            me()->dump(leaf);
            me()->dump(data);

            MEMORIA_ERROR(me(), "Invalid data page size", data->id(), leaf->id(), idx, key, data->size());
            error = true;
        }

//      if (key == 0)
//      {
//          MEMORIA_TRACE(me(), "Zero data page size", leaf->id(), idx, key, data->data().size());
//          error = true;
//      }

        return error;
    }
    else {
        MEMORIA_ERROR(me(), "No DataPage exists", leaf->id(), idx, key);
        return true;
    }
}




#undef M_TYPE
#undef M_PARAMS


}


#endif
