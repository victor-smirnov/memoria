
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP
#define _MEMORIA_MODELS_BLOB_MAP_FACTORY_HPP


#include <memoria/containers/blob_map/container/container.hpp>
#include <memoria/containers/blob_map/iterator/iterator.hpp>

namespace memoria {

template <typename Types>
struct BlobMapCtrTypes: Types {};

template <typename Types>
struct BlobMapIterTypes: Types {};

template <typename Profile_, typename T>
class CtrTF<Profile_, BlobMap, T> {

	typedef CtrTF<Profile_, BlobMap, T> 										MyType;

	typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator	Allocator;

public:

	struct Types {
		typedef Profile_					Profile;
		typedef MyType::Allocator			Allocator;

		typedef BlobMapCtrTypes<Types>		CtrTypes;
		typedef BlobMapIterTypes<Types>		IterTypes;
	};

	typedef typename Types::CtrTypes 											CtrTypes;
	typedef typename Types::IterTypes 											IterTypes;

	typedef Ctr<CtrTypes>														Type;
};


}

#endif
