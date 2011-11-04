
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ROOT_FACTORY_HPP
#define _MEMORIA_MODELS_ROOT_FACTORY_HPP


#include <memoria/containers/root/container/container.hpp>

namespace memoria {

template <typename Types>
struct RootCtrTypes: Types {};

template <typename Profile_, typename T>
class CtrTF<Profile_, Root, T> {

	typedef CtrTF<Profile_, Root, T> 											MyType;

	typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator	Allocator;

public:

	struct Types {
		typedef Profile_					Profile;
		typedef MyType::Allocator			Allocator;

		typedef RootCtrTypes<Types>		    CtrTypes;
	};

	typedef typename Types::CtrTypes 											CtrTypes;
	typedef Ctr<CtrTypes>														Type;
};


}

#endif
