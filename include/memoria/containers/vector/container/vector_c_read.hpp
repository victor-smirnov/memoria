
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_READ_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_READ_HPP

#include <memoria/containers/vector/names.hpp>
#include <memoria/containers/vector/pages/data_page.hpp>

#include <memoria/prototypes/btree/btree.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::ReadName)

	typedef typename Base::Types                                                Types;

	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::ID                                                   ID;

	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
	typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
	typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
	typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
	typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;


	typedef typename Base::Metadata                                             Metadata;

	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;

	typedef typename Base::DataPageG                                            DataPageG;


	static const Int Indexes                                                    = Types::Indexes;

	typedef typename Types::ElementType                                         ElementType;
	typedef typename Types::IDataType                                           IDataType;

	BigInt read(Iterator& iter, IDataType& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::ReadName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::read(Iterator& iter, IDataType& data)
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
            SizeT processed = data.put(iter.data()->addr(iter.dataPos()), to_read_local);

            data.skip(processed);
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


#undef M_TYPE
#undef M_PARAMS

}


#endif
