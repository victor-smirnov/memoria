
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_ARRAY_MODEL_API_HPP
#define _MEMORIA_MODELS_ARRAY_MODEL_API_HPP

#include <memoria/containers/vector/names.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/core/tools/bitmap.hpp>



namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::models::array::ApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::Page                                                 Page;
    typedef typename Base::ID                                                   ID;


    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::Counters                                             Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Node2RootMap                                         Node2RootMap;
    typedef typename Base::Root2NodeMap                                         Root2NodeMap;

    typedef typename Base::NodeFactory                                          NodeFactory;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Base::ApiKeyType                                           ApiKeyType;
    typedef typename Base::ApiValueType                                         ApiValueType;

    typedef typename Types::DataPage                                        	DataPage;
    typedef typename Types::DataPageG                                        	DataPageG;
    typedef typename Types::Buffer                                          	Buffer;
    typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
    typedef typename Types::CountData                                       	CountData;

    static const Int Indexes                                                    = Base::Indexes;

    void copy_data(const Buffer& data, DataPageG& page, BigInt start, BigInt pos, BigInt length);

    void move_data(DataPageG& from, DataPageG& to, BigInt local_idx, CountData &prefix, BigInt* keys);
    
    Iterator Find(BigInt pos, Int key_number);

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::models::array::ApiName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::copy_data(const Buffer& data, DataPageG& page, BigInt start, BigInt pos, BigInt length)
{
	page.update();
	memoria::CopyBuffer(data.data() + start, page->data().value_addr(pos), length);
}

M_PARAMS
void M_TYPE::move_data(DataPageG& from, DataPageG& to, BigInt local_idx, CountData &prefix, BigInt* keys)
{
	from.update();
	to.update();

	BigInt length = from->data().size() - local_idx;

	keys[0] = -length;

	memoria::CopyBuffer(to->data().value_addr(0), to->data().value_addr(length), to->data().size());

	memoria::CopyBuffer(from->data().value_addr(local_idx), to->data().value_addr(0), length);

	from->data().size() -= length;
	to->data().size() 	+= length;
}

M_PARAMS
typename M_TYPE::Iterator M_TYPE::Find(BigInt pos, Int key_number)
{
	Iterator iter = me()->FindLT(pos, key_number, true);

	if (iter.IsNotEmpty())
	{
		if (iter.IsEnd())
		{
			iter.PrevKey();
		}
		else {
			me()->FinishPathStep(iter.path(), iter.key_idx());
		}

		BigInt offset 	= iter.prefix(key_number);
		iter.data_pos() = pos - offset;

		if (iter.data_pos() > iter.data()->data().size())
		{
			iter.data_pos() = iter.data()->data().size();
		}
	}


	return iter;
}

#undef M_TYPE
#undef M_PARAMS


}



#endif
