
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_ROOT_API_HPP
#define	_MEMORIA_MODELS_ROOT_API_HPP


#include <memoria/containers/root/names.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::models::root::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;
	typedef typename Base::Key                                             		Key;
	typedef typename Base::Value                                             	Value;
	typedef typename Base::Element                                             	Element;
	typedef typename Base::Accumulator                                          Accumulator;
	typedef typename Base::Allocator                                          	Allocator;
	typedef typename Base::Metadata                                          	Metadata;
	typedef typename Base::NodeBaseG                                          	NodeBaseG;


    BigInt GetModelNameCounter() const
    {
    	const Metadata& meta = me()->GetRootMetadata();
    	return meta.model_name_counter();
    }

    void SetModelNameCounter(BigInt value)
    {
    	Metadata meta = me()->GetRootMetadata();

    	meta.model_name_counter() = value;

    	me()->SetRootMetadata(meta);
    }

    void AddModelNameCounter(BigInt value)
    {
    	Metadata meta = me()->GetRootMetadata();

    	meta.model_name_counter() += value;

    	me()->SetRootMetadata(meta);
    }

MEMORIA_CONTAINER_PART_END

}


#endif
