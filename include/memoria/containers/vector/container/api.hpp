
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR_API_HPP
#define _MEMORIA_CONTAINERS_VECTOR_API_HPP

#include <memoria/containers/vector/names.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/core/tools/bitmap.hpp>



namespace memoria    {


MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::ApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;

    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::DataPageG                                           DataPageG;

    typedef typename Base::Metadata                                             Metadata;

    static const Int Indexes                                                    = Base::Indexes;
    
    typedef typename Types::ElementType                                         ElementType;

    typedef IData<ElementType>                                                  IDataType;


    MEMORIA_PUBLIC Iterator operator[](BigInt pos) {
        return seek(pos);
    }

    Iterator seek(BigInt pos);
    BigInt   size();

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::ApiName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

MEMORIA_PUBLIC M_PARAMS
typename M_TYPE::Iterator M_TYPE::seek(BigInt pos)
{
    return me()->find(pos, 0);
}

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


#undef M_TYPE
#undef M_PARAMS


}



#endif
