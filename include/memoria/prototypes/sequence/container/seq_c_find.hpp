
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_FIND_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_FIND_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/sequence/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::sequence::CtrFindName)

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


    MEMORIA_PUBLIC Iterator operator[](BigInt pos) {
        return seek(pos);
    }

    Iterator seek(BigInt pos)
    {
    	MyType& ctr = *me();

    	typename Types::template SkipForwardWalker<
    		Types,
    		bt::EmptyExtender,
    		bt::EmptyExtender,
    		bt::EmptyExtenderState
    	> walker(pos, 0);

    	return ctr.find0(walker);
    }

    BigInt   size();

	BigInt read(Iterator& iter, IDataTargetType& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::sequence::CtrFindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


MEMORIA_PUBLIC M_PARAMS
BigInt M_TYPE::size()
{
    NodeBaseG node = me()->getRoot(Allocator::READ);

    if (node != NULL)
    {
        return me()->getMaxKeys(node).key(0);
    }
    else {
        return 0;
    }
}



M_PARAMS
BigInt M_TYPE::read(Iterator& iter, IDataTargetType& data)
{
    BigInt sum = 0;

    BigInt len = data.getRemainder();

    while (len > 0)
    {
        Int to_read = iter.data()->size() - iter.dataPos();

        if (to_read > len) to_read = len;

        BigInt to_read_local = to_read;

        while (to_read_local > 0)
        {
            SizeT processed = data.put(iter.data()->values(), iter.dataPos(), to_read_local);

            iter.skip(processed);

            to_read_local -= processed;
        }

        len     -= to_read;
        sum     += to_read;

        if (iter.isEof())
        {
            break;
        }
    }

    return sum;
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
