
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

    typedef typename Types::DataPageG                                           DataPageG;

    typedef typename Base::Metadata                                             Metadata;

    static const Int Indexes                                                    = Base::Indexes;
    
    typedef typename Types::ElementType                                         ElementType;

    typedef IData<ElementType>                                                  IDataType;


    MEMORIA_PUBLIC Metadata createNewRootMetadata() const
    {
        Metadata metadata = Base::createNewRootMetadata();
        metadata.element_size() = 1;

        return metadata;
    }

    MEMORIA_PUBLIC Int getElementSize() const {
        return me()->getRootMetadata().element_size();
    }

    MEMORIA_PUBLIC void setElementSize(Int size)
    {
        Metadata meta       = me()->getRootMetadata();
        meta.element_size() = size;

        me()->setRootMetadata(meta);
    }

    MEMORIA_PUBLIC Iterator operator[](BigInt pos) {
        return seek(pos);
    }

    template <typename T>
    MEMORIA_PUBLIC void append(const T& value)
    {
        Iterator i = me()->seek(me()->size());
        i.insert(ArrayData<ElementType>(value));
    }

    Iterator seek(BigInt pos);
    BigInt size();

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::models::array::ApiName)
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
        return me()->getMaxKeys(node).key(0) / me()->getElementSize();
    }
    else {
        return 0;
    }
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
