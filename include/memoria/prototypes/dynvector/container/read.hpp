
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_READ_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_READ_HPP

#include <memoria/prototypes/dynvector/names.hpp>

#include <memoria/prototypes/btree/btree.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::ReadName)

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

		typedef typename Base::DataPageG                                        	DataPageG;


		static const Int Indexes                                                    = Types::Indexes;

		BigInt Read(Iterator& iter, IData& data, BigInt start, BigInt len);

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::ReadName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::Read(Iterator& iter, IData& data, BigInt start, BigInt len)
{
	BigInt sum = 0;

	while (len > 0)
	{
		Int to_read = iter.data()->size() - iter.data_pos();

		if (to_read > len) to_read = len;

		data.put(iter.data()->data().value_addr(iter.data_pos()), start, to_read);

		len 	-= to_read;
		iter.Skip(to_read / me()->getElementSize());

		sum 	+= to_read;
		start 	+= to_read;

		if (iter.IsEof())
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
