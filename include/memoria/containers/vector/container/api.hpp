
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
    typedef typename Base::ID                                                   ID;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::DataPageG                                        	DataPageG;

    typedef typename Base::Metadata                                             Metadata;

    static const Int Indexes                                                    = Base::Indexes;
    

    void ConfigureRootMetadata(Metadata& metadata) const
    {
    	Base::ConfigureRootMetadata(metadata);
    	metadata.element_size() = 1;
    }

    Int GetElementSize() const {
    	return me()->GetRootMetadata().element_size();
    }

    void SetElementSize(Int size)
    {
    	Metadata meta 		= me()->GetRootMetadata();
    	meta.element_size() = size;

    	me()->SetRootMetadata(meta);
    }

    Iterator Seek(BigInt pos);
    BigInt Size();

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::models::array::ApiName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Iterator M_TYPE::Seek(BigInt pos)
{
	return me()->Find(pos, 0);
}

M_PARAMS
BigInt M_TYPE::Size()
{
	NodeBaseG node = me()->GetRoot(Allocator::READ);

	if (node != NULL)
	{
		return me()->GetMaxKeys(node).key(0) / me()->GetElementSize();
	}
	else {
		return 0;
	}
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
