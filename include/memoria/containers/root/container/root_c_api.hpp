
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_ROOT_API_HPP
#define _MEMORIA_MODELS_ROOT_API_HPP


#include <memoria/containers/root/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::root::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;
    typedef typename Base::Accumulator                                          Accumulator;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Metadata                                             Metadata;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::Page                                                 Page;

    Metadata createNewRootMetadata() const
    {
    	Metadata meta = Base::createNewRootMetadata();

    	meta.model_name_counter() = INITAL_CTR_NAME_COUNTER;

    	return meta;
    }


    BigInt getModelNameCounter() const
    {
        const Metadata& meta = me()->getRootMetadata();
        return meta.model_name_counter();
    }

    void setModelNameCounter(BigInt value)
    {
        Metadata meta = me()->getRootMetadata();

        meta.model_name_counter() = value;

        me()->setRootMetadata(meta);
    }

    void addModelNameCounter(BigInt value)
    {
        Metadata meta = me()->getRootMetadata();

        meta.model_name_counter() += value;

        me()->setRootMetadata(meta);
    }

    static bool isRoot(const Page* page)
    {
        if (page->model_hash()      == MyType::hash())
        {
            const NodeBase* node = T2T<const NodeBase*>(page);
            return node->is_root();
        }

        return false;
    }

MEMORIA_CONTAINER_PART_END

}


#endif
