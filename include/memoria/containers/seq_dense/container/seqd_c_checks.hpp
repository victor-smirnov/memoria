
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_seqd_CONTAINER_SEQ_C_CHECKS_HPP
#define _MEMORIA_CONTAINERS_seqd_CONTAINER_SEQ_C_CHECKS_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria    {



MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrChecksName)
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


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Types::DataPage                                            DataPage;
    typedef typename Types::DataPageG                                           DataPageG;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Symbols                                                    = DataPage::Sequence::Symbols;

    typedef typename Types::Accumulator                                         Accumulator;


    bool check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx) const;

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrChecksName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::check_leaf_value(const NodeBaseG& parent, Int parent_idx, const NodeBaseG& leaf, Int idx) const
{
    Accumulator keys    = me()->getKeys(leaf, idx);
    DataPageG data  	= me()->getValuePage(leaf, idx, Allocator::READ);

    if (data.isSet())
    {
        bool error = false;


        for (Int c = 0; c < Symbols; c++)
        {
        	BigInt rank = data->sequence().maxIndex(c);
        	BigInt rank1 = data->sequence().popCount(0, data->size(), c);

        	if (rank1 != rank)
        	{
        		MEMORIA_ERROR(me(), "Invalid data page index", data->id(), leaf->id(), idx, rank1, rank);
        		error = true;
        	}

        	if (keys[c + 1] != rank)
        	{
        		MEMORIA_ERROR(me(), "Invalid data rank size", data->id(), leaf->id(), idx, keys[c + 1], rank);
        		error = true;
        	}
        }

        if (keys[0] != data->size())
        {
            MEMORIA_ERROR(me(), "Invalid data page size", data->id(), leaf->id(), idx, keys[0], data->size());
            error = true;
        }

        if (error)
        {
        	me()->dump(leaf);
        	me()->dump(data);
        }

        return error;
    }
    else {
        MEMORIA_ERROR(me(), "No DataPage exists", leaf->id(), idx);
        return true;
    }
}




#undef M_TYPE
#undef M_PARAMS


}


#endif
