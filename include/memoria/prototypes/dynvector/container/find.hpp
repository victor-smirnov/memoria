
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_FIND_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_CONTAINER_FINDs_HPP

#include <memoria/prototypes/dynvector/names.hpp>

#include <memoria/prototypes/btree/btree.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::SeekName)

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

		Int GetElementSize() const {
			return 1;
		}

		Iterator Find(BigInt pos, Int key_number);

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::SeekName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Iterator M_TYPE::Find(BigInt pos, Int key_number)
{
	Iterator iter = me()->FindLT(pos * me()->GetElementSize(), key_number);

	if (iter.IsNotEmpty())
	{
		if (iter.IsEnd())
		{
			iter.PrevKey();
		}
		else {
			me()->FinishPathStep(iter.path(), iter.key_idx());
		}

		BigInt offset 	= iter.prefix(); //FIXME: key_number
		iter.data_pos() = pos * me()->GetElementSize() - offset;

		if (iter.data_pos() > iter.data()->size())
		{
			iter.data_pos() = iter.data()->size();
		}
	}


	return iter;
}


#undef M_TYPE
#undef M_PARAMS

}


#endif
