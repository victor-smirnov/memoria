
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_ROOT_API_HPP
#define _MEMORIA_MODELS_ROOT_API_HPP


#include <memoria/containers/root/root_names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::root::CtrApiName)

    typedef typename Base::Metadata                                             Metadata;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::Page                                                 Page;


//    BigInt getModelNameCounter() const
//    {
//        const Metadata& meta = self().getRootMetadata();
//        return meta.model_name_counter();
//    }
//
//    void setModelNameCounter(BigInt value)
//    {
//        Metadata meta = self().getRootMetadata();
//
//        meta.model_name_counter() = value;
//
//        self().setRootMetadata(meta);
//    }
//
//    void addModelNameCounter(BigInt value)
//    {
//        Metadata meta = self().getRootMetadata();
//
//        meta.model_name_counter() += value;
//
//        self().setRootMetadata(meta);
//    }

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
