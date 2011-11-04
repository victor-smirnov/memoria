
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_FACTORY_HPP
#define _MEMORIA_MODELS_ARRAY_FACTORY_HPP




#include <memoria/containers/idx_map/factory.hpp>

#include <memoria/containers/array/iterator/model_api.hpp>
#include <memoria/containers/array/iterator/tools.hpp>

#include <memoria/containers/array/pages/data_page.hpp>

#include <memoria/containers/array/container/api.hpp>
#include <memoria/containers/array/container/model_api.hpp>

#include <memoria/containers/array/names.hpp>
#include <memoria/containers/array/tools.hpp>

#include <memoria/prototypes/dynvector/dynvector.hpp>

namespace memoria {

template <typename Profile>
struct BTreeTypes<Profile, memoria::Array>: public BTreeTypes<Profile, memoria::DynVector>  {

	typedef BTreeTypes<Profile, memoria::DynVector> 								Base;

	typedef typename AppendTool<
			typename Base::ContainerPartsList,
			typename TLTool<
				memoria::models::array::ApiName,
				memoria::models::array::ContainerApiName
			>::List
	>::Result                                                               		ContainerPartsList;

	typedef typename AppendTool<
			typename Base::IteratorPartsList,
			typename TLTool<
				memoria::models::array::IteratorToolsName,
				memoria::models::array::IteratorContainerAPIName
			>::List
	>::Result                                                               		IteratorPartsList;

	typedef memoria::models::array::CountData                             			CountData;
	typedef memoria::models::array::BufferContentDescriptor<CountData>          	BufferContentDescriptor;

	typedef ArrayData                                                     			Buffer;

	template <Int Size>
	struct DataBlockTypeFactory {
		typedef memoria::array::DynVectorData<Size>                       			Type;
	};
};


template <typename Profile, typename T>
class CtrTF<Profile, memoria::Array, T>: public CtrTF<Profile, memoria::DynVector, T> {

};

}

#endif
